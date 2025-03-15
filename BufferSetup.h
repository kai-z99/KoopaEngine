#pragma once

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
    void SetupFinalImageFramebuffer(unsigned int& FBO, unsigned int& texture);
    void SetupDirShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h);
    void SetupPointShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h);

}