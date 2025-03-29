#include "pch.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "shaderSources.h"
#include "Shader.h"
#include "DrawCall.h"
#include "Setup.h"
#include "Camera.h"
#include "Model.h"

#include <iostream>

Renderer::Renderer()
{
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    //Static setup
	this->SetupVertexBuffers();
    //                              2               4                      10                  25
    this->cascadeLevels = { DEFAULT_FAR / 50.0f, DEFAULT_FAR / 25.0f, DEFAULT_FAR / 10.0f, DEFAULT_FAR / 3.0f };
    this->drawCalls = {};
    this->usingSkybox = false;
    this->clearColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

    //SHADERS-------------------------------------------------

    //Main lighting shader
    this->lightingShader = new Shader(ShaderSources::vs1, ShaderSources::fs1);
    this->lightingShader->use();
    this->currentDiffuseTexture = TextureSetup::LoadTexture("../ShareLib/Resources/wood.png");
    this->currentNormalMapTexture = TextureSetup::LoadTexture("../ShareLib/Resources/brickwall_normal.jpg");
    //ASSOCIATE TEXTURES----
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "currentDiffuse"), 0);   //GL_TEXTURE0
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "currentNormalMap"), 1); //GL_TEXTURE1
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirShadowMap"), 2);     //GL_TEXTURE2
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "pointShadowMapArray"), 3);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "cascadeShadowMaps"), 4); 

    //Stuff for cascade shadows
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "cascadeCount"), NUM_CASCADES); //4 (5 matrices)
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

    //2 pass blur shader
    this->blurShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsBlur);
    this->blurShader->use();
    glUniform1i(glGetUniformLocation(this->blurShader->ID, "brightScene"), 0); //GL_TEXTIRE0
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

    //LIGHTING-------------------------------------------------

    //Initialialize lighting
    this->InitializePointLights();
    this->InitializeDirLight();
    this->currentFramePointLightCount = 0;

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        std::cout << "glFramebufferTextureLayer error on efwcascade " << ": " << err << '\n';
    }
    
    //FBOs AND TEXTURES-------------------------------------------------
 
    //NOTE: DO THIS AFTER THE POINT LIGHTS ARE INITIALIZED... facepalm
    this->SetupFramebuffers();
}

Renderer::~Renderer()
{
    //VBO/VAO
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &screenQuadVAO);

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
    //dir
    if (this->dirLight.castShadows) this->RenderCascadedShadowMap(); //Sets active FB to the shadow one in function

    //point
    for (unsigned int i = 0; i < currentFramePointLightCount; i++)
    {
        if (this->pointLights[i].castShadows)
        {
            this->RenderPointShadowMap(i);
        }
    }
        
    //BIND SHADOWMAP TEXTURES---
    //dir
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, this->dirShadowMapTextureDepth); //global binding, not related to shader so cant do it in contructor.
    //point
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, this->pointShadowMapTextureArrayDepth);
    //cascade
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D_ARRAY, this->cascadeShadowMapTextureArrayDepth);

    //DRAW INTO FINAL IMAGE---
    //bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, this->hdrFBO);
    //clear main scene with current color, clear bright scene with black always.
    glClearBufferfv(GL_COLOR, 0, (float*) & this->clearColor); 
    GLfloat black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glClearBufferfv(GL_COLOR, 1, black); 

    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); 
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT); 
    //render
    for (DrawCall* d : this->drawCalls)
    {
        //Sends and binds the normalmap, diffusemap textures. (if applicable)
        d->BindTextureProperties(this->lightingShader);

        //Draw with shader. (model matrix is sent here)
        d->Render(this->lightingShader);
    }
    this->drawCalls.clear(); //raylib style
    //draw debug light into FBO is applicable
    if (this->drawDebugLights) this->DrawLightsDebug();

    //Draw skybox last (using z = w optimization)
    if (this->usingSkybox) this->DrawSkybox();
    

    //DO BLOOM STUFF---
    this->BlurBrightScene();

    //DRAW QUAD ONTO SCREEN---
    //go to main FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    glClear(GL_COLOR_BUFFER_BIT);
    //Draw the final scene on a full screen quad.
    this->DrawFinalQuad();

    //CLEANUP---
    //reset lights for the next frame
    this->SetAndSendAllLightsToFalse(); //uniforms are sent here too.
    
    
} 

