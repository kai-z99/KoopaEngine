#pragma once

#include <vector>
#include <utility>
#include "Definitions.h"

namespace VertexBufferSetup
{
    unsigned int SetupTriangleBuffers();
    MeshData SetupSphereBuffers();
    MeshData SetupCubeBuffers();
    MeshData SetupPlaneBuffers();
    unsigned int SetupScreenQuadBuffers();
    unsigned int SetupSkyboxBuffers();
    //return <VAO, texture id>
    std::pair<unsigned int, unsigned int> SetupTerrainBuffers(const char* path);
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