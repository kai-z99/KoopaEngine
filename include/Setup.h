#pragma once

#include <vector>
#include <utility>
#include "Definitions.h"
#include <glm/glm.hpp>

namespace VertexBufferSetup
{
    MeshData SetupTriangleBuffers();
    MeshData SetupSphereBuffers();
    MeshData SetupCubeBuffers();
    MeshData SetupPlaneBuffers();
    MeshData SetupScreenQuadBuffers();
    MeshData SetupSkyboxBuffers();
    //return <VAO, texture id>
    std::pair<MeshData, unsigned int> SetupTerrainBuffers(const char* path);
}

namespace FramebufferSetup
{
    //Note: RBO is lost.
    void SetupHDRFramebuffer(unsigned int& FBO, unsigned int& texture); //HDR buffer
    void SetupMSAAHDRFramebuffer(unsigned int& FBO, unsigned int& texture); //HDR buffer, MSAA
    void SetupHalfResBrightFramebuffer(unsigned int& FBO, unsigned int& texture); //only bright scene, half res
    void SetupTwoPassBlurFramebuffers(unsigned int FBOs[2], unsigned int colorBuffers[2]); //hald res
    void SetupVSMTwoPassBlurFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h); //shadowmap res
    void SetupDirShadowMapFramebuffer(unsigned int& FBO, unsigned int& texture, unsigned int w, unsigned int h);
    void SetupCascadedShadowMapFramebuffer(unsigned int& FBO, unsigned int& textureArray, unsigned int w, unsigned int h, int numCascades);
    void SetupPointShadowMapFramebuffer(unsigned int& FBO, unsigned int w, unsigned int h);
    void SetupGBufferFramebuffer(unsigned int& FBO, unsigned int& gNormal, unsigned int& gPosition);
    void SetupSSAOFramebuffer(unsigned int& FBO, unsigned int& texture);

}

namespace TextureSetup
{
    void SetupPointShadowMapTextureArray(unsigned int& textureArray, unsigned int w, unsigned int h);
    void SetupSSAONoiseTexture(unsigned int& texture, const std::vector<glm::vec3>& noise);
    unsigned int LoadTexture(char const* path);
    unsigned int LoadTextureCubeMap(const std::vector<const char*>& faces);
}