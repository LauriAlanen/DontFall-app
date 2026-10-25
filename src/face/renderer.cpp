#include "renderer.hpp"

#include <cmath>

#include "eye_shader.hpp"

namespace dontfall::face {

namespace {

// Glow around the eyes: logical pixels until it has faded to zero, and its
// brightness right at the edge (times the eye colour). The glow sets how
// big each eye's rectangle is, so a wider one costs GPU time: 12 keeps the
// heaviest mood (sleepy, both lids) at 15.6 ms per frame, 20 already drops
// frames (face plan step 3).
constexpr float glow_width = 12;
constexpr float glow_strength = 0.45f;

// A bottom lid with no curve is a circle so big its edge is straight.
constexpr float min_lid_curve = 1e-4f;

GLuint compile(GLenum type, const char *source, std::string &error)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        glGetShaderInfoLog(shader, sizeof log, nullptr, log);
        error = std::string(type == GL_VERTEX_SHADER ? "vertex" : "fragment") +
                " shader: " + log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Panel pixels to logical pixels: logical = (row0 . p, row1 . p) + offset.
// Panel pixels have their origin bottom left, logical ones top left.
struct Transform {
    float row0[2];
    float row1[2];
    float offset[2];
};

bool transform_for(int rotation, float w, float h, Transform &t)
{
    switch (rotation) {
    case 0:   t = {{1, 0}, {0, -1}, {0, h}}; return true;
    case 90:  t = {{0, -1}, {-1, 0}, {h, w}}; return true;
    case 180: t = {{-1, 0}, {0, 1}, {w, 0}}; return true;
    case 270: t = {{0, 1}, {1, 0}, {0, 0}}; return true;
    default:  return false;
    }
}

} // namespace

Renderer::~Renderer()
{
    if (quad_)
        glDeleteBuffers(1, &quad_);
    if (program_)
        glDeleteProgram(program_);
}

bool Renderer::init(int panel_width, int panel_height, int rotation, std::string &error)
{
    const float w = static_cast<float>(panel_width);
    const float h = static_cast<float>(panel_height);
    Transform t;
    if (!transform_for(rotation, w, h, t)) {
        error = "rotation must be 0, 90, 180 or 270";
        return false;
    }
    const bool sideways = rotation == 90 || rotation == 270;
    width_ = sideways ? h : w;
    height_ = sideways ? w : h;

    GLuint vs = compile(GL_VERTEX_SHADER, eye_vertex_shader, error);
    if (!vs)
        return false;
    GLuint fs = compile(GL_FRAGMENT_SHADER, eye_fragment_shader, error);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);  // freed with the program
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        glGetProgramInfoLog(program_, sizeof log, nullptr, log);
        error = std::string("link: ") + log;
        return false;
    }

    glUseProgram(program_);
    u_rect_ = glGetUniformLocation(program_, "u_rect");
    u_shape_ = glGetUniformLocation(program_, "u_shape");
    u_radius_ = glGetUniformLocation(program_, "u_radius");
    u_top_ = glGetUniformLocation(program_, "u_top");
    u_bottom_ = glGetUniformLocation(program_, "u_bottom");
    u_lids_ = glGetUniformLocation(program_, "u_lids");
    u_color_ = glGetUniformLocation(program_, "u_color");
    // These never change while running.
    glUniform2fv(glGetUniformLocation(program_, "u_row0"), 1, t.row0);
    glUniform2fv(glGetUniformLocation(program_, "u_row1"), 1, t.row1);
    glUniform2fv(glGetUniformLocation(program_, "u_offset"), 1, t.offset);
    glUniform2f(glGetUniformLocation(program_, "u_panel"), w, h);
    glUniform2f(glGetUniformLocation(program_, "u_glow"), glow_width, glow_strength);

    // The unit square as a triangle strip; u_rect places it.
    const GLfloat corners[] = {0, 0, 1, 0, 0, 1, 1, 1};
    const GLint a_corner = glGetAttribLocation(program_, "a_corner");
    glGenBuffers(1, &quad_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_);
    glBufferData(GL_ARRAY_BUFFER, sizeof corners, corners, GL_STATIC_DRAW);
    glVertexAttribPointer(a_corner, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(a_corner);

    glViewport(0, 0, panel_width, panel_height);
    glClearColor(0, 0, 0, 1);
    return true;
}

void Renderer::draw(const Face &face)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform3f(u_color_, face.color.r, face.color.g, face.color.b);
    draw_eye(face.left);
    draw_eye(face.right);
}

void Renderer::draw_eye(const Eye &eye)
{
    const float half_width = eye.width / 2;
    const float half_height = eye.height / 2;

    // Just the eye and its glow; the lids only ever cut inside the eye.
    const float margin = glow_width + 1;
    glUniform4f(u_rect_, eye.x - half_width - margin, eye.y - half_height - margin,
                eye.x + half_width + margin, eye.y + half_height + margin);
    glUniform4f(u_shape_, eye.x, eye.y, half_width, half_height);
    glUniform1f(u_radius_, eye.radius);

    // Top lid: a line through the point lid_top below the eye's top edge,
    // tilted by lid_angle. Its normal points up (y is down), to the cut side.
    const float nx = std::sin(eye.lid_angle);
    const float ny = -std::cos(eye.lid_angle);
    const float top_y = eye.y - half_height + eye.lid_top;
    glUniform3f(u_top_, nx, ny, nx * eye.x + ny * top_y);

    // Bottom lid: a circle whose top is lid_bottom above the eye's bottom edge.
    const float r = 1 / std::fmax(eye.lid_curve, min_lid_curve);
    glUniform3f(u_bottom_, eye.x, eye.y + half_height - eye.lid_bottom + r, r);

    // A lid that cuts less than this many pixels isn't drawn at all.
    constexpr float unused = 0.5f;
    const bool top = eye.lid_top > unused || std::fabs(eye.lid_angle) * half_width > unused;
    glUniform2f(u_lids_, top ? 1 : 0, eye.lid_bottom > unused ? 1 : 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace dontfall::face
