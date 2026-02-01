#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <exception>
#include <filesystem>
#include <vector>
#include <string>
#include <framework/opengl_includes.h>

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Texture {
public:
    Texture() : m_texture(INVALID) {
    }

    Texture(std::filesystem::path filePath);

    Texture(const Texture &) = delete;

    Texture(Texture &&);

    ~Texture();

    Texture &operator=(const Texture &) = delete;

    Texture &operator=(Texture &&) = default;

    void bind(GLint textureSlot) const;

    void bindCubemap(GLint textureSlot = 0) const;

    void loadTexture(const char *path, GLenum wrapMode = GL_REPEAT);

    void createEmptyColorBuffer(GLenum format, int width, int height);

    void createEmptyDepthBuffer(int width, int height);

    void createFallback(int color = 0);

    void createCubemap(const std::vector<std::string> &faces);

    void keepCPUData(bool enable);

    float getValueAt(float u, float v) const;

    GLuint id() const { return m_texture; }

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture{INVALID};
    GLenum m_target{GL_TEXTURE_2D};
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
    std::vector<unsigned char> m_cpuData;
    bool m_keepData = false;
};
