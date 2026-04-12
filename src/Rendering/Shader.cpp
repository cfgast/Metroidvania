#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        std::cerr << "Shader link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader()
{
    if (m_program)
        glDeleteProgram(m_program);
}

void Shader::use() const
{
    glUseProgram(m_program);
}

void Shader::setMat4(const char* name, const glm::mat4& m) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_program, name),
                       1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setVec4(const char* name, const glm::vec4& v) const
{
    glUniform4f(glGetUniformLocation(m_program, name), v.x, v.y, v.z, v.w);
}

void Shader::setVec3(const char* name, const glm::vec3& v) const
{
    glUniform3f(glGetUniformLocation(m_program, name), v.x, v.y, v.z);
}

void Shader::setVec2(const char* name, const glm::vec2& v) const
{
    glUniform2f(glGetUniformLocation(m_program, name), v.x, v.y);
}

void Shader::setFloat(const char* name, float value) const
{
    glUniform1f(glGetUniformLocation(m_program, name), value);
}

void Shader::setInt(const char* name, int value) const
{
    glUniform1i(glGetUniformLocation(m_program, name), value);
}

GLuint Shader::compile(GLenum type, const std::string& src)
{
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        const char* label = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        std::cerr << label << " shader compile error:\n" << log << "\n";
    }
    return shader;
}
