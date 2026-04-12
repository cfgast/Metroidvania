#pragma once

#include <string>
#include <glad/gl.h>
#include <glm/glm.hpp>

// Compiles and links a vertex + fragment shader pair.
// Provides uniform setters for the most common types.
class Shader
{
public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setVec2(const char* name, const glm::vec2& v) const;
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;

    GLuint id() const { return m_program; }

private:
    GLuint m_program = 0;

    static GLuint compile(GLenum type, const std::string& src);
};
