#pragma once

#include <vector>

namespace VertexBufferSetup
{
    unsigned int SetupTriangleBuffers();
    unsigned int SetupSphereBuffers();
    unsigned int SetupCubeBuffers();
    unsigned int SetupPlaneBuffers();
    unsigned int SetupScreenQuadBuffers();
    unsigned int SetupSkyboxBuffers();
}

namespace FramebufferSetup
{
    //Note: RBO is lost.
    void SetupHDRFramebuffer(unsigned int& FBO, unsigned int colorBuffers[2]); //HDR buffer
    void SetupTwoPassBlurFramebuffers(unsigned int FBOs[2], unsigned int colorBuffers[2]);
    void SetupHalfResBrightFramebuffer(unsigned int& FBO, unsigned int texture);
    void SetupDirShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h);
    void SetupCascadedShadowMapFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h, int numCascades);
    void SetupPointShadowMapFramebuffer(unsigned int& FBO);
}

namespace TextureSetup
{
    void SetupPointShadowMapTextureArray(unsigned int& textureArray, unsigned int w, unsigned int h);
    unsigned int LoadTexture(char const* path);
    unsigned int LoadTextureCubeMap(const std::vector<const char*>& faces);
}