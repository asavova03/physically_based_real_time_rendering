#include "bloom_effect.h"
#include "helpers.h"
#include "imgui/imgui.h"

void Bloom::initBloom(const int width, const int height, GLuint targetFBO) {
    m_width = width;
    m_height = height;
    m_blurShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/bloom_vert.glsl").
                    addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blur_frag.glsl").build();
    m_bloomFinalShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/bloom_vert.glsl").
                   addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/bloom_frag.glsl").build();

    glGenFramebuffers(1, &m_hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

    glGenTextures(2, m_colorBuffers);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, m_colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_colorBuffers[i], 0);
    }

    GLuint attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    glGenRenderbuffers(1, &m_rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rboDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);

    int blurWidth  = width  / m_downsample;
    int blurHeight = height / m_downsample;

    glGenFramebuffers(2, m_pingpongFBO);
    glGenTextures(2, m_pingpongColorBuffers);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_pingpongColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, blurWidth, blurHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpongColorBuffers[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
}

void Bloom::render(GLuint framebuffer) const {
    bool horizontal = true, first_iteration = true;
    m_blurShader.bind();

    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // set smaller viewport
    int blurWidth  = m_width / m_downsample;
    int blurHeight = m_height / m_downsample;
    glViewport(0, 0, blurWidth, blurHeight);

    for (int i = 0; i < m_blurIterations; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingpongFBO[horizontal]);
        glUniform1i(m_blurShader.getUniformLocation("horizontal"), horizontal);
        glUniform1i(m_blurShader.getUniformLocation("radius"), radius / m_downsample);
        glActiveTexture(GL_TEXTURE0);
        if (first_iteration)
            glBindTexture(GL_TEXTURE_2D, m_colorBuffers[1]); // bright color buffer
        else
            glBindTexture(GL_TEXTURE_2D, m_pingpongColorBuffers[!horizontal]);
        glUniform1i(m_blurShader.getUniformLocation("image"), 0);
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]); // restore
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_bloomFinalShader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colorBuffers[0]); // full scene
    glUniform1i(m_bloomFinalShader.getUniformLocation("scene"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pingpongColorBuffers[!horizontal]); // blurred bloom
    glUniform1i(m_bloomFinalShader.getUniformLocation("bloomBlur"), 1);

    glUniform1f(m_bloomFinalShader.getUniformLocation("bloomStrength"),
                m_enableBloom ? m_bloomStrength : 0.0f);

    renderQuad();
}

void Bloom::renderUI() {
    ImGui::Checkbox("Enable Bloom", &m_enableBloom);
    if (m_enableBloom) {
        ImGui::SliderFloat("Bloom Strength", &m_bloomStrength, 0.0f, 3.0f);
        ImGui::SliderInt("Blur Iterations", &m_blurIterations, 1, 30);
        ImGui::SliderInt("Blur Radius", &radius, 5, 80);
    }
}

