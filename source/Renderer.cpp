//#include "pch.h"
#include "../include/Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <GLFW/glfw3.h>

#include "../include/shaderSources.h"
#include "../include/Shader.h"
#include "../include/ComputeShader.h"
#include "../include/DrawCall.h"
#include "../include/Setup.h"
#include "../include/Camera.h"
#include "../include/Model.h"
#include "../include/ParticleEmitter.h"

#include <iostream>
#include <random>

//temp
static inline float ourLerp(float a, float b, float f)
{
    return a + f * (b - a);
}

Renderer::Renderer()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE); 
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    //initial setup   
    this->drawCalls = {};                                             
    this->cascadeLevels = { DEFAULT_FAR / 35.0f, DEFAULT_FAR / 15.0f, DEFAULT_FAR / 6.0f, DEFAULT_FAR / 2.0f };
    this->cascadeMultipliers = {12.0f, 10.0f, 4.0f, 2.0f, 1.2f}; //minecraft{12.0f, 10.0f, 4.0f, 2.0f, 1.2f}
    assert(cascadeLevels.size() == cascadeMultipliers.size() - 1);

    this->usingSkybox = false;
    this->clearColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    this->fogColor = glm::vec3(0.0f); //disabled
    this->expFogDensity = 0.15f;
    this->linearFogStart = 70.0f;
    this->fogType = EXPONENTIAL_SQUARED;
    this->msaa = true;
    
    // shaders
    this->InitializeShaders();

    //NOTE: DO THIS AFTER THE POINT LIGHT VECTOR IS INITIALIZED... facepalm
    this->SetupFramebuffers();
    this->SetupVertexBuffers();

   
    //LIGHTING-------------------------------------------------
    //Initialialize lighting
    this->bloomThreshold = 1.0f;
    this->ambientLighting = 0.015f;
    this->InitializeDirLight();
    this->currentFramePointLightCount = 0;
    this->currentFrameShadowArrayIndex = 0;

    //Since these texture units are exclusivley for these wont change, we can just set them once
    //here in the constructor.
    //point
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, this->pointShadowMapTextureArrayRG);
    //cascade
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->cascadeShadowMapTextureArrayDepth);
    //ssao
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, this->ssaoBlurTextureR);

    glActiveTexture(GL_TEXTURE0);

    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    printf("OpenGL version: %d.%d\n", major, minor);
    printf("%s\n", glGetString(GL_VERSION));

    if (this->usingPBR)
    {
        this->SetupEnvironmentCubeMap();
        this->SetupIBLMaps();
    }
    
}

void Renderer::InitializeShaders()
{
    //SHADERS-------------------------------------------------
    this->currentMaterial = Material();
    this->ResetMaterial();

    //Main lighting shader
    this->lightingShader = new Shader(ShaderSources::vs1, ShaderSources::fs1);
    this->lightingShader->use();
    //ASSOCIATE TEXTURES----
    if (!usingPBR)
    {
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "usingPBR"), 0);             
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "material.diffuse"), 0);      //GL_TEXTURE0
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "material.normal"), 1);       //GL_TEXTURE1
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "material.specular"), 2);     //GL_TEXTURE2
        //bind there no matter what since otherwise they will take sampler 0, causing conflict. (theyre cubemaps insteead of 2d)
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "irradianceMap"), 7);         //GL_TEXTURE7
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "prefilterMap"), 8);         //GL_TEXTURE8
    }
    else
    {
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "usingPBR"), 1);
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.albedo"), 0);      //GL_TEXTURE0
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.normal"), 1);       //GL_TEXTURE1
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.metallic"), 2);     //GL_TEXTURE2
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.roughness"), 5);     //GL_TEXTURE5
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.ao"), 6);           //GL_TEXTURE6
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "irradianceMap"), 7);         //GL_TEXTURE7
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "prefilterMap"), 8);         //GL_TEXTURE8
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "brdfLUT"), 11);         //GL_TEXTURE11
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "PBRmaterial.height"), 12);         //GL_TEXTURE12
    }
    
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "pointShadowMapArray"), 3);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "cascadeShadowMaps"), 4);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "ssao"), 10);                

    //Stuff for cascade shadows
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "cascadeCount"), (unsigned int)this->cascadeLevels.size()); //3 (4 matrices)
    for (int i = 0; i < this->cascadeLevels.size(); i++)
    {
        std::string l = "cascadeDistances[" + std::to_string(i) + "]";
        glUniform1f(glGetUniformLocation(this->lightingShader->ID, l.c_str()), this->cascadeLevels[i]);
    }

    //Debug lighting shader
    this->debugLightShader = new Shader(ShaderSources::vs1, ShaderSources::fsLight);
    this->drawDebugLights = false;

    //Final quad shader
    this->screenShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsScreenQuad);
    this->screenShader->use();
    glUniform1i(glGetUniformLocation(this->screenShader->ID, "hdrBuffer"), 0); //GL_TEXTIRE0
    glUniform1i(glGetUniformLocation(this->screenShader->ID, "blurBuffer"), 1); //GL_TEXTIRE1
    glUniform1f(glGetUniformLocation(this->screenShader->ID, "exposure"), DEFAULT_EXPOSURE); //set exposure

    //extract bright parts from hdrScene
    this->brightShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsBright);
    glUniform1i(glGetUniformLocation(this->screenShader->ID, "hdrScene"), 0); //GL_TEXTIRE0

    //2 pass blur shader
    this->blurShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsBlur);
    this->blurShader->use();
    glUniform1i(glGetUniformLocation(this->blurShader->ID, "scene"), 0); //GL_TEXTIRE0
    //horizontal is set in BlurBrightScene()

    //Dir shadow shader
    this->dirShadowShader = new Shader(ShaderSources::vsDirShadow, ShaderSources::fsDirShadow);
    //Cascade shadow shader
    this->cascadeShadowShader = new Shader(ShaderSources::vsCascadedShadow, ShaderSources::fsCascadedShadow);
    //point shadow shader
    this->pointShadowShader = new Shader(ShaderSources::vsPointShadow, ShaderSources::fsPointShadow);

    //skybox shader
    this->skyShader = new Shader(ShaderSources::vsSkybox, ShaderSources::fsSkybox);
    glUniform1i(glGetUniformLocation(this->skyShader->ID, "skyboxTexture"), 0); //GL_TEXTURE0

    //tesselation for heightmap
    this->terrainShader = new Shader(ShaderSources::vsTerrain, ShaderSources::fs1, nullptr,
        ShaderSources::tcsTerrain, ShaderSources::tesTerrain);
    this->terrainShader->use();
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "heightMap"), 9);             //GL_TEXTURE9
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "material.diffuse"), 0);      //GL_TEXTURE0
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "material.normal"), 1);       //GL_TEXTURE1
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "material.specular"), 2);     //GL_TEXTURE2
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "pointShadowMapArray"), 3);
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "cascadeShadowMaps"), 4);
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "ssao"), 10);
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "irradianceMap"), 7);         //GL_TEXTURE7
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "prefilterMap"), 8);         //GL_TEXTURE8


    //Stuff for cascade shadows
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "cascadeCount"), (unsigned int)this->cascadeLevels.size()); //4 (5 matrices)
    for (int i = 0; i < this->cascadeLevels.size(); i++)
    {
        std::string l = "cascadeDistances[" + std::to_string(i) + "]";
        glUniform1f(glGetUniformLocation(this->terrainShader->ID, l.c_str()), this->cascadeLevels[i]);
    }

    //gBuffer
    this->geometryPassShader = new Shader(ShaderSources::vsGeometryPass, ShaderSources::fsGeometryPass);

    //SSAAO shader
    this->SetupSSAOData();
    this->ssaoShader = new Shader(ShaderSources::vsSSAO, ShaderSources::fsSSAO);
    this->ssaoShader->use();
    glUniform1i(glGetUniformLocation(this->ssaoShader->ID, "gNormal"), 0);              //GL_TEXTURE0
    glUniform1i(glGetUniformLocation(this->ssaoShader->ID, "gPosition"), 1);            //GL_TEXTURE1
    glUniform1i(glGetUniformLocation(this->ssaoShader->ID, "ssaoNoiseTexture"), 2);     //GL_TEXTURE2
    glUniform1f(glGetUniformLocation(this->ssaoShader->ID, "screenWidth"), SCREEN_WIDTH);
    glUniform1f(glGetUniformLocation(this->ssaoShader->ID, "screenHeight"), SCREEN_HEIGHT);

    for (unsigned int i = 0; i < 32; ++i) //send sample kernels
    {
        std::string s = "samples[" + std::to_string(i) + "]";
        const char* cs = s.c_str();
        glUniform3fv(glGetUniformLocation(this->ssaoShader->ID, cs), 1, glm::value_ptr(ssaoKernel[i]));
    }

    //ssao blur
    this->ssaoBlurShader = new Shader(ShaderSources::vsSSAO, ShaderSources::fsSSAOBlur);
    glUniform1i(glGetUniformLocation(this->ssaoShader->ID, "ssaoTexture"), 0);            //GL_TEXTURE0

    //vsm blur
    this->vsmPointBlurShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsVSMPointBlur);
    glUniform1i(glGetUniformLocation(this->vsmPointBlurShader->ID, "source"), 0);            //GL_TEXTURE0

    this->particleUpdateComputeShader = new ComputeShader(ShaderSources::csParticle);
    this->particleShader = new Shader(ShaderSources::vsParticle, ShaderSources::fsParticle);
                    
    this->tileCullShader = new ComputeShader(ShaderSources::csTileCulling);

    this->equiToCubeShader = new Shader(ShaderSources::vsCube, ShaderSources::fsEquirectangularToCubemap);

    this->irradianceShader = new Shader(ShaderSources::vsCube, ShaderSources::fsIrradiance);

    this->prefilterShader = new Shader(ShaderSources::vsCube, ShaderSources::fsPreFilter);
    
    this->brdfShader = new Shader(ShaderSources::vsBRDF, ShaderSources::fsBRDF);

}