void Renderer::DrawFinalQuad()
{
    glDisable(GL_DEPTH_TEST); //will be drawing directly in front screen
    this->screenShader->use();
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glBindVertexArray(this->screenQuadVAO); //whole screen
    glActiveTexture(GL_TEXTURE0); //0: hdrBuffer in shader
    glBindTexture(GL_TEXTURE_2D, this->hdrColorBuffers[0]); // 0:hdrTextureRGBA...HDRframebuffer texture to hdrBuffer in shader
    glActiveTexture(GL_TEXTURE1); //1: blurBuffer in shader
    glBindTexture(GL_TEXTURE_2D, this->twoPassBlurTexturesRGBA[1]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::BlurBrightScene()
{
    //blit bright scene into half res texture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->hdrFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->halfResBrightFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT1); //bright-only scene

    glBlitFramebuffer(
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,      // Source rectangle (full resolution)
        0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2,    // Destination rectangle (half resolution)
        GL_COLOR_BUFFER_BIT,                    // Copy only the color buffer
        GL_LINEAR                               // Linear filter for downsampling
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    blurShader->use();
    unsigned int amount = BLUR_PASSES * 2; //* 2 since its a two pass blur
    bool horizontal = true;
    bool first = true;
    for (unsigned int i = 0; i < amount; i++)
    {
        glUniform1i(glGetUniformLocation(blurShader->ID, "horizontal"), horizontal);
        //Bind the correct FBO
        glBindFramebuffer(GL_FRAMEBUFFER, this->twoPassBlurFBOs[horizontal]);
        glViewport(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        //Bind textures
        glActiveTexture(GL_TEXTURE0);   //          BrightScene
        glBindTexture(GL_TEXTURE_2D, first ? this->hdrColorBuffers[1] : this->twoPassBlurTexturesRGBA[!horizontal]);
        
        glBindVertexArray(this->screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
        if (first) first = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawSkybox()
{
    this->skyShader->use();
    glDepthFunc(GL_LEQUAL);
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);
    glBindVertexArray(this->skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->currentSkyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    unsigned int attachments2[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments2);
}

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

glm::mat4 Renderer::CalculateLightSpaceCascadeMatrix(float near, float far)
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
    glm::mat4 lightView = glm::lookAt(center - glm::normalize(this->dirLight.direction), center, glm::vec3(0.0f, 1.0f, 0.0f));
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

    

    //Pull in near plane, push out far plane
    float zMult = 10.0f;
    minZ = (minZ < 0) ? minZ * zMult : minZ / zMult;
    maxZ = (maxZ < 0) ? maxZ / zMult : maxZ * zMult;
    
    float offsetNear = 0.0f;
    float offsetFar = 3.0f; //fixes bug where shadow disappears if your too close.

    minZ -= offsetNear;
    maxZ += offsetFar;
    
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
            ret.push_back(CalculateLightSpaceCascadeMatrix(DEFAULT_NEAR, this->cascadeLevels[i]));
        }
        else if (i < this->cascadeLevels.size())
        {
            ret.push_back(CalculateLightSpaceCascadeMatrix(this->cascadeLevels[i - 1], this->cascadeLevels[i]));
        }
        else
        {
            ret.push_back(CalculateLightSpaceCascadeMatrix(this->cascadeLevels[i - 1], DEFAULT_FAR));
        }
    }

    return ret;
}

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

        //send lightspace matrix to lighting shader's array for later use
        this->lightingShader->use();
        std::string l = "cascadeLightSpaceMatrices[" + std::to_string(i) + "]";
        glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, l.c_str()), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));

        //send lightspace matrix to cascade vertex shader for current use
        this->cascadeShadowShader->use();
        glUniformMatrix4fv(glGetUniformLocation(this->cascadeShadowShader->ID, "lightSpaceMatrix"), 1,
            false, glm::value_ptr(lightSpaceMatrices[i]));
       
        //note: model matrix is sent in d->Render()
        //glCullFace(GL_FRONT);
        for (DrawCall* d : this->drawCalls)
        {
            d->SetCulling(false);
            d->Render(this->cascadeShadowShader);
        }
        glCullFace(GL_BACK);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

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

void Renderer::RenderPointShadowMap(unsigned int index)
{
    this->pointShadowShader->use();

    //create shadow proj matrix base
    float aspect = (float)P_SHADOW_WIDTH / (float)P_SHADOW_HEIGHT;
    float near = 0.1f;
    float far = 25.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

    //Set shadow transforms
    this->shadowTransforms.clear();
    glm::vec3 lightPos = this->pointLights[index].position;   //view

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
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "farPlane"), far);
    this->pointShadowShader->use();
    glUniform1f(glGetUniformLocation(this->pointShadowShader->ID, "farPlane"), far);
    glUniform3fv(glGetUniformLocation(this->pointShadowShader->ID, "lightPos"), 1, glm::value_ptr(lightPos));

    //Render each face of the cubemap
    for (int i = 0; i < 6; i++)
    {
        GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        //set output texture (the thing being poured into) (render target)
        //set the face from the cube texture array
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->pointShadowMapTextureArrayDepth, 0, index * 6 + i);
        glUniformMatrix4fv(glGetUniformLocation(this->pointShadowShader->ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));

        //clear the currently bound attachment's depth buffer
        glClear(GL_DEPTH_BUFFER_BIT); 
        //render
        for (DrawCall* d : this->drawCalls)
        {
            d->SetCulling(false);
            d->Render(this->pointShadowShader);
            d->SetCulling(true);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void Renderer::SetCurrentDiffuse(const char* path)
{
    AddToTextureMap(path);
    this->drawCalls.back()->SetDiffuseMapTexture(this->textureToID[path]);
}

void Renderer::SetCurrentColorDiffuse(Vec3 col)
{
    this->drawCalls.back()->SetDiffuseColor(col);
}

void Renderer::SetCurrentNormal(const char* path)
{
    AddToTextureMap(path);
    this->drawCalls.back()->SetNormalMapTexture(this->textureToID[path]);
}

void Renderer::SetSkybox(const std::vector<const char*>& faces)
{
    this->currentSkyboxTexture = TextureSetup::LoadTextureCubeMap(faces);
    this->usingSkybox = true;
}

void Renderer::SetExposure(float exposure)
{
    this->screenShader->use();
    glUniform1f(glGetUniformLocation(this->screenShader->ID, "exposure"), exposure);
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
    this->drawCalls.push_back(new DrawCall(this->triangleVAO, 3, model));
}

void Renderer::DrawCube(Vec3 pos, Vec3 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, size);
    this->drawCalls.push_back(new DrawCall(this->cubeVAO, 36, model));
}

