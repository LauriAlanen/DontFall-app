// dontfall-face: the robot's eyes on the DSI panel (docs/face-plan.md in
// DontFall).
//
// SDL2 opens the panel fullscreen through DRM/KMS with an OpenGL ES 2.0
// context (ST's gcnano driver); there is no compositor, this program owns
// the screen. Each frame the face model's numbers go to the eye shader,
// and the swap waits for vsync, which paces the loop.
//
// Runs until SIGTERM/SIGINT (SDL turns both into SDL_QUIT).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <SDL.h>

#include "face.hpp"
#include "renderer.hpp"

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {

// How the logical screen is turned on the portrait 480x800 panel. 90 =
// landscape; the real value comes with the robot (face plan, open questions).
constexpr int default_rotation = 90;

int usage()
{
    std::fprintf(stderr, "usage: dontfall-face [--rotation 0|90|180|270]\n");
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

    int rotation = default_rotation;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--rotation") == 0 && i + 1 < argc)
            rotation = std::atoi(argv[++i]);
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
            const auto face = dontfall::face::neutral_face(renderer.width(), renderer.height());
            for (bool running = true; running;) {
                SDL_Event event;
                while (SDL_PollEvent(&event))
                    if (event.type == SDL_QUIT)
                        running = false;
                renderer.draw(face);
                SDL_GL_SwapWindow(window);
            }
        } else {
            std::fprintf(stderr, "renderer: %s\n", error.c_str());
            status = 1;
        }
    }  // the renderer's GL objects go before the context

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return status;
}