void Renderer::SetupFramebuffers()
{
    FramebufferSetup::SetupHDRFramebuffer(this->hdrFBO, this->hdrTextureRGBA);
    FramebufferSetup::SetupMSAAHDRFramebuffer(this->hdrMSAAFBO, this->hdrMSAATextureRGBA);
    FramebufferSetup::SetupHalfResBrightFramebuffer(this->halfResBrightFBO, this->halfResBrightTextureRGBA);
    FramebufferSetup::SetupTwoPassBlurFramebuffers(this->twoPassBlurFBOs, this->twoPassBlurTexturesRGBA);
    FramebufferSetup::SetupVSMTwoPassBlurFramebuffer(this->vsmBlurFBO[0], this->vsmBlurTextureArrayRG[0], this->P_SHADOW_WIDTH, this->P_SHADOW_HEIGHT);
    FramebufferSetup::SetupVSMTwoPassBlurFramebuffer(this->vsmBlurFBO[1], this->vsmBlurTextureArrayRG[1], this->P_SHADOW_WIDTH, this->P_SHADOW_HEIGHT);

    /*
    FramebufferSetup::SetupDirShadowMapFramebuffer(this->dirShadowMapFBO, this->dirShadowMapTextureDepth,
        this->D_SHADOW_WIDTH, this->D_SHADOW_HEIGHT);
    */

    FramebufferSetup::SetupCascadedShadowMapFramebuffer(this->cascadeShadowMapFBO, this->cascadeShadowMapTextureArrayDepth,
        this->CASCADE_SHADOW_WIDTH, this->CASCADE_SHADOW_HEIGHT, (int)this->cascadeLevels.size() + 1);

    //Point shadows
    FramebufferSetup::SetupPointShadowMapFramebuffer(this->pointShadowMapFBO, this->P_SHADOW_WIDTH, this->P_SHADOW_HEIGHT);
    TextureSetup::SetupPointShadowMapTextureArray(this->pointShadowMapTextureArrayRG, P_SHADOW_WIDTH, P_SHADOW_HEIGHT);

    //SSAO
    FramebufferSetup::SetupGBufferFramebuffer(this->gBufferFBO, this->gNormalTextureRGBA, this->gPositionTextureRGBA);
    FramebufferSetup::SetupSSAOFramebuffer(this->ssaoFBO, this->ssaoQuadTextureR);
    FramebufferSetup::SetupSSAOFramebuffer(this->ssaoBlurFBO, this->ssaoBlurTextureR);
    TextureSetup::SetupSSAONoiseTexture(this->ssaoNoiseTexture, this->ssaoNoise);

    FramebufferSetup::SetupTiledSSBOs(this->lightSSBO, this->countSSBO, this->indexSSBO);

}

void Renderer::SetupSSAOData()
{
    //TEMP
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 32; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, //(-1, 1) x
            randomFloats(generator) * 2.0 - 1.0, //(-1, 1) y
            randomFloats(generator)              //( 0, 1) z (hemisphere)
        );
        sample = glm::normalize(sample); //direction
        sample *= randomFloats(generator); //magnitude

        //bias more near center of hemisphere
        float scale = float(i) / 32.0f;
        scale = ourLerp(0.1f, 1.0f, scale * scale);
        sample *= scale;

        this->ssaoKernel.push_back(sample); //TANGENT SPACE
    }

    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise =
        {
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f
        };

        this->ssaoNoise.push_back(noise);
    }
}

