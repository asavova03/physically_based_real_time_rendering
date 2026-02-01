#include "ibl.h"
#include <glad/glad.h>
#include <math.h>

#include "glm/fwd.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "helpers.h"


IBL::IBL() {
}

IBL::~IBL() {
    glDeleteFramebuffers(1, &m_captureFBO);
    glDeleteRenderbuffers(1, &m_captureRBO);
    glDeleteTextures(1, &m_envCubemap);
    glDeleteTextures(1, &m_irradianceMap);
    glDeleteTextures(1, &m_prefilterMap);
    glDeleteTextures(1, &m_brdfLUT);
}

/**
 * Initializes an Image Based Lighting class holding all textures necessary for image-based lighting
 * This is done in 'init' and not in the constructor to allow late initialization
 * @param cubemap the environment skybox texture
 * @param cubemapSize the size of the environment texture
 */
void IBL::init(GLuint cubemap, int cubemapSize) {
    glGenFramebuffers(1, &m_captureFBO);
    glGenRenderbuffers(1, &m_captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRBO);

    m_envCubemap = cubemap;
}

void IBL::generateIrradianceMap(int size) {
    // create cubemap
    glGenTextures(1, &m_irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     size, size, 0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // FBO setup
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);

    // draw to color attachment 0 explicitly
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // bind shader and uniforms
    m_irradianceShader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
    glUniform1i(m_irradianceShader.getUniformLocation("environmentMap"), 0);
    glUniformMatrix4fv(m_irradianceShader.getUniformLocation("projection"), 1, GL_FALSE,
                       glm::value_ptr(captureProjection));

    glViewport(0, 0, size, size);
    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(m_irradianceShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               m_irradianceMap, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void IBL::generatePrefilterMap(int size) {
    // create cubemap with mip levels
    glGenTextures(1, &m_prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);

    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = std::max(1u, (unsigned int) (size * std::pow(0.5f, mip)));
        unsigned int mipHeight = mipWidth;
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip, GL_RGB16F, mipWidth, mipHeight, 0, GL_RGB, GL_FLOAT,
                         nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    m_prefilterShader.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubemap);
    glUniform1i(m_prefilterShader.getUniformLocation("skybox"), 0);
    glUniformMatrix4fv(m_prefilterShader.getUniformLocation("projection"), 1, GL_FALSE,
                       glm::value_ptr(captureProjection));

    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = std::max(1u, (unsigned int) (size * std::pow(0.5f, mip)));
        unsigned int mipHeight = mipWidth;

        // resize RBO to match the size of the mip
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);

        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float) mip / (float) (maxMipLevels - 1);
        glUniform1f(m_prefilterShader.getUniformLocation("roughness"), roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            // attach each face
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   m_prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUniformMatrix4fv(m_prefilterShader.getUniformLocation("view"), 1, GL_FALSE,
                               glm::value_ptr(captureViews[i]));
            renderCube();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_prefilterMap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void IBL::generateBRDFLUT(int size) {
    // create 2D texture
    glGenTextures(1, &m_brdfLUT);
    glBindTexture(GL_TEXTURE_2D, m_brdfLUT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_brdfLUT, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glViewport(0, 0, size, size);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    m_bdrfShader.bind();
    glClear(GL_COLOR_BUFFER_BIT);

    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