void Renderer::DrawPlane(Vec3 pos, Vec2 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, Vec3(size.x, 1.0f, size.y));
    this->drawCalls.push_back(new DrawCall(this->planeVAO, 6, model));
    this->drawCalls.back()->SetCulling(false); //Cannot cull flat things like plane
    //TODO: How is this not being set to true in the shadow rendering?
}

void Renderer::DrawSphere(Vec3 pos, Vec3 size, Vec4 rotation)
{
    glm::mat4 model = CreateModelMatrix(pos, rotation, size);
    this->drawCalls.push_back(new DrawCall(this->sphereVAO, SPHERE_X_SEGMENTS * SPHERE_Y_SEGMENTS * 6, model));
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
    this->drawCalls.push_back(new DrawCall(this->pathToModel[path], model));

    stbi_set_flip_vertically_on_load(false);
}

void Renderer::DrawLightsDebug()
{
    this->debugLightShader->use();
    glBindVertexArray(cubeVAO);

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        PointLight curr = this->pointLights[i];
        if (!curr.isActive) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(curr.position.x, curr.position.y, curr.position.z));
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        glUniformMatrix4fv(glGetUniformLocation(this->debugLightShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(debugLightShader->ID, "lightColor"), 1, glm::value_ptr(curr.color));
        glUniform1f(glGetUniformLocation(debugLightShader->ID, "intensity"),curr.intensity);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void Renderer::AddPointLightToFrame(Vec3 pos, Vec3 col, float intensity, bool shadow)
{
    unsigned int i = this->currentFramePointLightCount;

    if (i + 1 <= MAX_POINT_LIGHTS)
    {
        //"Activate the light"
        this->pointLights[i].isActive = true;
        this->pointLights[i].position = { pos.x, pos.y, pos.z };
        this->pointLights[i].color = { col.r, col.g, col.b };
        this->pointLights[i].intensity = intensity;
        this->pointLights[i].castShadows = shadow;
        
        this->SendPointLightUniforms(this->currentFramePointLightCount);
        this->currentFramePointLightCount++;
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, "numPointLights"), this->currentFramePointLightCount);
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
    for (int i = 0; i < 4; i++)
    {
        this->pointLights[i].isActive = false;
        this->SendPointLightUniforms(i);
    }
    this->currentFramePointLightCount = 0;
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "numPointLights"), 0);

    this->dirLight.isActive = false;
    this->SendDirLightUniforms();
}

