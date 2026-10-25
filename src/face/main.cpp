// dontfall-face: the robot's eyes on the DSI panel (docs/face-plan.md in
// DontFall).
//
// SDL2 opens the panel fullscreen through DRM/KMS with an OpenGL ES 2.0
// context (ST's gcnano driver); there is no compositor, this program owns
// the screen. Each frame the face model's numbers (the mood, the idle
// blinks and glances, and the googly physics driven by the IMU) go to the
// eye shader, and the swap waits for vsync, which paces the loop.
//
// Touch: poke an eye and it squints, tap elsewhere and the eyes look there,
// hold a finger down and they get dizzy. Touch never commands the robot.
//
// It is also the ROS 2 node dontfall_face: it subscribes to the M4's IMU
// (/dontfall_m4/imu) and statuses (/diagnostics); the robot's state picks
// the mood (mood_picker.hpp). The node spins in its own thread; the render
// loop only reads the latest values (face_node.hpp).
//
// --demo steps through the mood table, to see and tune every mood
// (temporary, face plan step 3); --mood <name> holds one of them.
// --stats prints the frame rate every few seconds; --gpu-time adds the
// GPU's drawing time per frame (it waits for the GPU with glFinish() each
// frame to measure it, which itself slows the loop down a little).
//
// Runs until SIGTERM/SIGINT (SDL turns both into SDL_QUIT; rclcpp's own
// signal handlers are off, the render loop shuts ROS down).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <SDL.h>

#include "face.hpp"
#include "face_node.hpp"
#include "googly.hpp"
#include "idle.hpp"
#include "mood.hpp"
#include "mood_picker.hpp"
#include "robot_state.hpp"
#include "renderer.hpp"
#include "tween.hpp"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

using namespace std::chrono_literals;
using dontfall::face::Clock;

// --demo: time on each mood, and how long the change to the next takes
constexpr auto demo_hold = 2s;
constexpr auto demo_change = 400ms;
// --stats: how often to print the frame rate
constexpr auto stats_period = 5s;
// IMU samples older than this count as no IMU (they come at 50 Hz).
constexpr auto imu_max_age = 500ms;
// A hit of the googly eyes against their edge at least this hard (0..1) is
// a knock that surprises the face.
constexpr float bump_hit = 0.6f;
// Touch: a press this long is a long press; a touch this close to an eye
// (logical pixels) pokes it.
constexpr auto long_press = 800ms;
constexpr float poke_margin = 30;

// How the logical screen is turned on the portrait 480x800 panel. 90 =
// landscape; the real value comes with the robot (face plan, open questions).
constexpr int default_rotation = 90;

int usage()
{
    std::fprintf(stderr, "usage: dontfall-face [--rotation 0|90|180|270] [--demo | --mood <name>]"
                         " [--stats] [--gpu-time]\n");
    return 1;
}

int fail(const char *what)
{
    std::fprintf(stderr, "%s: %s\n", what, SDL_GetError());
    SDL_Quit();
    return 1;
}

} // namespace

