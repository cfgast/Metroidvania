#include "GLRenderer.h"
#include "../Input/GLFWInput.h"

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <vector>

// ── Shader sources ────────────────────────────────────────────────────────────

// Shared GLSL lighting code injected into both flat and texture fragment shaders
static const char* kLightingGLSL = R"(
#define MAX_LIGHTS 32

struct Light {
    vec2  position;
    vec3  color;
    float intensity;
    float radius;
    float z;
    vec2  direction;
    float innerCone;
    float outerCone;
    int   type;   // 0 = point, 1 = spot
};

uniform Light uLights[MAX_LIGHTS];
uniform int   uNumLights;
uniform vec3  uAmbientColor;

vec3 calcLighting(vec2 worldPos, vec3 normal)
{
    vec3 totalLight = uAmbientColor;

    for (int i = 0; i < uNumLights; ++i)
    {
        vec2 toLight2D = uLights[i].position - worldPos;
        float dist2D   = length(toLight2D);

        if (dist2D >= uLights[i].radius)
            continue;

        // 3D light direction (light is above the 2D plane at height z)
        vec3 lightDir = normalize(vec3(toLight2D, uLights[i].z));

        // Quadratic attenuation fading to zero at radius
        float atten = 1.0 - (dist2D / uLights[i].radius);
        atten = atten * atten;

        // Diffuse (Lambertian)
        float diff = max(dot(normal, lightDir), 0.0);

        // Spot light angular falloff
        float spotFactor = 1.0;
        if (uLights[i].type == 1)
        {
            vec2 spotDir = normalize(uLights[i].direction);
            float cosAngle = dot(normalize(-toLight2D), spotDir);
            spotFactor = smoothstep(uLights[i].outerCone, uLights[i].innerCone, cosAngle);
        }

        totalLight += uLights[i].color * uLights[i].intensity * atten * diff * spotFactor;
    }

    return totalLight;
}
)";

static const char* kFlatVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform mat4 uModel;

out vec2 vWorldPos;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 0.0, 1.0);
    vWorldPos = worldPos.xy;
    gl_Position = uProjection * worldPos;
}
)";

// kFlatFS is built at runtime by prepending kLightingGLSL (see constructor)
static const char* kFlatFS_body = R"(
in vec2 vWorldPos;
out vec4 FragColor;

uniform vec4 uColor;

void main()
{
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 lighting = calcLighting(vWorldPos, normal);
    FragColor = vec4(uColor.rgb * lighting, uColor.a);
}
)";

static const char* kTexVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uProjection;

out vec2 vTexCoord;
out vec2 vWorldPos;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vWorldPos = aPos;
}
)";

// kTexFS is built at runtime by prepending kLightingGLSL
static const char* kTexFS_body = R"(
in vec2 vTexCoord;
in vec2 vWorldPos;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform bool uHasNormalMap;

void main()
{
    vec4 texColor = texture(uTexture, vTexCoord);

    vec3 normal;
    if (uHasNormalMap)
    {
        normal = texture(uNormalMap, vTexCoord).rgb;
        normal = normal * 2.0 - 1.0;
        normal.y = -normal.y;  // flip Y for Y-down coordinate system
        normal = normalize(normal);
    }
    else
    {
        normal = vec3(0.0, 0.0, 1.0);
    }

    vec3 lighting = calcLighting(vWorldPos, normal);
    FragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

// Text shader: samples a single-channel glyph atlas and applies a color uniform
static const char* kTextVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uProjection;

out vec2 vTexCoord;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* kTextFS = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uGlyphAtlas;
uniform vec4 uTextColor;

void main()
{
    float alpha = texture(uGlyphAtlas, vTexCoord).r;
    FragColor = vec4(uTextColor.rgb, uTextColor.a * alpha);
}
)";

// Per-vertex-color shader: each vertex carries its own RGBA color
static const char* kVertColorVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vColor;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* kVertColorFS = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
}
)";

// Fullscreen passthrough blit shader: samples the FBO color attachment
static const char* kBlitVS = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* kBlitFS = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uScreen;

void main()
{
    FragColor = texture(uScreen, vTexCoord);
}
)";

// ── GLFW callbacks ────────────────────────────────────────────────────────────

void GLRenderer::framebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    glViewport(0, 0, w, h);
    GLRenderer* self = static_cast<GLRenderer*>(glfwGetWindowUserPointer(win));
    if (self)
    {
        self->m_windowW = static_cast<float>(w);
        self->m_windowH = static_cast<float>(h);
        if (w > 0 && h > 0)
        {
            self->destroyFBO();
            self->createFBO(w, h);
        }
    }
}

// ── Constructor / Destructor ──────────────────────────────────────────────────

