// Draws a Face with OpenGL ES 2.0: per eye one rectangle just big enough
// for the eye and its glow, filled by the eye shader (eye_shader.hpp).
// Needs a current GLES 2.0 context.
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

    // A point on the panel, 0..1 from its top left corner (how SDL reports
    // touches), in logical pixels.
    void panel_to_logical(float px, float py, float &x, float &y) const;

    void draw(const Face &face);

private:
    void draw_eye(const Eye &eye);

    GLuint program_ = 0;
    GLuint quad_ = 0;  // vertex buffer: the unit square
    GLint u_rect_ = -1;
    GLint u_shape_ = -1;
    GLint u_radius_ = -1;
    GLint u_top_ = -1;
    GLint u_bottom_ = -1;
    GLint u_lids_ = -1;
    GLint u_color_ = -1;
    float width_ = 0;
    float height_ = 0;
    float panel_width_ = 0;
    float panel_height_ = 0;
    float row0_[2] {};  // panel pixels (origin bottom left) to logical
    float row1_[2] {};
    float offset_[2] {};
};

} // namespace dontfall::face