void Renderer::SetupEnvironmentCubeMap()
{

    //IBL setup
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf("../ShareLib/Resources/lakeside_night_4k.hdr", &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }
    stbi_set_flip_vertically_on_load(false);

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    glGenTextures(1, &this->environmentCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->environmentCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //dot artifacts
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    this->equiToCubeShader->use();
    this->equiToCubeShader->setInt("equirectangularMap", 0);
    glUniformMatrix4fv(glGetUniformLocation(this->equiToCubeShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(this->equiToCubeShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, this->environmentCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(this->skyboxMeshData.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //make sure to gen mip maps for dot artifacts
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->environmentCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

}

void Renderer::SetupIBLMaps()
{
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &this->irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);


    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    this->irradianceShader->use();
    this->irradianceShader->setInt("environmentMap", 0);
    glUniformMatrix4fv(glGetUniformLocation(this->irradianceShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->environmentCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glUniformMatrix4fv(glGetUniformLocation(this->irradianceShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, this->irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(this->skyboxMeshData.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //ATP; this->irradianceMap is filled with the correct irradiance for every N.
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradianceMap);
    glActiveTexture(GL_TEXTURE0);


    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &this->preFilterMapRGB);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->preFilterMapRGB);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter cubemap.
   // ----------------------------------------------------------------------------------------------------
    this->prefilterShader->use();
    this->prefilterShader->setInt("environmentMap", 0);
    glUniformMatrix4fv(glGetUniformLocation(this->prefilterShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->environmentCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // resize framebuffer according to mip-level size.
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        this->prefilterShader->setFloat("roughness", roughness);

        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(glGetUniformLocation(this->prefilterShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, this->preFilterMapRGB, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(this->skyboxMeshData.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    glGenTextures(1, &this->brdfLUTRG);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, this->brdfLUTRG);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->brdfLUTRG, 0);

    glViewport(0, 0, 512, 512);
    this->brdfShader->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(this->screenQuadMeshData.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->preFilterMapRGB);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, this->brdfLUTRG);
    glActiveTexture(GL_TEXTURE0);

}

void Renderer::SetupVertexBuffers()
{
    this->triangleMeshData = VertexBufferSetup::SetupTriangleBuffers();
    this->cubeMeshData = VertexBufferSetup::SetupCubeBuffers();
    this->planeMeshData = VertexBufferSetup::SetupPlaneBuffers();
    this->sphereMeshData = VertexBufferSetup::SetupSphereBuffers();
    this->screenQuadMeshData = VertexBufferSetup::SetupScreenQuadBuffers();
    this->skyboxMeshData = VertexBufferSetup::SetupSkyboxBuffers();
}

void Renderer::InitializeDirLight()
{
    DirLight d = DirLight();
    this->SendDirLightUniforms();
}

Renderer::~Renderer()
{
    //VBO/VAO
    glDeleteVertexArrays(1, &this->triangleMeshData.VAO);
    glDeleteVertexArrays(1, &this->screenQuadMeshData.VAO);

    //delete VBOs? reference is lost right now.

    delete this->lightingShader;
    delete this->debugLightShader;
    delete this->screenShader;

    for (DrawCall* d : this->drawCalls) delete d;
}

void Renderer::BeginRenderFrame()
{
    //glBindFramebuffer(GL_FRAMEBUFFER, this->hdrFBO); //off screen render
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::EndRenderFrame()
{
    //RENDER SHADOW MAPS---
    this->RenderShadowMaps();

    //Render the ssao Texture
    this->RenderSSAO();

    this->DoTileCulling();

    glEnable(GL_BLEND);

    //Render the main scene into hdrMSAATexture, then blit that to hdrTexture
    this->RenderMainScene();

    //draw debug lights into hdrFBO is applicable
    this->DrawLightsDebug();
    
    glDisable(GL_BLEND);
    //Draw skybox last (using z = w optimization)
    this->DrawSkybox();

    //DO BLOOM STUFF---
    this->BlurBrightScene();
    //DRAW QUAD ONTO SCREEN---
    //Draw the final scene on a full screen quad.
    this->DrawFinalQuad();

    //CLEANUP---
    //reset lights for the next frame
    this->SetAndSendAllLightsToFalse(); //uniforms are sent here too.
    this->drawCalls.clear(); //raylib style

    this->CleanUpParticles();

}

void Renderer::RenderMainScene()
{
    //DRAW INTO FINAL IMAGE---
    //bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, this->hdrMSAAFBO);

    //clear main scene with current color, clear bright scene with black always.
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    //Note: binding vsmtexture is expected in renderdoc (only horiz blur) but uses empty in realtime (OG)
    //      binding pointshadowmaptexture is expected in renderdoc (2 tap blur) but uses OG texture in realtime.
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, this->pointShadowMapTextureArrayRG);
    //glActiveTexture(GL_TEXTURE7);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradianceMap);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_DEPTH_TEST);
    //render
    for (DrawCall* d : this->drawCalls)
    {
        //Frustum culling
        AABB worldAABB = d->GetWorldAABB();
        if (!this->IsAABBVisible(worldAABB, this->cameraFrustumPlanes))
        {
            continue;
        }

        //Draw with shader. (model matrix is sent here)
        if (d->GetHeightMapPath() == nullptr) //not drawing terrain
        {
            this->lightingShader->use();
            if (!this->usingPBR) d->BindMaterialUniforms(this->lightingShader); //set the material unique to each draw call
            else d->BindPBRMaterialUniforms(this->lightingShader);

            d->Render(this->lightingShader);
        }
        else
        {
            this->terrainShader->use();
            glActiveTexture(GL_TEXTURE9); // Activate unit 9, the heightmap
            glBindTexture(GL_TEXTURE_2D, this->pathToTerrainMeshDataAndTextureID[d->GetHeightMapPath()].second); // Bind the stored heightmap ID
            d->BindMaterialUniforms(this->terrainShader); //set the material unique to each draw call
            d->Render(this->terrainShader);
        }
        glActiveTexture(GL_TEXTURE0);
    }

    //PARTICLE
    this->particleUpdateComputeShader->use();
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    float dt = float(now - lastTime);
    lastTime = now;

    for (ParticleEmitter* p : this->particleEmitters)
    {
        p->Update(this->particleUpdateComputeShader, dt);
        p->Render(this->particleShader);
    }

    std::cout << "Size: " << this->particleEmitters.size() << '\n';

    //blit msaa hdr texture to normal hdr texture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->hdrMSAAFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->hdrFBO);
    glBlitFramebuffer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,   // src rect
                      0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,   // dst rect
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);                         // average samples

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderShadowMaps()
{
    //dir
    if (this->dirLight.castShadows) this->RenderCascadedShadowMap(); //Sets active FB to the shadow one in function

    //point
    
    for (unsigned int i = 0; i < currentFramePointLightCount; i++)
    {
        if (this->pointLights[i].shadowMapIndex != -1)
        {
            this->RenderPointShadowMap(i);
        }
    }
    
}

void Renderer::RenderSSAO()
{
    //GBUFFER-------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, this->gBufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    this->geometryPassShader->use();

    for (DrawCall* d : this->drawCalls)
    {
        d->Render(this->geometryPassShader);
    }
    
    //SSAO-------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, this->ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //viewport set

    this->ssaoShader->use();

    //Textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->gNormalTextureRGBA);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->gPositionTextureRGBA);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->ssaoNoiseTexture);

    glBindVertexArray(this->screenQuadMeshData.VAO); //whole screen
    glDrawArrays(GL_TRIANGLES, 0, 6); //drwa quad

    //BLUR------
    glBindFramebuffer(GL_FRAMEBUFFER, this->ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->ssaoBlurShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->ssaoQuadTextureR); //blurring

    //whole screen
    glBindVertexArray(this->screenQuadMeshData.VAO); //whole screen
    glDrawArrays(GL_TRIANGLES, 0, 6); //drwa quad

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::DrawFinalQuad()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST); //will be drawing directly in front screen
    this->screenShader->use();
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glBindVertexArray(this->screenQuadMeshData.VAO); //whole screen
    glActiveTexture(GL_TEXTURE0); //0
    glBindTexture(GL_TEXTURE_2D, this->hdrTextureRGBA);
    glActiveTexture(GL_TEXTURE1); //1: blurBuffer in shader
    glBindTexture(GL_TEXTURE_2D, this->twoPassBlurTexturesRGBA[1]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::CleanUpParticles()
{
    std::vector<ParticleEmitter*> temp;

    for (ParticleEmitter* p : this->particleEmitters)
    {
        if (!p->DoneEmitting())
        {
            temp.push_back(p);
        }
        else
        {
            delete p;
        }
    }

    this->particleEmitters = temp;
}

void Renderer::DoTileCulling()
{
    uint32_t nx = (SCREEN_WIDTH + TILE_SIZE - 1) / TILE_SIZE;
    uint32_t ny = (SCREEN_HEIGHT + TILE_SIZE - 1) / TILE_SIZE;
    uint32_t nTiles = nx * ny;

    glClearBufferSubData(countSSBO, GL_R32UI, 0, nTiles * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glClearBufferSubData(indexSSBO, GL_R32UI, 0, nTiles * MAX_LIGHTS_PER_TILE * sizeof(uint32_t), GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

    size_t n = std::min(currentFramePointLightCount, MAX_POINT_LIGHTS);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(PointLightGPU) * n, this->pointLights); //no need to clear, we loop with numLights

    this->tileCullShader->use();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightSSBO); //binding = 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, indexSSBO); //binding = 2
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, countSSBO); //binding = 3

    glDispatchCompute(nx,ny, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Renderer::BlurBrightScene()
{
    //EXTRACT BRIGHT BRIGHT SCENE INTO HALFRES------------------------------------------
    //bind fb
    glBindFramebuffer(GL_FRAMEBUFFER, this->halfResBrightFBO);
    glViewport(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST); //will be drawing directly in front screen

    this->brightShader->use();
    
    //Bind texture
    glActiveTexture(GL_TEXTURE0);   //hdrScene
    glBindTexture(GL_TEXTURE_2D, this->hdrTextureRGBA);

    //draw quad
    glBindVertexArray(this->screenQuadMeshData.VAO); //whole screen
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    //NOW BLUR THAT BRIGHT SCENE---------------------------------------------------------
    blurShader->use();

    constexpr unsigned int amount = BLUR_PASSES * 2; //* 2 since its a two pass blur
    bool horizontal = true;
    bool first = true;
    for (unsigned int i = 0; i < amount; i++)
    {
        glUniform1i(glGetUniformLocation(blurShader->ID, "horizontal"), horizontal);
        //Bind the correct FBO
        glBindFramebuffer(GL_FRAMEBUFFER, this->twoPassBlurFBOs[horizontal]);
        glViewport(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        //Bind textures
        glActiveTexture(GL_TEXTURE0);  
        glBindTexture(GL_TEXTURE_2D, first ? this->halfResBrightTextureRGBA : this->twoPassBlurTexturesRGBA[!horizontal]);
        
        glBindVertexArray(this->screenQuadMeshData.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
        if (first) first = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::GetFrustumPlanes(const glm::mat4& vp, glm::vec4* frustumPlanes)
{
    //REMEMBER: M * v for (dim = 4) = vec4(dot(r0,v), dot(r1,v), dot(r2,v), dot(r3,v))

    //THEREFORE: P_clip = vp * P_world
    //           x_clip = dot(r_0, P_world) 
    //           y_clip = dot(r_1, P_world)
    //           z_clip = dot(r_2, P_world)
    //           w_clip = dot(r_3, P_world)

    //REMEMBER:
    //          -w_clip <= x_clip <= w_clip
    //          -w_clip <= y_clip <= w_clip
    //          -w_clip <= z_clip <= w_clip
    // (Since after perspective divide, anything out that range will be out of [-1,1] therefore
    //      outside the frustum.)

    //REMEMBER: Ax + By + Cz + D = 0 === dot((A,B,C,D), (x,y,z,1)) = 0
    // 
    //          A point (x,y,z) is on the side of the normal if Ax + By + Cz + D >= 0
    //          
    //      (1) A point (x,y,z) is on the side of the normal if dot((A,B,C,D), (x,y,z,1)) >= 0    

    //FORM THE PLANES
    //
    //Example: Right Plane ----------------------------
    //
    // x_clip <= w_clip  WHEN x_clip is in the right frustum (2)
    // w_clip - x_clip >= 0                                       
    // dot(r_3, P_world) - dot(r_0, P_world) >= 0              
    // dot(r_3 - r_0, P_world) >= 0  WHEN x_clip is in frustum
    // 
    // According to (1):              
    // The point P_world.xyz is on the positive side of the (world space) plane 
    // defined by coefficients (r_3 - r_0) === (A,B,C,D)
    // if dot((r_3 - r_0), P_world) >= 0 
    // Since this condition is equivalent to the original clip-space condition for being
    // inside the right boundary, this plane MUST be the
    // *Right Frustum Plane* in WORLD space.
    // 
    // Therefore we let (r_3 - r_0) be our right frustum plane. 
    //
    //Example: Left Plane -----------------------------
    //
    // -w_clip <= x_clip
    //  ...
    // dot(r_3 + r_0, P_world) >= 0
    // Since we have (1)
    // (r_3 + r_0) = (A,B,C,D) for left plane
    //
    // So On... ---------------------------------------
    //
    // Top: r_3 - r_1
    // Bot: r_3 + r_1
    // Near: r_3 + r_2
    // Far: r_3 - r_2
                    
    //REMEMBER: mat4[col][row] 
    //EG: vec4 colX = mat4[0]
    //    float colYrow3 = mat4[1][3]
    
    //                               r3.x + r0.x          r3.y + r0.y          r3.z + r0.z          r3.w + r0.w
    //Left:(r_3+r_0) = (A,B,C,D)   A: (r3 + r0).x       B: (r3 + r0).y       C: (r3 + r0).z       D: (r3 + r0).w
    frustumPlanes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
    //Right:(r_3-r_0)= (A,B,C,D)   A: (r3 - r0).x       B: (r3 - r0).y       C: (r3 - r0).z       D: (r3 - r0).w
    frustumPlanes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
    // Bottom: r_3+r_1
    frustumPlanes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
    // Top: r_3-r_1
    frustumPlanes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
    // Near: r_3+r_2
    frustumPlanes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
    // Far: r_3-r_2
    frustumPlanes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

    // Normalize the planes
    for (int i = 0; i < 6; i++) 
    {
        float mag = glm::length(glm::vec3(frustumPlanes[i])); // Length of normal (a,b,c)
        if (mag > std::numeric_limits<float>::epsilon()) {
            frustumPlanes[i] /= mag;
        }
    }
}

bool Renderer::IsAABBVisible(const AABB& worldAABB, glm::vec4* frustumPlanes)
{
    //Vec3 -> glm::vec3
    glm::vec3 minBounds = glm::vec3(worldAABB.min.x, worldAABB.min.y, worldAABB.min.z);
    glm::vec3 maxBounds = glm::vec3(worldAABB.max.x, worldAABB.max.y, worldAABB.max.z);

    //get center of aabb
    glm::vec3 center = glm::vec3(maxBounds + minBounds) * 0.5f;
    //the extents represent how far the AABB stretches from the center in each direction.
    //like half the size of the box in each direction.
    //NOTE: the direction of this vector is usually similar. (its always in the same octant)
    glm::vec3 extents = maxBounds - center;

    //go through each plane (a,b,c,d)
    for (int i = 0; i < 6; i++)
    {
        const glm::vec4& plane = frustumPlanes[i];
        glm::vec3 normal = glm::vec3(plane); //(a,b,c) = normal (pointing inwards of the frustum, normalized)
        float distance = plane.w;            //d = how far the plane is shifted from the origin.
        //Therefore a plane that crosses the origin has d = 0.

        // Projected radius tells us how thick the AABB is along the normal of the plane.
        // Note: it yields 0 if extends = 0 aka if AABB is a point. Refer to (1)
        // 
        // dot(extents,|normal|) is just the scalar projection of the extents vector onto |normal|
        // The scalar projection represents the contribution of each axis's extent in the direction
        // of the plane normal. It's a scalar value representing how far the box extends along the normal,
        // just what we need. 
        // 
        // Note that since normal is normalized, the result of the dot product is 
        // in the same length units as the extents.
        float projectedRadius = glm::dot(extents, glm::abs(normal));

        //How far is the center of the AABB is from the plane (sign is relative to normal).
        //NOTE: distance_p = a·x + b·y + c·z + d. Where p = (x,y,z)
        //      IF NORMAL is NORMALIZED:
        //      distance_p > 0 means the point is on the same side as where the normal is pointing
        //      distance_p < 0 means the point is on the opposite side
        //      distance_p = 0 means point is on plane.

        // boxDistance = a*center.x + b*center.y + c*center.z + distance (d)
        float boxDistance = glm::dot(normal, center) + distance;

        //checks whether the entire AABB is completely on the “negative” side of the plane.
        //Note: (1) if the AABB was a point , the check would just be if boxDistance is negative.
        //      But we must take into account that an AABB is not just a point and might poke into the plane.
        if (boxDistance + projectedRadius < 0) return false;
    }

    return true; //is in all planes
}

void Renderer::DrawSkybox()
{
    if (this->usingSkybox)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, this->hdrFBO);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        this->skyShader->use();
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        //draw
        glBindVertexArray(this->skyboxMeshData.VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->currentSkyboxTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        //clean
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDepthFunc(GL_LESS);
    }
    glDisable(GL_DEPTH_TEST);
}

std::vector<glm::vec4> Renderer::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    glm::mat4 inverse = glm::inverse(proj * view);

    std::vector<glm::vec4> corners;
    
    for (int x = -1; x <= 1; x += 2)
    {
        for (int y = -1; y <= 1; y += 2)
        {
            for (int z = -1; z <= 1; z += 2)
            {
                glm::vec4 corner = inverse * glm::vec4(x,y,z, 1.0f); 
                corners.push_back(corner / corner.w); //perspective divide
            }
        }
    }

    return corners;
}

glm::mat4 Renderer::CalculateLightSpaceCascadeMatrix(float near, float far, int index)
{
    glm::mat4 proj = glm::perspective(glm::radians(cam->zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, near, far);
    glm::mat4 view = cam->GetViewMatrix();
    std::vector<glm::vec4> corners = this->GetFrustumCornersWorldSpace(proj, view);

    glm::vec3 center = glm::vec3(0.0f);

    //Add all positions of the corners then divide to get center of cam frustum in world space.
    for (const auto& c : corners)
    {
        center += glm::vec3(c); //vec4 -> vec3
    }
    center /= corners.size();

    //View
    glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };

    //Note: (0,-1,0) x (0,1,0) = 0, edge case inside lookat()
    constexpr float threshold = 1.0f - std::numeric_limits<float>::epsilon() * 100; //~0.9999

    //check if lightDir and worldUp are almost paralell or antiparralell
    if (std::abs(glm::dot(glm::normalize(this->dirLight.direction), worldUp)) > threshold)
    {
        worldUp = { 0.0f, 0.0f, 1.0f };
    }

    //subtract the negative of the lightDir from the center, moving the vector along -lightDir. (by 1 unit)
    glm::mat4 lightView = glm::lookAt(center - glm::normalize(this->dirLight.direction), center, worldUp);
    
    //Proj
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& c : corners)
    {
        glm::vec4 cLightSpace = lightView * c;
        minX = std::min(minX, cLightSpace.x);
        maxX = std::max(maxX, cLightSpace.x);
        minY = std::min(minY, cLightSpace.y);
        maxY = std::max(maxY, cLightSpace.y);
        minZ = std::min(minZ, cLightSpace.z);
        maxZ = std::max(maxZ, cLightSpace.z);
    }

    //'#include "pch.h"'

    //Pull in near plane, push out far plane because things can be outside view frustum but still cast shadows.
    float zMult = this->cascadeMultipliers[index];
    minZ = (minZ < 0) ? minZ * zMult : minZ / zMult;
    maxZ = (maxZ < 0) ? maxZ / zMult : maxZ * zMult;
        
    glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    return lightProjection * lightView;
}

std::vector<glm::mat4> Renderer::GetCascadeMatrices()
{
    std::vector<glm::mat4> ret;

    for (int i = 0; i < this->cascadeLevels.size() + 1; i++)
    {
        if (i == 0)
        {
            ret.push_back(CalculateLightSpaceCascadeMatrix(DEFAULT_NEAR, this->cascadeLevels[i], i));
        }
        else if (i < this->cascadeLevels.size())
        {
            ret.push_back(CalculateLightSpaceCascadeMatrix(this->cascadeLevels[i - 1], this->cascadeLevels[i], i));
        }
        else
        {
            ret.push_back(CalculateLightSpaceCascadeMatrix(this->cascadeLevels[i - 1], DEFAULT_FAR, i));
        }
    }

    return ret;
}

/*
The RenderDoc data strongly supports the hypothesis that the dominant reason the nearest cascade
is significantly slower (1178 µs) than the farther cascade (128 µs) for a high-vertex model is 
due to the increased pixel processing load during rasterization and ROP (Raster Output Pipeline) operations.

The VS Invocations (119964) and Rasterized Primitives (39988) are identical for both draw calls. 
This confirms the same geometry is being processed by the vertex shader and sent to the rasterizer in both cases. 
Therefore, the difference in performance is not due to vertex shading or triangle setup overhead differences between the cascades.
The  difference lies in "Samples Passed". The closer cascade resulted in 127,532 samples passed, 
while the farther cascade only had 39154 samples pass.

"Samples Passed" directly measures the number of pixels on the shadow map that were covered by 
the model and required a depth value to be written. This is because the model's larger projection onto the near cascade's 
shadow map forced the GPU to determine coverage, test depth, and write depth values for significantly more
pixels compared to its smaller projection on the farther cascade's map.
*/
void Renderer::RenderCascadedShadowMap()
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->cascadeShadowMapFBO); //texture array is attached
    glViewport(0, 0, CASCADE_SHADOW_WIDTH, CASCADE_SHADOW_HEIGHT);
    this->cascadeShadowShader->use();

    std::vector<glm::mat4> lightSpaceMatrices = this->GetCascadeMatrices();
                
    for (unsigned int i = 0; i < lightSpaceMatrices.size(); i++)
    {   
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->cascadeShadowMapTextureArrayDepth, 0, i);
        glClear(GL_DEPTH_BUFFER_BIT);

        std::string l = "cascadeLightSpaceMatrices[" + std::to_string(i) + "]";

        //send lightspace matrix to lighting shader's array for later use
        this->lightingShader->use();
        glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, l.c_str()), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));

        //give it to terrain shader too
        this->terrainShader->use();
        glUniformMatrix4fv(glGetUniformLocation(this->terrainShader->ID, l.c_str()), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));

        //send lightspace matrix to cascade vertex shader for current use
        this->cascadeShadowShader->use();
        glUniformMatrix4fv(glGetUniformLocation(this->cascadeShadowShader->ID, "lightSpaceMatrix"), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));
       
        //static int count = 0;
        //if (count % 60 == 0) std::cout << "Draw calls culled in cascade mapping: " << count << '\n';

        glm::vec4 frustumPlanes[6];
        //note: model matrix is sent in d->Render()
        //glCullFace(GL_FRONT);
        for (DrawCall* d : this->drawCalls)
        {
            AABB worldAABB = d->GetWorldAABB();
            this->GetFrustumPlanes(lightSpaceMatrices[i], frustumPlanes);

            if (FRUSTUM_CULLING && !this->IsAABBVisible(worldAABB, frustumPlanes))
            {
                continue;
            }

            d->RenderLOD(this->cascadeShadowShader, true);
        }
        glCullFace(GL_BACK);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::RenderPointShadowMap(unsigned int index)
{
    glDisable(GL_CULL_FACE);
    this->pointShadowShader->use();

    //create shadow proj matrix base
    float aspect = (float)P_SHADOW_WIDTH / (float)P_SHADOW_HEIGHT;
    float near = SHADOW_PROJECTION_NEAR;
    float far = SHADOW_PROJECTION_FAR;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
                                    
    //Set shadow transforms
    this->shadowTransforms.clear();
    glm::vec3 lightPos = glm::vec3(this->pointLights[index].positionRange);   //view

    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0),  glm::vec3(0.0, -1.0, 0.0)));
    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0),  glm::vec3(0.0,  0.0, 1.0)));
    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0,  0.0,-1.0)));
    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0),  glm::vec3(0.0, -1.0, 0.0)));
    this->shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

    //bind framebuffer and viewport
    glBindFramebuffer(GL_FRAMEBUFFER, this->pointShadowMapFBO); //write to the shadowMap
    glViewport(0, 0, this->P_SHADOW_WIDTH, this->P_SHADOW_HEIGHT); //make sure the window rectangle is the shadowmap size
    
    //send some uniforms
    this->lightingShader->use();
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "pointShadowProjFarPlane"), far);
    this->terrainShader->use();
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "pointShadowProjFarPlane"), far);
    this->pointShadowShader->use();
    glUniform1f(glGetUniformLocation(this->pointShadowShader->ID, "pointShadowProjFarPlane"), far);
    glUniform3fv(glGetUniformLocation(this->pointShadowShader->ID, "lightPos"), 1, glm::value_ptr(lightPos));

    glm::vec4 shadowProjFrustumPlanes[6];

    //Render each face of the cubemap
    for (int i = 0; i < 6; i++)
    {
        GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        //set output texture (the thing being poured into) (render target)
        //set the face from the cube texture array
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->pointShadowMapTextureArrayRG, 0, index * 6 + i);
        glUniformMatrix4fv(glGetUniformLocation(this->pointShadowShader->ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));

        //clear the currently bound attachment's depth buffer
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f); //r = d g = d^2
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT); 
        glClearColor(this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.a);

        this->GetFrustumPlanes(shadowTransforms[i], shadowProjFrustumPlanes);

        static int count = 0;
        //if (count % 60 == 0 && count != 0) std::cout << "Draw calls culled in point shadow mapping: " << count << '\n';

        //render
        //glCullFace(GL_FRONT);      // <<< CULL the _front_ faces
        for (DrawCall* d : this->drawCalls)
        {
            AABB worldAABB = d->GetWorldAABB();
            if (FRUSTUM_CULLING && !this->IsAABBVisible(worldAABB, shadowProjFrustumPlanes) )
            {
                count++;
                continue; //object is not in that side of the cubemap, dont bother with it.
            }

            //d->Render(this->pointShadowShader, true);
            d->RenderLOD(this->pointShadowShader, true);
        }
        //glCullFace(GL_BACK);       // restore
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    T1 = this->pointShadowMapTextureArrayRG;
    glEnable(GL_CULL_FACE);
    this->BlurPointShadowMap(index);
}