GLRenderer::GLRenderer(const std::string& title, unsigned int width,
                       unsigned int height, unsigned int fpsCap)
    : m_windowW(static_cast<float>(width))
    , m_windowH(static_cast<float>(height))
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(static_cast<int>(width),
                                static_cast<int>(height),
                                title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);

    if (fpsCap > 0)
        glfwSwapInterval(1); // VSync on
    else
        glfwSwapInterval(0);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Build shaders — flat and tex shaders get lighting code prepended
    std::string flatFS = std::string("#version 330 core\n") + kLightingGLSL + kFlatFS_body;
    std::string texFS  = std::string("#version 330 core\n") + kLightingGLSL + kTexFS_body;
    m_flatShader = std::make_unique<Shader>(kFlatVS, flatFS);
    m_texShader  = std::make_unique<Shader>(kTexVS, texFS);
    m_textShader = std::make_unique<Shader>(kTextVS, kTextFS);
    m_vertColorShader = std::make_unique<Shader>(kVertColorVS, kVertColorFS);
    m_blitShader = std::make_unique<Shader>(kBlitVS, kBlitFS);

    // Create persistent GPU buffers
    initQuadVAO();
    initSpriteVAO();
    initTextVAO();
    initDynamicVAO();
    initVertexColorVAO();
    initFullscreenVAO();

    // Create FBO for off-screen rendering
    createFBO(static_cast<int>(width), static_cast<int>(height));

    // Initialize FreeType
    if (FT_Init_FreeType(&m_ftLib))
    {
        std::cerr << "GLRenderer: failed to initialize FreeType\n";
        m_ftLib = nullptr;
    }

    // Default projection: top-left origin
    resetView();

    // Create the GLFW-backed input system (must be after window + user pointer setup)
    m_glfwInput = std::make_unique<GLFWInput>(m_window, *this);
}