int main(int argc, char **argv)
{
    // stdout goes to journald under systemd: flush per line, not per 4 KiB.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::printf("dontfall-face %s\n", APP_VERSION);

    rclcpp::init(argc, argv, rclcpp::InitOptions(), rclcpp::SignalHandlerOptions::None);
    // our own arguments, without the --ros-args part
    const std::vector<std::string> args = rclcpp::remove_ros_arguments(argc, argv);

    int rotation = default_rotation;
    bool demo = false;
    const dontfall::face::Mood *fixed_mood = nullptr;
    bool stats = false;
    bool gpu_time = false;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--rotation" && i + 1 < args.size())
            rotation = std::atoi(args[++i].c_str());
        else if (args[i] == "--demo")
            demo = true;
        else if (args[i] == "--mood" && i + 1 < args.size()) {
            const std::string_view name = args[++i];
            for (const auto &m : dontfall::face::moods)
                if (m.name == name)
                    fixed_mood = &m.mood;
            if (!fixed_mood)
                return usage();
        } else if (args[i] == "--stats")
            stats = true;
        else if (args[i] == "--gpu-time")
            gpu_time = stats = true;
        else
            return usage();
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return fail("SDL_Init");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // On KMS/DRM the window is the whole panel, at its current mode.
    SDL_Window *window = SDL_CreateWindow("dontfall-face", 0, 0, 0, 0,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!window)
        return fail("SDL_CreateWindow");
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
        return fail("SDL_GL_CreateContext");
    SDL_GL_SetSwapInterval(1);  // swap on vsync
    SDL_ShowCursor(SDL_DISABLE);

    int panel_width = 0;
    int panel_height = 0;
    SDL_GL_GetDrawableSize(window, &panel_width, &panel_height);
    std::printf("video %s, %dx%d, %s\n", SDL_GetCurrentVideoDriver(), panel_width,
                panel_height, reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    int status = 0;
    {
        dontfall::face::Renderer renderer;
        std::string error;
        if (renderer.init(panel_width, panel_height, rotation, error)) {
            dontfall::face::Tween<dontfall::face::Mood> mood(
                fixed_mood ? *fixed_mood : dontfall::face::neutral);
            dontfall::face::Idle idle(Clock::now());
            dontfall::face::Googly googly;
            dontfall::face::MoodPicker picker(Clock::now());
            auto node = std::make_shared<dontfall::face::FaceNode>();
            std::thread ros([node] { rclcpp::spin(node); });
            auto last_frame = Clock::now();
            dontfall::face::Face face;  // as last drawn, for touches
            bool finger_down = false;
            bool long_pressed = false;
            Clock::time_point finger_since;
            size_t demo_mood = 0;
            auto next_mood = Clock::now() + demo_hold;
            int frames = 0;
            Clock::duration drawing {};
            auto stats_start = Clock::now();
            for (bool running = true; running;) {
                const auto now = Clock::now();
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = false;
                    } else if (event.type == SDL_FINGERDOWN && !finger_down) {
                        finger_down = true;
                        long_pressed = false;
                        finger_since = now;
                        float x = 0;
                        float y = 0;
                        renderer.panel_to_logical(event.tfinger.x, event.tfinger.y, x, y);
                        if (dontfall::face::on_eye(face.left, x, y, poke_margin) ||
                            dontfall::face::on_eye(face.right, x, y, poke_margin)) {
                            idle.blink_now(now);
                            picker.poke(now);
                        } else {
                            idle.look_at(
                                dontfall::face::gaze_towards(renderer.width(), renderer.height(), x, y),
                                now);
                        }
                    } else if (event.type == SDL_FINGERUP) {
                        finger_down = false;
                    }
                }
                if (finger_down && !long_pressed && now - finger_since >= long_press) {
                    long_pressed = true;
                    googly.spin();
                }

                if (demo && now >= next_mood) {
                    demo_mood = (demo_mood + 1) % dontfall::face::moods.size();
                    const auto &next = dontfall::face::moods[demo_mood];
                    std::printf("mood %.*s\n", static_cast<int>(next.name.size()),
                                next.name.data());
                    mood.go(next.mood, demo_change, now);
                    next_mood = now + demo_hold;
                }
                // Idle life says where to look and when to blink; the googly
                // physics moves the eyes there, pushed by the IMU.
                float push_x = 0;
                float push_y = 0;
                const auto sample = node->imu();
                if (sample && now - sample->received <= imu_max_age)
                    dontfall::face::imu_push(sample->ax, sample->ay, push_x, push_y);
                auto look = idle.at(now);
                googly.update(std::chrono::duration<float>(now - last_frame).count(), look.gaze,
                              push_x, push_y);
                last_frame = now;
                look.gaze = googly.gaze();
                look.squash_x = googly.squash_x();
                look.squash_y = googly.squash_y();

                // The robot's state picks the mood, unless one was asked for.
                if (!demo && !fixed_mood) {
                    const auto status = node->m4_status();
                    const auto state = dontfall::face::robot_state(
                        now, sample ? &sample->received : nullptr, status ? &*status : nullptr);
                    dontfall::face::MoodChoice choice;
                    if (picker.pick(now, state, googly.hit() >= bump_hit, choice)) {
                        const auto name = dontfall::face::mood_name(choice.mood);
                        const bool fault = state == dontfall::face::RobotState::fault;
                        std::printf("mood %.*s (%s%s%s)\n", static_cast<int>(name.size()),
                                    name.data(), dontfall::face::to_string(state),
                                    fault ? ": " : "", fault ? status->fault_text.c_str() : "");
                        mood.go(*choice.mood, choice.change, now);
                    }
                }

                face = dontfall::face::make_face(renderer.width(), renderer.height(), mood.at(now),
                                                 look);
                renderer.draw(face);
                if (gpu_time) {
                    glFinish();  // wait for the GPU to finish this frame
                    drawing += Clock::now() - now;
                }
                SDL_GL_SwapWindow(window);

                ++frames;
                if (stats && now - stats_start >= stats_period) {
                    using ms = std::chrono::duration<float, std::milli>;
                    std::printf("%.1f fps",
                                frames / std::chrono::duration<float>(now - stats_start).count());
                    if (gpu_time)
                        std::printf(", drawing %.1f ms per frame", ms(drawing).count() / frames);
                    std::printf("\n");
                    frames = 0;
                    drawing = {};
                    stats_start = now;
                }
            }
            rclcpp::shutdown();  // ends rclcpp::spin()
            ros.join();
        } else {
            std::fprintf(stderr, "renderer: %s\n", error.c_str());
            status = 1;
        }
    }  // the renderer's GL objects go before the context

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (rclcpp::ok())
        rclcpp::shutdown();
    return status;
}