void Renderer::BlurPointShadowMap(unsigned int index)
{
    glDisable(GL_DEPTH_TEST);

    const GLuint ping[2] = { vsmBlurTextureArrayRG[0], vsmBlurTextureArrayRG[1] };
    const GLuint fbo[2] = { vsmBlurFBO[0]         , vsmBlurFBO[1] };

    GLuint src = pointShadowMapTextureArrayRG;   // read raw moments first
    int dstIdx = 0;
    //glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowMapTextureArrayRG);
    //glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
    vsmPointBlurShader->use();
    glActiveTexture(GL_TEXTURE0);
    glViewport(0, 0, P_SHADOW_WIDTH, P_SHADOW_HEIGHT);
                    
    for (int pass = 0; pass < 2; ++pass)
    {
        bool horizontal = (pass == 0);
        glUniform1i(glGetUniformLocation(this->vsmPointBlurShader->ID, "horizontal"), horizontal);

        // write
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[dstIdx]);

        for (int face = 0; face < 6; ++face)
        {
            int layer = 6 * index + face;

            //based on layer and texture
            glFramebufferTextureLayer(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                ping[dstIdx], 0, layer);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glUniform1i(glGetUniformLocation(this->vsmPointBlurShader->ID, "layer"), layer);

            // read
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, src); //to texture0

            glBindVertexArray(this->screenQuadMeshData.VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT |
        //    GL_FRAMEBUFFER_BARRIER_BIT);

        src = ping[dstIdx];   // next pass samples what we just wrote
        dstIdx ^= 1;              // swap buffers
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, ping[dstIdx ^ 1]); // final result
    this->pointShadowMapTextureArrayRG = ping[1];
    //glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowMapTextureArrayRG);
    //glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::ClearScreen(Vec4 col)
{
    glClearColor(col.r, col.g, col.b, col.a); //This sets what glClear will clear as.
    this->drawCalls.clear();
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //fill it with that clear color.
    //note: All draw calls draw onto this buffer so each glClearColor will be updated.
}

void Renderer::AddToTextureMap(const char* path)
{
    auto it = this->textureToID.find(path);

    if (it == this->textureToID.end())
    {
        unsigned int textureID = TextureSetup::LoadTexture(path);
        this->textureToID[path] = textureID;
    }
}

void Renderer::ResetMaterial()
{
    this->currentMaterial.baseColor = noTexturePink;
    this->currentMaterial.diffuse = -1;
    this->currentMaterial.normal = -1;
    this->currentMaterial.specular = -1;
    this->currentMaterial.baseSpecular = 1.0f;
    this->currentMaterial.useDiffuseMap = false;
    this->currentMaterial.useNormalMap = false;
    this->currentMaterial.useSpecularMap = false;
    this->currentMaterial.hasAlpha = false;
}

void Renderer::SetCurrentDiffuse(const char* path)
{
    AddToTextureMap(path);
    this->currentMaterial.diffuse = this->textureToID[path];
    this->currentMaterial.useDiffuseMap = true;
                    
    GLint bits;
    glGetTextureLevelParameteriv(this->currentMaterial.diffuse, 0, GL_TEXTURE_ALPHA_SIZE, &bits);
    this->currentMaterial.hasAlpha = bits > 0;
}

void Renderer::SetCurrentBaseColor(Vec3 col)
{
    this->currentMaterial.baseColor = col;
}

void Renderer::SetCurrentNormal(const char* path)
{
    AddToTextureMap(path);
    this->currentMaterial.normal = this->textureToID[path];
    this->currentMaterial.useNormalMap = true;
}

void Renderer::SetCurrentSpecular(const char* path)
{
    AddToTextureMap(path);
    this->currentMaterial.specular = this->textureToID[path];
    this->currentMaterial.useSpecularMap = true;
}

void Renderer::SetCurrentPBRMaterial(const char* albedo, const char* normal, const char* height, const char* metallic, const char* roughness, const char* ao)
{
    AddToTextureMap(albedo);
    AddToTextureMap(normal);
    AddToTextureMap(height);
    AddToTextureMap(metallic);
    AddToTextureMap(roughness);
    AddToTextureMap(ao);

    this->currentPBRMaterial.albedo = this->textureToID[albedo];
    this->currentPBRMaterial.normal = this->textureToID[normal];
    this->currentPBRMaterial.height = this->textureToID[height];
    this->currentPBRMaterial.metallic = this->textureToID[metallic];
    this->currentPBRMaterial.roughness = this->textureToID[roughness];
    this->currentPBRMaterial.ao = this->textureToID[ao];
}

void Renderer::SetBaseSpecular(float spec)
{
    this->currentMaterial.baseSpecular = spec;
}

void Renderer::SetSkybox(const std::vector<const char*>& faces)
{
    this->currentSkyboxTexture = TextureSetup::LoadTextureCubeMap(faces);
    this->usingSkybox = true;

    if (usingPBR) this->currentSkyboxTexture = this->environmentCubemap;
}

void Renderer::SetExposure(float exposure)
{
    this->screenShader->use();
    glUniform1f(glGetUniformLocation(this->screenShader->ID, "exposure"), exposure);
}

void Renderer::SetFogType(FogType fog)
{
    this->fogType = fog;
}

void Renderer::SetFogColor(Vec3 col)
{
    this->fogColor = glm::vec3(col.r, col.g, col.b);
}

void Renderer::SetExpFogDensity(float density)
{
    this->expFogDensity = density;
}

void Renderer::SetLinearFogStart(float start)
{
    this->linearFogStart = start;
}

void Renderer::SetAmbientLighting(float ambient)
{
    this->ambientLighting = ambient;
}

void Renderer::SetBloomThreshold(float threshold)
{
    this->bloomThreshold = threshold;
}

static glm::mat4 CreateModelMatrix(const Vec3& pos, const Vec4& rotation, const Vec3& scale)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(pos.x, pos.y, pos.z));
    model = glm::rotate(model, glm::radians(rotation.w), glm::vec3(rotation.x, rotation.y, rotation.z));
    model = glm::scale(model, glm::vec3(scale.x, scale.y, scale.z));
    return model;
}

