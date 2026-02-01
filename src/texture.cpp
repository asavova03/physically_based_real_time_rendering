#include "texture.h"

#include <algorithm>
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>
#include "../framework/third_party/stb/include/stb/stb_image.h"
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>

Texture::Texture(std::filesystem::path filePath) {
    //Load image from disk to CPU memory.
    //Image class is defined in <framework/image.h>
    Image cpuTexture{filePath};

    // Create a texture on the GPU and bind it for parameter setting
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set behavior for when texture coordinates are outside the [0, 1] range (wrap around).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set interpolation for texture sampling (bilinear interpolation across mip-maps).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Define GPU texture parameters and upload corresponding data based on number of image channels
    switch (cpuTexture.channels) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, cpuTexture.width, cpuTexture.height, 0, GL_RED, GL_UNSIGNED_BYTE,
                         cpuTexture.get_data());
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cpuTexture.width, cpuTexture.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                         cpuTexture.get_data());
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cpuTexture.width, cpuTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         cpuTexture.get_data());
            break;
        default:
            std::cerr << "Number of channels read for texture is not supported" << std::endl;
            throw std::exception();
    }

    // Generate mip-maps
    glGenerateMipmap(GL_TEXTURE_2D);
}

/**
 * Alternative loading of textures when you want to initialize them later.
 * There is the option to also store the map on the CPU side, which is used for height maps for object collision
 * @param path to the texture
 * @param wrapMode such as clamping, repeating, mirroring, etc.
 */
void Texture::loadTexture(const char *path, GLenum wrapMode) {
    int width, height, numChannels;
    stbi_uc *pixels = stbi_load(path, &width, &height, &numChannels, 0);

    m_width = width;
    m_height = height;
    m_channels = numChannels;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format;
    if (numChannels == 1) format = GL_RED;
    else if (numChannels == 3) format = GL_RGB;
    else if (numChannels == 4) format = GL_RGBA;
    else throw std::runtime_error("Unsupported texture format: " + std::string(path));

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (m_keepData)
        m_cpuData.assign(pixels, pixels + width * height * numChannels);

    stbi_image_free(pixels);
}

/**
 * Creates an environment cube map
 * @param faces all six faces of the skybox
 */
void Texture::createCubemap(const std::vector<std::string> &faces) {
    m_target = GL_TEXTURE_CUBE_MAP;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

    int w, h, channels;
    for (GLuint i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &w, &h, &channels, 0);
        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Failed to load cubemap face: " << faces[i] << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

/**
 * Creates an empty color buffer without content because content would be captured later
 * @param format GL_R, GL_RGB, GL_RGBA, etc.
 * @param width specifies the size of the texture
 * @param height
 */
void Texture::createEmptyColorBuffer(GLenum format, int width, int height) {
    m_target = GL_TEXTURE_2D;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0,
                 format, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/**
 * Creates a depth buffer without content because content would be captured later
 * @param width specifies the size of the texture
 * @param height
 */
void Texture::createEmptyDepthBuffer(int width, int height) {
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


/**
 * Creates a solid uniform colored texture.
 * Used for when a PBR model lacks a certain texture, so the same PBR shader can be reused.
 * @param color the color of the texture
 * (for example, if it is a texture for opacity it should be white, while a missing metallic texture should be black).
 */
void Texture::createFallback(int color) {
    m_target = GL_TEXTURE_2D;

    std::vector<unsigned char> data(3, color);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


Texture::Texture(Texture &&other)
    : m_texture(other.m_texture) {
    other.m_texture = INVALID;
}

Texture::~Texture() {
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}

void Texture::bind(GLint textureSlot) const {
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::bindCubemap(GLint textureSlot) const {
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
}

void Texture::keepCPUData(bool enable) { m_keepData = enable; }


float Texture::getValueAt(float u, float v) const {
    if (m_cpuData.empty()) return 0.0f;
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    int x = int(u * (m_width - 1));
    int y = int(v * (m_height - 1));
    int idx = (y * m_width + x) * m_channels;
    return m_cpuData[idx] / 255.0f;
}