GLRenderer::~GLRenderer()
{
    // Destroy input system before the window it depends on
    m_glfwInput.reset();

    destroyFBO();

    if (m_quadVAO) glDeleteVertexArrays(1, &m_quadVAO);
    if (m_quadVBO) glDeleteBuffers(1, &m_quadVBO);
    if (m_spriteVAO) glDeleteVertexArrays(1, &m_spriteVAO);
    if (m_spriteVBO) glDeleteBuffers(1, &m_spriteVBO);
    if (m_textVAO) glDeleteVertexArrays(1, &m_textVAO);
    if (m_textVBO) glDeleteBuffers(1, &m_textVBO);
    if (m_dynVAO) glDeleteVertexArrays(1, &m_dynVAO);
    if (m_dynVBO) glDeleteBuffers(1, &m_dynVBO);
    if (m_vcVAO) glDeleteVertexArrays(1, &m_vcVAO);
    if (m_vcVBO) glDeleteBuffers(1, &m_vcVBO);
    if (m_fsVAO) glDeleteVertexArrays(1, &m_fsVAO);
    if (m_fsVBO) glDeleteBuffers(1, &m_fsVBO);

    for (auto& [handle, info] : m_textures)
    {
        if (info.glId) glDeleteTextures(1, &info.glId);
    }

    for (auto& [path, normalId] : m_normalMaps)
    {
        if (normalId) glDeleteTextures(1, &normalId);
    }

    // Clean up font atlases and FreeType faces
    for (auto& [handle, fontData] : m_fonts)
    {
        for (auto& [size, atlas] : fontData.atlases)
        {
            if (atlas.textureId) glDeleteTextures(1, &atlas.textureId);
        }
        if (fontData.face) FT_Done_Face(fontData.face);
    }

    if (m_ftLib) FT_Done_FreeType(m_ftLib);

    m_flatShader.reset();
    m_texShader.reset();
    m_textShader.reset();
    m_vertColorShader.reset();
    m_blitShader.reset();

    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Quad VAO (unit square 0,0 → 1,1) ─────────────────────────────────────────

void GLRenderer::initQuadVAO()
{
    // Two triangles forming a unit quad
    float verts[] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,

        0.f, 0.f,
        1.f, 1.f,
        0.f, 1.f,
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ── Input ─────────────────────────────────────────────────────────────────────

InputSystem& GLRenderer::getInput()
{
    return *m_glfwInput;
}

InputSystem* GLRenderer::getInputPtr() const
{
    return m_glfwInput.get();
}

// ── Window helpers for GLFWInput callbacks ────────────────────────────────────

void GLRenderer::handleWindowResize(int width, int height)
{
    m_windowW = static_cast<float>(width);
    m_windowH = static_cast<float>(height);
    glViewport(0, 0, width, height);
    if (width > 0 && height > 0)
    {
        destroyFBO();
        createFBO(width, height);
    }
}

void GLRenderer::handleWindowClose()
{
    m_open = false;
}

// ── Window operations ─────────────────────────────────────────────────────────

bool GLRenderer::isOpen() const
{
    return m_open && !glfwWindowShouldClose(m_window);
}

void GLRenderer::close()
{
    m_open = false;
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void GLRenderer::setMouseCursorVisible(bool visible)
{
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

void GLRenderer::setWindowSize(unsigned int w, unsigned int h)
{
    glfwSetWindowSize(m_window, static_cast<int>(w), static_cast<int>(h));
    m_windowW = static_cast<float>(w);
    m_windowH = static_cast<float>(h);
}

void GLRenderer::setWindowPosition(int x, int y)
{
    glfwSetWindowPos(m_window, x, y);
}

void GLRenderer::getDesktopSize(unsigned int& w, unsigned int& h) const
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    w = static_cast<unsigned int>(mode->width);
    h = static_cast<unsigned int>(mode->height);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void GLRenderer::clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::display()
{
    if (m_frameBegan)
        endFrame();
    glfwSwapBuffers(m_window);
}

// ── Frame / lighting pass ─────────────────────────────────────────────────────

void GLRenderer::beginFrame()
{
    m_frameBegan = true;
    m_worldPass  = true;
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_fboWidth, m_fboHeight);
}

void GLRenderer::endFrame()
{
    if (!m_frameBegan)
        return;
    m_frameBegan = false;
    m_worldPass  = false;

    // Blit FBO color attachment to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, static_cast<int>(m_windowW), static_cast<int>(m_windowH));

    m_blitShader->use();
    m_blitShader->setInt("uScreen", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fboColorTex);

    glBindVertexArray(m_fsVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::addLight(const Light& light)
{
    m_lights.push_back(light);
}

void GLRenderer::clearLights()
{
    m_lights.clear();
}

void GLRenderer::setAmbientColor(float r, float g, float b)
{
    m_ambientColor = glm::vec3(r, g, b);
}

// ── Normal map loader ─────────────────────────────────────────────────────────

GLuint GLRenderer::loadNormalMap(const std::string& diffusePath)
{
    auto it = m_normalMaps.find(diffusePath);
    if (it != m_normalMaps.end())
        return it->second;

    // Build the _n.png path: "assets/foo.png" -> "assets/foo_n.png"
    std::string normalPath;
    auto dotPos = diffusePath.rfind('.');
    if (dotPos != std::string::npos)
        normalPath = diffusePath.substr(0, dotPos) + "_n" + diffusePath.substr(dotPos);
    else
        normalPath = diffusePath + "_n";

    stbi_set_flip_vertically_on_load(0);
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(normalPath.c_str(), &w, &h, &channels, 4);

    GLuint texId = 0;
    if (data)
    {
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }

    // Cache result (0 = not found, avoids re-trying)
    m_normalMaps[diffusePath] = texId;
    return texId;
}

// ── Light uniform upload ──────────────────────────────────────────────────────

void GLRenderer::uploadLightUniforms(Shader& shader)
{
    // UI rendering (outside beginFrame/endFrame) gets full brightness, no lights
    if (!m_worldPass)
    {
        shader.setVec3("uAmbientColor", glm::vec3(1.f, 1.f, 1.f));
        shader.setInt("uNumLights", 0);
        return;
    }

    shader.setVec3("uAmbientColor", m_ambientColor);

    int count = static_cast<int>(m_lights.size());
    if (count > 32) count = 32;
    shader.setInt("uNumLights", count);

    char buf[64];
    for (int i = 0; i < count; ++i)
    {
        const Light& L = m_lights[i];

        snprintf(buf, sizeof(buf), "uLights[%d].position", i);
        shader.setVec2(buf, L.position);

        snprintf(buf, sizeof(buf), "uLights[%d].color", i);
        shader.setVec3(buf, L.color);

        snprintf(buf, sizeof(buf), "uLights[%d].intensity", i);
        shader.setFloat(buf, L.intensity);

        snprintf(buf, sizeof(buf), "uLights[%d].radius", i);
        shader.setFloat(buf, L.radius);

        snprintf(buf, sizeof(buf), "uLights[%d].z", i);
        shader.setFloat(buf, L.z);

        snprintf(buf, sizeof(buf), "uLights[%d].direction", i);
        shader.setVec2(buf, L.direction);

        snprintf(buf, sizeof(buf), "uLights[%d].innerCone", i);
        shader.setFloat(buf, L.innerCone);

        snprintf(buf, sizeof(buf), "uLights[%d].outerCone", i);
        shader.setFloat(buf, L.outerCone);

        snprintf(buf, sizeof(buf), "uLights[%d].type", i);
        shader.setInt(buf, static_cast<int>(L.type));
    }
}

// ── FBO creation / destruction ────────────────────────────────────────────────

void GLRenderer::createFBO(int width, int height)
{
    m_fboWidth  = width;
    m_fboHeight = height;

    // Color attachment
    glGenTextures(1, &m_fboColorTex);
    glBindTexture(GL_TEXTURE_2D, m_fboColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Depth/stencil renderbuffer
    glGenRenderbuffers(1, &m_fboDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fboDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Framebuffer object
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_fboColorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_fboDepthRb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "GLRenderer: FBO is not complete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderer::destroyFBO()
{
    if (m_fbo)        { glDeleteFramebuffers(1, &m_fbo);       m_fbo = 0; }
    if (m_fboColorTex){ glDeleteTextures(1, &m_fboColorTex);   m_fboColorTex = 0; }
    if (m_fboDepthRb) { glDeleteRenderbuffers(1, &m_fboDepthRb); m_fboDepthRb = 0; }
}

// ── Fullscreen quad VAO (NDC -1..1, with texcoords) ──────────────────────────

void GLRenderer::initFullscreenVAO()
{
    // Fullscreen triangle-pair in NDC with texcoords
    float verts[] = {
        // pos (x,y)     texcoord (u,v)
        -1.f, -1.f,      0.f, 0.f,
         1.f, -1.f,      1.f, 0.f,
         1.f,  1.f,      1.f, 1.f,

        -1.f, -1.f,      0.f, 0.f,
         1.f,  1.f,      1.f, 1.f,
        -1.f,  1.f,      0.f, 1.f,
    };

    glGenVertexArrays(1, &m_fsVAO);
    glGenBuffers(1, &m_fsVBO);

    glBindVertexArray(m_fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // texcoord (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── View / camera ─────────────────────────────────────────────────────────────

void GLRenderer::setView(float centerX, float centerY,
                         float width, float height)
{
    float halfW = width  * 0.5f;
    float halfH = height * 0.5f;
    // Top-left origin: top = centerY - halfH, bottom = centerY + halfH
    m_projection = glm::ortho(centerX - halfW, centerX + halfW,
                              centerY + halfH, centerY - halfH,
                              -1.f, 1.f);
}

void GLRenderer::resetView()
{
    // Top-left origin (Y-down) for 2D screen-space drawing
    m_projection = glm::ortho(0.f, m_windowW, m_windowH, 0.f, -1.f, 1.f);
}

void GLRenderer::getWindowSize(float& w, float& h) const
{
    w = m_windowW;
    h = m_windowH;
}

// ── Primitive helpers ─────────────────────────────────────────────────────────

void GLRenderer::drawQuad(float x, float y, float w, float h,
                          float r, float g, float b, float a)
{
    m_flatShader->use();
    m_flatShader->setMat4("uProjection", m_projection);

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(x, y, 0.f));
    model = glm::scale(model, glm::vec3(w, h, 1.f));
    m_flatShader->setMat4("uModel", model);

    m_flatShader->setVec4("uColor", glm::vec4(r, g, b, a));

    uploadLightUniforms(*m_flatShader);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ── Primitives ────────────────────────────────────────────────────────────────

void GLRenderer::drawRect(float x, float y, float w, float h,
                          float r, float g, float b, float a)
{
    drawQuad(x, y, w, h, r, g, b, a);
}

void GLRenderer::drawRectOutlined(float x, float y, float w, float h,
                                  float fillR, float fillG, float fillB, float fillA,
                                  float outR, float outG, float outB, float outA,
                                  float outlineThickness)
{
    // Draw the filled rectangle
    drawQuad(x, y, w, h, fillR, fillG, fillB, fillA);

    if (outlineThickness <= 0.f)
        return;

    float t = outlineThickness;

    // Top edge
    drawQuad(x - t, y - t, w + 2 * t, t,
             outR, outG, outB, outA);
    // Bottom edge
    drawQuad(x - t, y + h, w + 2 * t, t,
             outR, outG, outB, outA);
    // Left edge
    drawQuad(x - t, y, t, h,
             outR, outG, outB, outA);
    // Right edge
    drawQuad(x + w, y, t, h,
             outR, outG, outB, outA);
}

// ── Text VAO (dynamic, position + texcoord) ──────────────────────────────────

void GLRenderer::initTextVAO()
{
    glGenVertexArrays(1, &m_textVAO);
    glGenBuffers(1, &m_textVBO);

    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
    // Allocate enough for a batch of quads; re-uploaded each drawText call
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float) * 256, nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // texcoord (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── Glyph atlas builder ──────────────────────────────────────────────────────

GLRenderer::GlyphAtlas& GLRenderer::getOrBuildAtlas(FontData& font, unsigned int size)
{
    auto it = font.atlases.find(size);
    if (it != font.atlases.end())
        return it->second;

    FT_Set_Pixel_Sizes(font.face, 0, size);

    // First pass: measure total atlas dimensions
    int totalWidth = 0;
    int maxHeight = 0;
    for (int c = 32; c < 127; ++c)
    {
        if (FT_Load_Char(font.face, c, FT_LOAD_RENDER))
            continue;
        totalWidth += static_cast<int>(font.face->glyph->bitmap.width) + 1; // 1px padding
        int h = static_cast<int>(font.face->glyph->bitmap.rows);
        if (h > maxHeight) maxHeight = h;
    }

    GlyphAtlas atlas;
    atlas.atlasWidth  = totalWidth;
    atlas.atlasHeight = maxHeight > 0 ? maxHeight : 1;
    atlas.ascender    = static_cast<int>(font.face->size->metrics.ascender >> 6);
    atlas.lineHeight  = static_cast<int>((font.face->size->metrics.ascender -
                                          font.face->size->metrics.descender) >> 6);

    // Create the GL texture
    glGenTextures(1, &atlas.textureId);
    glBindTexture(GL_TEXTURE_2D, atlas.textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas.atlasWidth, atlas.atlasHeight, 0,
                 GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Second pass: rasterize each glyph into the atlas
    int xOffset = 0;
    for (int c = 32; c < 127; ++c)
    {
        if (FT_Load_Char(font.face, c, FT_LOAD_RENDER))
        {
            atlas.glyphs[c] = {};
            continue;
        }

        FT_GlyphSlot g = font.face->glyph;
        int bw = static_cast<int>(g->bitmap.width);
        int bh = static_cast<int>(g->bitmap.rows);

        if (bw > 0 && bh > 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, 0, bw, bh,
                            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        }

        GlyphInfo& gi = atlas.glyphs[c];
        gi.width    = bw;
        gi.height   = bh;
        gi.bearingX = g->bitmap_left;
        gi.bearingY = g->bitmap_top;
        gi.advance  = static_cast<int>(g->advance.x); // in 1/64 pixels
        gi.u0 = static_cast<float>(xOffset) / atlas.atlasWidth;
        gi.v0 = 0.f;
        gi.u1 = static_cast<float>(xOffset + bw) / atlas.atlasWidth;
        gi.v1 = static_cast<float>(bh) / atlas.atlasHeight;

        xOffset += bw + 1;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);

    auto [insertIt, _] = font.atlases.emplace(size, atlas);
    return insertIt->second;
}

// ── Dynamic VAO for circle / rounded-rect geometry (vec2 position) ────────────

void GLRenderer::initDynamicVAO()
{
    glGenVertexArrays(1, &m_dynVAO);
    glGenBuffers(1, &m_dynVBO);

    glBindVertexArray(m_dynVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_dynVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ── Vertex-color VAO (vec2 pos + vec4 color = 6 floats per vertex) ────────────

void GLRenderer::initVertexColorVAO()
{
    glGenVertexArrays(1, &m_vcVAO);
    glGenBuffers(1, &m_vcVBO);

    glBindVertexArray(m_vcVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_vcVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // color (location 1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── Circle ────────────────────────────────────────────────────────────────────

static constexpr int kCircleSegments = 32;
static constexpr float kPi = 3.14159265358979323846f;

void GLRenderer::drawCircle(float cx, float cy, float radius,
                            float r, float g, float b, float a,
                            float outR, float outG, float outB, float outA,
                            float outlineThickness)
{
    // Build triangle-fan vertices: center + (segments+1) rim points
    std::vector<float> verts;
    verts.reserve(2 * (kCircleSegments + 2));

    // Center
    verts.push_back(cx);
    verts.push_back(cy);

    for (int i = 0; i <= kCircleSegments; ++i)
    {
        float angle = 2.f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
        verts.push_back(cx + radius * std::cos(angle));
        verts.push_back(cy + radius * std::sin(angle));
    }

    // Upload and draw the fill
    m_flatShader->use();
    m_flatShader->setMat4("uProjection", m_projection);
    glm::mat4 identity(1.f);
    m_flatShader->setMat4("uModel", identity);
    m_flatShader->setVec4("uColor", glm::vec4(r, g, b, a));

    uploadLightUniforms(*m_flatShader);

    glBindVertexArray(m_dynVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_dynVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(verts.size() / 2));

    // Outline as a triangle-strip ring
    if (outlineThickness > 0.f && outA > 0.f)
    {
        float innerR = radius;
        float outerR = radius + outlineThickness;

        std::vector<float> ring;
        ring.reserve(2 * 2 * (kCircleSegments + 1));

        for (int i = 0; i <= kCircleSegments; ++i)
        {
            float angle = 2.f * kPi * static_cast<float>(i) / static_cast<float>(kCircleSegments);
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            // Inner vertex
            ring.push_back(cx + innerR * cosA);
            ring.push_back(cy + innerR * sinA);
            // Outer vertex
            ring.push_back(cx + outerR * cosA);
            ring.push_back(cy + outerR * sinA);
        }

        m_flatShader->setVec4("uColor", glm::vec4(outR, outG, outB, outA));

        glBindBuffer(GL_ARRAY_BUFFER, m_dynVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(ring.size() * sizeof(float)),
                     ring.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(ring.size() / 2));
    }

    glBindVertexArray(0);
}

// ── Rounded rectangle ─────────────────────────────────────────────────────────

static constexpr int kCornerSegments = 8;

void GLRenderer::drawRoundedRect(float x, float y, float w, float h,
                                 float cornerRadius,
                                 float r, float g, float b, float a,
                                 float outR, float outG, float outB, float outA,
                                 float outlineThickness)
{
    // Clamp corner radius so it doesn't exceed half the rect size
    float maxR = std::min(w * 0.5f, h * 0.5f);
    float cr = std::min(cornerRadius, maxR);
    if (cr < 0.f) cr = 0.f;

    // Center of the rectangle
    float cxr = x + w * 0.5f;
    float cyr = y + h * 0.5f;

    // Build triangle-fan: center + perimeter points around the rounded rect
    // Corner centers (inner corners):
    //   top-left:     (x + cr,     y + cr)
    //   top-right:    (x + w - cr, y + cr)
    //   bottom-right: (x + w - cr, y + h - cr)
    //   bottom-left:  (x + cr,     y + h - cr)
    struct CornerArc {
        float cx, cy;
        float startAngle; // radians
    };

    CornerArc corners[4] = {
        { x + w - cr, y + cr,     -kPi * 0.5f },  // top-right (from -90° to 0°)
        { x + w - cr, y + h - cr,  0.f },           // bottom-right (from 0° to 90°)
        { x + cr,     y + h - cr,  kPi * 0.5f },    // bottom-left (from 90° to 180°)
        { x + cr,     y + cr,      kPi },            // top-left (from 180° to 270°)
    };

    std::vector<float> verts;
    // Center + 4 corners * (segments+1) + 1 to close
    verts.reserve(2 * (1 + 4 * (kCornerSegments + 1) + 1));

    // Center point of fan
    verts.push_back(cxr);
    verts.push_back(cyr);

    for (int c = 0; c < 4; ++c)
    {
        float arcCx = corners[c].cx;
        float arcCy = corners[c].cy;
        float start = corners[c].startAngle;

        for (int i = 0; i <= kCornerSegments; ++i)
        {
            float angle = start + (kPi * 0.5f) * static_cast<float>(i) / static_cast<float>(kCornerSegments);
            verts.push_back(arcCx + cr * std::cos(angle));
            verts.push_back(arcCy + cr * std::sin(angle));
        }
    }

    // Close the fan by repeating the first rim vertex
    verts.push_back(verts[2]);
    verts.push_back(verts[3]);

    // Upload and draw the fill
    m_flatShader->use();
    m_flatShader->setMat4("uProjection", m_projection);
    glm::mat4 identity(1.f);
    m_flatShader->setMat4("uModel", identity);
    m_flatShader->setVec4("uColor", glm::vec4(r, g, b, a));

    uploadLightUniforms(*m_flatShader);

    glBindVertexArray(m_dynVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_dynVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(verts.size() / 2));

    // Outline as a triangle-strip ring
    if (outlineThickness > 0.f && outA > 0.f)
    {
        float ot = outlineThickness;

        // Build inner and outer perimeter points
        // Inner = same perimeter as the fill (skip the center = index 1 onward)
        // Outer = expanded by outlineThickness along the normal direction
        int perimCount = static_cast<int>((verts.size() / 2) - 1); // skip center
        std::vector<float> ring;
        ring.reserve(2 * 2 * perimCount);

        for (int c = 0; c < 4; ++c)
        {
            float arcCx = corners[c].cx;
            float arcCy = corners[c].cy;
            float start = corners[c].startAngle;

            for (int i = 0; i <= kCornerSegments; ++i)
            {
                float angle = start + (kPi * 0.5f) * static_cast<float>(i) / static_cast<float>(kCornerSegments);
                float cosA = std::cos(angle);
                float sinA = std::sin(angle);

                // Inner vertex (on the rect edge)
                ring.push_back(arcCx + cr * cosA);
                ring.push_back(arcCy + cr * sinA);
                // Outer vertex (pushed outward along normal)
                ring.push_back(arcCx + (cr + ot) * cosA);
                ring.push_back(arcCy + (cr + ot) * sinA);
            }
        }

        // Close the ring by repeating the first pair
        ring.push_back(ring[0]);
        ring.push_back(ring[1]);
        ring.push_back(ring[2]);
        ring.push_back(ring[3]);

        m_flatShader->setVec4("uColor", glm::vec4(outR, outG, outB, outA));

        glBindBuffer(GL_ARRAY_BUFFER, m_dynVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(ring.size() * sizeof(float)),
                     ring.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(ring.size() / 2));
    }

    glBindVertexArray(0);
}

// ── Sprite VAO (dynamic, position + texcoord) ────────────────────────────────

void GLRenderer::initSpriteVAO()
{
    glGenVertexArrays(1, &m_spriteVAO);
    glGenBuffers(1, &m_spriteVBO);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    // 6 vertices × 4 floats (x, y, u, v) — re-uploaded each draw call
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    // texcoord (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// ── Magenta fallback texture ──────────────────────────────────────────────────

GLuint GLRenderer::createMagentaTexture()
{
    unsigned char magenta[2 * 2 * 4] = {
        255, 0, 255, 255,   255, 0, 255, 255,
        255, 0, 255, 255,   255, 0, 255, 255,
    };

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, magenta);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

// ── Textured sprites ──────────────────────────────────────────────────────────

Renderer::TextureHandle GLRenderer::loadTexture(const std::string& path)
{
    // Return cached handle if already loaded
    auto it = m_texturePaths.find(path);
    if (it != m_texturePaths.end())
        return it->second;

    TextureHandle handle = m_nextTexHandle++;
    TextureInfo info;

    stbi_set_flip_vertically_on_load(0); // top-left origin, no flip
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &info.width, &info.height,
                                    &channels, 4); // force RGBA

    if (!data)
    {
        std::cerr << "GLRenderer: failed to load texture: " << path << "\n";
        info.glId   = createMagentaTexture();
        info.width  = 2;
        info.height = 2;
    }
    else
    {
        glGenTextures(1, &info.glId);
        glBindTexture(GL_TEXTURE_2D, info.glId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, info.width, info.height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
    }

    m_textures[handle] = info;
    m_texturePaths[path] = handle;
    return handle;
}

void GLRenderer::drawSprite(TextureHandle tex, float x, float y,
                            int frameX, int frameY, int frameW, int frameH,
                            float originX, float originY)
{
    auto it = m_textures.find(tex);
    if (it == m_textures.end())
        return;

    const TextureInfo& info = it->second;
    float texW = static_cast<float>(info.width);
    float texH = static_cast<float>(info.height);

    // Compute UV coordinates from the frame rect
    float u0 = static_cast<float>(frameX) / texW;
    float v0 = static_cast<float>(frameY) / texH;
    float u1 = static_cast<float>(frameX + frameW) / texW;
    float v1 = static_cast<float>(frameY + frameH) / texH;

    // Position quad with origin offset (top-left origin)
    float px = x - originX;
    float py = y - originY;
    float pw = static_cast<float>(frameW);
    float ph = static_cast<float>(frameH);

    // 6 vertices: 2 triangles, each with (x, y, u, v)
    float verts[] = {
        px,      py,      u0, v0,
        px + pw, py,      u1, v0,
        px + pw, py + ph, u1, v1,

        px,      py,      u0, v0,
        px + pw, py + ph, u1, v1,
        px,      py + ph, u0, v1,
    };

    m_texShader->use();
    m_texShader->setMat4("uProjection", m_projection);
    m_texShader->setInt("uTexture", 0);

    uploadLightUniforms(*m_texShader);

    // Look up the normal map for this texture
    GLuint normalMapId = 0;
    {
        // Find the diffuse path from the handle
        for (auto& [path, handle] : m_texturePaths)
        {
            if (handle == tex)
            {
                normalMapId = loadNormalMap(path);
                break;
            }
        }
    }

    m_texShader->setInt("uNormalMap", 1);
    m_texShader->setInt("uHasNormalMap", normalMapId != 0 ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, info.glId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMapId);

    glBindVertexArray(m_spriteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_spriteVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer::FontHandle GLRenderer::loadFont(const std::string& path)
{
    // Return cached handle if already loaded
    auto it = m_fontPaths.find(path);
    if (it != m_fontPaths.end())
        return it->second;

    if (!m_ftLib)
    {
        std::cerr << "GLRenderer: FreeType not initialized, cannot load font: " << path << "\n";
        return 0;
    }

    FontData fontData;
    if (FT_New_Face(m_ftLib, path.c_str(), 0, &fontData.face))
    {
        std::cerr << "GLRenderer: failed to load font: " << path << "\n";
        return 0;
    }

    FontHandle handle = m_nextFontHandle++;
    m_fonts[handle] = std::move(fontData);
    m_fontPaths[path] = handle;
    return handle;
}

void GLRenderer::drawText(FontHandle font, const std::string& str,
                           float x, float y, unsigned int size,
                           float r, float g, float b, float a)
{
    auto fontIt = m_fonts.find(font);
    if (fontIt == m_fonts.end() || str.empty())
        return;

    GlyphAtlas& atlas = getOrBuildAtlas(fontIt->second, size);
    if (!atlas.textureId)
        return;

    // Build vertex data: 6 vertices per character (x, y, u, v)
    std::vector<float> vertices;
    vertices.reserve(str.size() * 6 * 4);

    float cursorX = x;
    float cursorY = y + static_cast<float>(atlas.ascender);

    for (char ch : str)
    {
        if (ch == '\n')
        {
            cursorX = x;
            cursorY += static_cast<float>(atlas.lineHeight);
            continue;
        }

        int c = static_cast<unsigned char>(ch);
        if (c < 32 || c >= 127)
            c = '?';

        const GlyphInfo& gi = atlas.glyphs[c];

        float xpos = cursorX + static_cast<float>(gi.bearingX);
        float ypos = cursorY - static_cast<float>(gi.bearingY);
        float w = static_cast<float>(gi.width);
        float h = static_cast<float>(gi.height);

        if (w > 0 && h > 0)
        {
            // Two triangles per glyph quad
            vertices.insert(vertices.end(), {xpos,     ypos,     gi.u0, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos,     gi.u1, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos + h, gi.u1, gi.v1});

            vertices.insert(vertices.end(), {xpos,     ypos,     gi.u0, gi.v0});
            vertices.insert(vertices.end(), {xpos + w, ypos + h, gi.u1, gi.v1});
            vertices.insert(vertices.end(), {xpos,     ypos + h, gi.u0, gi.v1});
        }

        cursorX += static_cast<float>(gi.advance >> 6);
    }

    if (vertices.empty())
        return;

    m_textShader->use();
    m_textShader->setMat4("uProjection", m_projection);
    m_textShader->setVec4("uTextColor", glm::vec4(r, g, b, a));
    m_textShader->setInt("uGlyphAtlas", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.textureId);

    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);

    GLsizeiptr dataSize = static_cast<GLsizeiptr>(vertices.size() * sizeof(float));
    glBufferData(GL_ARRAY_BUFFER, dataSize, vertices.data(), GL_DYNAMIC_DRAW);

    int vertexCount = static_cast<int>(vertices.size()) / 4;
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::measureText(FontHandle font, const std::string& str,
                              unsigned int size,
                              float& outWidth, float& outHeight)
{
    outWidth = outHeight = 0.f;

    auto fontIt = m_fonts.find(font);
    if (fontIt == m_fonts.end() || str.empty())
        return;

    GlyphAtlas& atlas = getOrBuildAtlas(fontIt->second, size);

    float maxLineWidth = 0.f;
    float curLineWidth = 0.f;
    int lineCount = 1;

    for (char ch : str)
    {
        if (ch == '\n')
        {
            if (curLineWidth > maxLineWidth)
                maxLineWidth = curLineWidth;
            curLineWidth = 0.f;
            ++lineCount;
            continue;
        }

        int c = static_cast<unsigned char>(ch);
        if (c < 32 || c >= 127)
            c = '?';

        curLineWidth += static_cast<float>(atlas.glyphs[c].advance >> 6);
    }

    if (curLineWidth > maxLineWidth)
        maxLineWidth = curLineWidth;

    outWidth  = maxLineWidth;
    outHeight = static_cast<float>(atlas.lineHeight * lineCount);
}

// ── Vertex-colored geometry ───────────────────────────────────────────────────

void GLRenderer::drawTriangleStrip(const std::vector<Vertex>& verts)
{
    if (verts.size() < 3) return;

    m_vertColorShader->use();
    m_vertColorShader->setMat4("uProjection", m_projection);

    glBindVertexArray(m_vcVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_vcVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);
}

void GLRenderer::drawLines(const std::vector<Vertex>& verts)
{
    if (verts.size() < 2) return;

    m_vertColorShader->use();
    m_vertColorShader->setMat4("uProjection", m_projection);

    glBindVertexArray(m_vcVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_vcVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                 verts.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);
}