void Renderer::DrawTriangle(Vec3 pos, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, {1,1,1});

    if (!this->usingPBR) this->drawCalls.push_back(new DrawCall(this->triangleMeshData, this->currentMaterial, model));
    else this->drawCalls.push_back(new DrawCall(this->triangleMeshData, this->currentPBRMaterial, model));
}

void Renderer::DrawCube(Vec3 pos, Vec3 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, size);
    if (!this->usingPBR) this->drawCalls.push_back(new DrawCall(this->cubeMeshData, this->currentMaterial, model));
    else this->drawCalls.push_back(new DrawCall(this->cubeMeshData, this->currentPBRMaterial, model));
}

void Renderer::DrawPlane(Vec3 pos, Vec2 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, Vec3(size.x, 1.0f, size.y));
    if (!this->usingPBR) this->drawCalls.push_back(new DrawCall(this->planeMeshData, this->currentMaterial, model));
    else this->drawCalls.push_back(new DrawCall(this->planeMeshData, this->currentPBRMaterial, model));
    this->drawCalls.back()->SetCulling(false); //Cannot cull flat things like plane
}

void Renderer::DrawSphere(Vec3 pos, Vec3 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, size);
    if (!this->usingPBR) this->drawCalls.push_back(new DrawCall(this->sphereMeshData, this->currentMaterial, model));
    else this->drawCalls.push_back(new DrawCall(this->sphereMeshData, this->currentPBRMaterial, model));
}

