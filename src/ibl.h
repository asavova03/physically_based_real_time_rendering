#pragma once
#include "texture.h"
#include "framework/shader.h"

class IBL {
public:
    IBL();

    ~IBL();

    void init(GLuint envCubemap, int captureSize = 512);

    void generateIrradianceMap(int size = 32);

    void generatePrefilterMap(int size = 128);

    void generateBRDFLUT(int size = 512);

    const GLuint &getEnvCubemap() const { return m_envCubemap; }
    const GLuint &getIrradianceMap() const { return m_irradianceMap; }
    const GLuint &getPrefilterMap() const { return m_prefilterMap; }
    const GLuint &getBRDFLUT() const { return m_brdfLUT; }
    const GLuint &getMaxMipLevels() const { return maxMipLevels; }

private:
    GLuint m_envCubemap;
    GLuint m_irradianceMap;
    GLuint m_prefilterMap;
    GLuint m_brdfLUT;

    Shader m_irradianceShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ibl/vertex.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ibl/irradiance_frag.glsl").build();
    Shader m_prefilterShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ibl/vertex.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ibl/prefilter_frag.glsl").build();
    Shader m_bdrfShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/ibl/bdrf_vertex.glsl").
            addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/ibl/bdrf_frag.glsl").build();
    const unsigned int maxMipLevels = 5;
    GLuint m_captureFBO = 0;
    GLuint m_captureRBO = 0;
};
