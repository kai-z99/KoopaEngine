#pragma once

#include <vector>

namespace VertexBufferSetup
{
    unsigned int SetupTriangleBuffers();
    unsigned int SetupCubeBuffers();
    unsigned int SetupPlaneBuffers();
    unsigned int SetupScreenQuadBuffers();
    unsigned int SetupSkyboxBuffers();
}

namespace FramebufferSetup
{
    //Note: RBO is lost.
    void SetupHDRFramebuffer(unsigned int& FBO, unsigned int& texture); //HDR buffer
    void SetupDirShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h);
    void SetupPointShadowMapFramebuffer(unsigned int& FBO);
}

namespace TextureSetup
{
    void SetupPointShadowMapTexture(unsigned int& texture, unsigned int w, unsigned int h);
    unsigned int LoadTexture(char const* path);
    unsigned int LoadTextureCubeMap(const std::vector<const char*>& faces);
}