void Renderer::DrawModel(const char* path, bool flipTexture, Vec3 pos, Vec3 size, Vec4 rotation)
{
    if (flipTexture) stbi_set_flip_vertically_on_load(true);

    auto it = this->pathToModel.find(path);

    if (it == this->pathToModel.end())
    {
        this->pathToModel[path] = new Model(path);
    }
    stbi_set_flip_vertically_on_load(false);

    glm::mat4 model = CreateModelMatrix(pos, rotation, size);

    //for every mesh in the model, create a drawcall
    for (ModelMesh& mesh : this->pathToModel[path]->meshes)
    {
        DrawCall* drawCall = new DrawCall(mesh.GetMeshData(), mesh.GetMaterial(), model);
        if (mesh.lodMeshData.has_value()) drawCall->SetLODMesh(*mesh.lodMeshData);

        this->drawCalls.push_back(drawCall);
    }
}

void Renderer::DrawTerrain(const char* path, Vec3 pos, Vec3 size, Vec4 rotation)
{
    auto it = this->pathToTerrainMeshDataAndTextureID.find(path);

    if (it == this->pathToTerrainMeshDataAndTextureID.end())
    {
        this->pathToTerrainMeshDataAndTextureID[path] = VertexBufferSetup::SetupTerrainBuffers(path);
    }

    glm::mat4 model = CreateModelMatrix(pos, rotation, size);                       //MeshData
    this->drawCalls.push_back(new DrawCall(this->pathToTerrainMeshDataAndTextureID[path].first, 
        this->currentMaterial, model, GL_PATCHES));

    this->drawCalls.back()->SetHeightMapPath(path);
}

