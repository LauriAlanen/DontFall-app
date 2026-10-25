// Draws a Face with OpenGL ES 2.0: one full-screen rectangle and the eye
// shader (eye_shader.hpp). Needs a current GLES 2.0 context.
#pragma once

#include <string>

#include <SDL_opengles2.h>

#include "face.hpp"

namespace dontfall::face {

class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    ~Renderer();

    // panel_width/height: the drawable in panel pixels. rotation: 0, 90,
    // 180 or 270, how the logical screen is turned on the panel. False and
    // a message in error if the shaders don't build or rotation is wrong.
    bool init(int panel_width, int panel_height, int rotation, std::string &error);

    // Logical screen size, what Face coordinates are in.
    float width() const { return width_; }
    float height() const { return height_; }

    void draw(const Face &face);

private:
    GLuint program_ = 0;
    GLuint quad_ = 0;  // vertex buffer
    GLint a_pos_ = -1;
    GLint u_left_ = -1;
    GLint u_right_ = -1;
    GLint u_radius_ = -1;
    float width_ = 0;
    float height_ = 0;
};

} // namespace dontfall::face