void Renderer::SendPointLightUniforms(unsigned int index)
{
    this->lightingShader->use();

    std::string s = "pointLights[" + std::to_string(index) + "].position";
    const char* cs = s.c_str();
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, cs), 1, glm::value_ptr(this->pointLights[index].position));

    s = "pointLights[" + std::to_string(index) + "].color";
    const char* csc = s.c_str();
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, csc), 1, glm::value_ptr(this->pointLights[index].color));

    s = "pointLights[" + std::to_string(index) + "].isActive";
    const char* csa = s.c_str();
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, csa), this->pointLights[index].isActive);

    s = "pointLights[" + std::to_string(index) + "].intensity";
    const char* csi = s.c_str();
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, csi), this->pointLights[index].intensity);

    s = "pointLights[" + std::to_string(index) + "].castShadows";
    const char* css = s.c_str();
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, css), this->pointLights[index].castShadows);
}

void Renderer::SendDirLightUniforms()
{
    this->lightingShader->use();

    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "dirLight.direction"), 1, glm::value_ptr(dirLight.direction));
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "dirLight.color"), 1, glm::value_ptr(dirLight.color));
    glUniform1f(glGetUniformLocation(this->lightingShader->ID, "dirLight.intensity"), dirLight.intensity);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirLight.isActive"), dirLight.isActive);
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirLight.castShadows"), dirLight.castShadows);
}

void Renderer::InitializePointLights()
{
    for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        this->pointLights[i] = PointLight();
        this->SendPointLightUniforms(i);
    }
}

void Renderer::InitializeDirLight()
{
    DirLight d = DirLight();
    this->SendDirLightUniforms();
}

void Renderer::SetCameraMatrices(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& position)
{
    this->lightingShader->use();
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(this->lightingShader->ID, "viewPos"), 1, glm::value_ptr(position));

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
}

void Renderer::SetupFramebuffers()
{
    FramebufferSetup::SetupHDRFramebuffer(this->hdrFBO, this->hdrColorBuffers);
    FramebufferSetup::SetupTwoPassBlurFramebuffers(this->twoPassBlurFBOs, this->twoPassBlurTexturesRGBA);
    FramebufferSetup::SetupHalfResBrightFramebuffer(this->halfResBrightFBO, this->halfResBrightTextureRGBA);

    FramebufferSetup::SetupDirShadowMapFramebuffer(this->dirShadowMapFBO, this->dirShadowMapTextureDepth, 
        this->D_SHADOW_WIDTH, this->D_SHADOW_HEIGHT);

    FramebufferSetup::SetupCascadedShadowMapFramebuffer(this->cascadeShadowMapFBO, this->cascadeShadowMapTextureArrayDepth,
        this->CASCADE_SHADOW_WIDTH, this->CASCADE_SHADOW_HEIGHT, (int)this->cascadeLevels.size() + 1);

    //Point shadows
    FramebufferSetup::SetupPointShadowMapFramebuffer(this->pointShadowMapFBO);
    TextureSetup::SetupPointShadowMapTextureArray(this->pointShadowMapTextureArrayDepth, P_SHADOW_WIDTH, P_SHADOW_HEIGHT);
}

void Renderer::SetupVertexBuffers()
{
    this->triangleVAO = VertexBufferSetup::SetupTriangleBuffers();
    this->cubeVAO = VertexBufferSetup::SetupCubeBuffers();
    this->planeVAO = VertexBufferSetup::SetupPlaneBuffers();
    this->sphereVAO = VertexBufferSetup::SetupSphereBuffers();
    this->screenQuadVAO = VertexBufferSetup::SetupScreenQuadBuffers();
    this->skyboxVAO = VertexBufferSetup::SetupSkyboxBuffers();
}