void Renderer::CreateParticleEmitter(double duration, unsigned int count, Vec3 pos, Vec3 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, size );
    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    float dt = float(now - lastTime);
    lastTime = now;
    
    this->particleEmitters.push_back(new ParticleEmitter(model, count, duration));
}

void Renderer::DrawLightsDebug()
{
    glBindFramebuffer(GL_FRAMEBUFFER, this->hdrFBO);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (this->drawDebugLights)
    {
        
        this->debugLightShader->use();
        glBindVertexArray(this->cubeMeshData.VAO);

        for (int i = 0; i < MAX_POINT_LIGHTS; i++)
        {
            PointLightGPU curr = this->pointLights[i];
            if (!curr.isActive) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(curr.positionRange.x, curr.positionRange.y, curr.positionRange.z));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
            glUniformMatrix4fv(glGetUniformLocation(this->debugLightShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(debugLightShader->ID, "lightColor"), 1, glm::value_ptr(glm::vec3(curr.colorIntensity.r, curr.colorIntensity.g, curr.colorIntensity.b)));
            glUniform1f(glGetUniformLocation(debugLightShader->ID, "intensity"), curr.colorIntensity.w);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::AddPointLightToFrame(Vec3 pos, Vec3 col, float range, float intensity, bool shadow)
{
    if (this->currentFramePointLightCount + 1 <= MAX_POINT_LIGHTS)
    {
        PointLightGPU p = PointLightGPU();

        p.isActive = true;
        p.positionRange = { pos.x, pos.y, pos.z, std::min(range, 100.0f) };
        p.colorIntensity = { col.r, col.g, col.b, intensity };

        if (shadow) p.shadowMapIndex = this->currentFrameShadowArrayIndex++;
        else p.shadowMapIndex = -1;
        assert(this->currentFrameShadowArrayIndex <= MAX_SHADOW_CASTING_POINT_LIGHTS);
        
        this->pointLights[this->currentFramePointLightCount] = p;
        this->currentFramePointLightCount++;

        this->tileCullShader->use();
        glUniform1i(glGetUniformLocation(this->tileCullShader->ID, "numLights"), this->currentFramePointLightCount);
        this->lightingShader->use();
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "numPointLights"), this->currentFramePointLightCount);
        this->terrainShader->use();
        glUniform1i(glGetUniformLocation(this->terrainShader->ID, "numPointLights"), this->currentFramePointLightCount);
    }
    else
    {
        std::cout << "ERROR: Max pointlights exceeded\n";
    }


}

void Renderer::AddDirLightToFrame(Vec3 dir, Vec3 col, float intensity, bool shadow)
{
    //If its the first time it would be false. This will "activate" the light.
    this->dirLight.isActive = true;

    this->dirLight.direction = { dir.x, dir.y, dir.z };
    this->dirLight.color = { col.r, col.g, col.b };
    this->dirLight.intensity = intensity;
    this->dirLight.castShadows = shadow;

    this->SendDirLightUniforms();
}

void Renderer::SetAndSendAllLightsToFalse()
{
    this->dirLight.isActive = false;
    this->SendDirLightUniforms();

    this->lightingShader->use();
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "numPointLights"), 0);
    this->terrainShader->use();
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "numPointLights"), 0);
    this->tileCullShader->use();
    glUniform1i(glGetUniformLocation(this->tileCullShader->ID, "numLights"), 0);
    this->currentFramePointLightCount = 0;
    this->currentFrameShadowArrayIndex = 0;
}

