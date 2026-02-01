#include "framework/shader.h"
#include "glad/glad.h"

/**
 * Handles the bloom post-processing effect (the soft glow around gems).
 * Uses HDR rendering, Gaussian blur, and multiple framebuffers to achieve the effect.
*/
class Bloom {
public:
    // HDR framebuffer where the scene is first rendered
    GLuint m_hdrFBO = 0;

    // Two color attachments: one for the regular scene colors, one for glowing areas (with emission color specific for each gem type)
    GLuint m_colorBuffers[2] = {0, 0};

    // Depth renderbuffer used by the HDR framebuffer
    GLuint m_rboDepth = 0;

    // Ping-pong framebuffers used to apply the blur repeatedly (we load into one and then into the second one and so forth)
    GLuint m_pingpongFBO[2] = {0, 0};

    // Color buffers attached to the ping-pong FBOs for horizontal/vertical blurring
    GLuint m_pingpongColorBuffers[2] = {0, 0};

    // Shader that applies Gaussian blur on the gems
    Shader m_blurShader;

    // Shader that combines the blurred blooming gems with the original scene
    Shader m_bloomFinalShader;

    // Toggles bloom on/off
    bool m_enableBloom = false;
    float m_bloomStrength = 1.5f;

    // How many blur passes to apply, if higher the glow is softer but more computationally expensive
    int m_blurIterations = 15;

    // Downsample factor, smaller textures improve performance because less blurring computations
    int m_downsample = 4.0;

    // Resolution of the bloom buffers
    int m_width = 0.0;
    int m_height = 0.0;

    // Optional radius for the blur kernel (how large is the area affected by the blur)
    int radius = 70;

    // Sets up all the framebuffers, color attachments, and shaders needed for bloom.
    // Should be called once at startup or whenever the screen is resized.
    void initBloom(int width, int height, GLuint targetFBO);

    // Applies the bloom effect after rendering the main scene and composites the final image.
    void render(GLuint framebuffer) const;

    // Renders the UI controls for bloom parameters
    void renderUI();
};
