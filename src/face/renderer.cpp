#include "renderer.hpp"

#include "eye_shader.hpp"

namespace dontfall::face {

namespace {

// Background black, eyes cyan.
constexpr float eye_color[3] = {0.15f, 0.85f, 1.0f};

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

// gl_FragCoord to logical pixels: logical = (row0 . p, row1 . p) + offset.
// gl_FragCoord has its origin bottom left, logical coordinates top left.
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

void set_eye(GLint location, const Eye &eye)
{
    glUniform4f(location, eye.x, eye.y, eye.width / 2, eye.height / 2);
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
    a_pos_ = glGetAttribLocation(program_, "a_pos");
    u_left_ = glGetUniformLocation(program_, "u_left");
    u_right_ = glGetUniformLocation(program_, "u_right");
    u_radius_ = glGetUniformLocation(program_, "u_radius");
    // These never change while running.
    glUniform2fv(glGetUniformLocation(program_, "u_row0"), 1, t.row0);
    glUniform2fv(glGetUniformLocation(program_, "u_row1"), 1, t.row1);
    glUniform2fv(glGetUniformLocation(program_, "u_offset"), 1, t.offset);
    glUniform3fv(glGetUniformLocation(program_, "u_color"), 1, eye_color);

    // Two triangles covering the screen, as a triangle strip.
    const GLfloat corners[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    glGenBuffers(1, &quad_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_);
    glBufferData(GL_ARRAY_BUFFER, sizeof corners, corners, GL_STATIC_DRAW);
    glVertexAttribPointer(a_pos_, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(a_pos_);

    glViewport(0, 0, panel_width, panel_height);
    return true;
}

void Renderer::draw(const Face &face)
{
    set_eye(u_left_, face.left);
    set_eye(u_right_, face.right);
    glUniform2f(u_radius_, face.left.radius, face.right.radius);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace dontfall::face