void Renderer::SendDirLightUniforms()
{
    this->lightingShader->use();

    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "dirLight.direction"), 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "dirLight.color"), 1, glm::value_ptr(dirLight.color));
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "dirLight.intensity"), dirLight.intensity);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirLight.isActive"), dirLight.isActive);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirLight.shadowMapIndex"), dirLight.castShadows);

    this->terrainShader->use();

    glUniform3fv(glGetUniformLocation(this->terrainShader->ID, "dirLight.direction"), 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(glGetUniformLocation(this->terrainShader->ID, "dirLight.color"), 1, glm::value_ptr(dirLight.color));
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "dirLight.intensity"), dirLight.intensity);
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "dirLight.isActive"), dirLight.isActive);
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "dirLight.shadowMapIndex"), dirLight.castShadows);
}

void Renderer::SendCameraUniforms(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position)
{
    //update camerea frustum planes since we have access to the camera here
    this->GetFrustumPlanes(projection * view, this->cameraFrustumPlanes);

    this->lightingShader->use();
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "viewPos"), 1, glm::value_ptr(position));
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "farPlane"), DEFAULT_FAR);
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "nearPlane"), DEFAULT_NEAR);

    if (this->drawDebugLights)
    {
        debugLightShader->use();
        glUniformMatrix4fv(glGetUniformLocation(debugLightShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(debugLightShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    }
     
    if (this->usingSkybox)
    {
        this->skyShader->use();

        glm::mat4 noTransView = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(noTransView));
        glUniformMatrix4fv(glGetUniformLocation(skyShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    }

    this->terrainShader->use();
    glUniformMatrix4fv(glGetUniformLocation(this->terrainShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(this->terrainShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(this->terrainShader->ID, "viewPos"), 1, glm::value_ptr(position));
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "farPlane"), DEFAULT_FAR);
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "nearPlane"), DEFAULT_NEAR);

    this->geometryPassShader->use();
    glUniformMatrix4fv(glGetUniformLocation(geometryPassShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(geometryPassShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    this->ssaoShader->use();
    glUniformMatrix4fv(glGetUniformLocation(ssaoShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    this->particleShader->use();
    glUniformMatrix4fv(glGetUniformLocation(particleShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(particleShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //temp
    this->tileCullShader->use();
    glUniformMatrix4fv(glGetUniformLocation(tileCullShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(tileCullShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform2f(glGetUniformLocation(tileCullShader->ID, "screen"), SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform1f(glGetUniformLocation(tileCullShader->ID, "farPlane"), DEFAULT_FAR);

}

void Renderer::SendOtherUniforms()
{
    this->lightingShader->use();
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "fogColor"), 1, glm::value_ptr(this->fogColor));
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "fogType"), this->fogType);
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "expFogDensity"), this->expFogDensity);
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "linearFogStart"), this->linearFogStart);
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "sceneAmbient"), this->ambientLighting);
    
    this->terrainShader->use();
    glUniform3fv(glGetUniformLocation(this->terrainShader->ID, "fogColor"), 1, glm::value_ptr(this->fogColor));
    glUniform1i(glGetUniformLocation(this->terrainShader->ID, "fogType"), this->fogType);
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "expFogDensity"), this->expFogDensity);
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "linearFogStart"), this->linearFogStart);
    glUniform1f(glGetUniformLocation(this->terrainShader->ID, "sceneAmbient"), this->ambientLighting);

    this->brightShader->use();
    glUniform1f(glGetUniformLocation(this->brightShader->ID, "bloomThreshold"), this->bloomThreshold);
}

/*
void Renderer::RenderCascadedShadowMapGeo()
{
    this->cascadeShadowShader->use();

    glBindFramebuffer(GL_FRAMEBUFFER, this->cascadeShadowMapFBO); //texture array is attached
    glViewport(0, 0, CASCADE_SHADOW_WIDTH, CASCADE_SHADOW_HEIGHT);

    std::vector<glm::mat4> lightSpaceMatrices = this->GetCascadeMatrices();

    for (unsigned int i = 0; i < lightSpaceMatrices.size(); i++)
    {
        std::string m = "lightSpaceMatrices[" + std::to_string(i) + "]";
        glUniformMatrix4fv(glGetUniformLocation(this->cascadeShadowShader->ID, m.c_str()), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));
    }

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Cascade Framebuffer is not complete!" << '\n';
    }

    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);  // peter panning
    for (DrawCall* d : this->drawCalls)
    {
        d->SetCulling(false);
        d->Render(this->cascadeShadowShader);
    }

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
*/

/*
void Renderer::RenderDirShadowMap()
{
    //Simulate a position.
    //TODO: This should probably follow where the camera generally is.
    glm::vec3 lightPos = -(this->dirLight.direction) * 3.0f;

    //Snapshot properties
    float nearPlane = D_NEAR_PLANE;
    float farPlane = D_FAR_PLANE;
    float size = D_FRUSTUM_SIZE; //side length of sqaure which is the shadowmap snapshot.

    //This matrix creates the size of the shadowMap snapshot.
    //Note, its a square for simplity rn.
    glm::mat4 lightProjectionMatrix = glm::ortho(-size, size, -size, size, nearPlane, farPlane);
    glm::mat4 lightViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //looking at origin from "lightpos"

    //Transforms a fragment to the pov of the directional light
    glm::mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;

    glViewport(0, 0, this->D_SHADOW_WIDTH, this->D_SHADOW_HEIGHT); //make sure viewport is same as the texture size
    //NOTE: Setting viewport==texturesize makes it basically "fullscreen" no matter the texture resolution.

    //drawing into dir shadow framebuffer texture
    glBindFramebuffer(GL_FRAMEBUFFER, this->dirShadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT); //clear the depth buffer, start clean.

    //Send it!
    this->lightingShader->use();
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "dirLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    this->dirShadowShader->use();
    glUniformMatrix4fv(glGetUniformLocation(this->dirShadowShader->ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    //note: model matrix is sent in d->Render()

    for (DrawCall* d : this->drawCalls)
    {
        //IMPORTANT: DISABLE CULLING
        //things are culled from your view perspecitve, but when you move to lightspace those
        //things will still be culled.
        d->SetCulling(false);
        d->Render(this->dirShadowShader);
        d->SetCulling(true);
    }
}
*/
