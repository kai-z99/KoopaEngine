#include "pch.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include "shaderSources.h"
#include "Shader.h"
#include "DrawCall.h"
#include "Setup.h"

#include <iostream>

Renderer::Renderer()
{
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    //Static setup
	this->SetupVertexBuffers();
    
    //Main lighting shader
    this->lightingShader = new Shader(ShaderSources::vs1, ShaderSources::fs1);
    this->lightingShader->use();
    this->currentDiffuseTexture = TextureSetup::LoadTexture("../ShareLib/Resources/wood.png");
    this->currentNormalMapTexture = TextureSetup::LoadTexture("../ShareLib/Resources/brickwall_normal.jpg");
    //Associate
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "currentDiffuse"), 0);   //GL_TEXTURE0
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "currentNormalMap"), 1); //GL_TEXTURE1
    glUniform1i(glGetUniformLocation(this->lightingShader->ID, "dirShadowMap"), 2);     //GL_TEXTURE2

    //glUniform1i(glGetUniformLocation(this->lightingShader->ID, "pointShadowMap"), 3);   //GL_TEXTURE3
    // Bind each point shadow cubemap to a consecutive texture unit starting from GL_TEXTURE3
    for (unsigned int i = 0; i < this->MAX_POINT_LIGHTS; i++) 
    {
        std::string uniformName = "pointShadowMaps[" + std::to_string(i) + "]";
        glUniform1i(glGetUniformLocation(this->lightingShader->ID, uniformName.c_str()), 3 + i);
    }
    //GL_TEXTURE[3-6] have been used.
     
    //Check DrawCall.BindTextureProperties

    //Debug lighting shader
    this->debugLightShader = new Shader(ShaderSources::vs1, ShaderSources::fsLight);
    this->drawDebugLights = false;

    //Final quad shader
    this->screenShader = new Shader(ShaderSources::vsScreenQuad, ShaderSources::fsScreenQuad);
    this->screenShader->use();
    glUniform1i(glGetUniformLocation(this->screenShader->ID, "screenTexture"), 0); //GL_TEXTIRE0

    //Dir shadow shader
    this->dirShadowShader = new Shader(ShaderSources::vsDirShadow, ShaderSources::fsDirShadow);

    //point shadow shader
    this->pointShadowShader = new Shader(ShaderSources::vsPointShadow, ShaderSources::fsPointShadow);

    //skybox shader
    this->skyShader = new Shader(ShaderSources::vsSkybox, ShaderSources::fsSkybox);
    glUniform1i(glGetUniformLocation(this->skyShader->ID, "skyboxTexture"), 0); //GL_TEXTURE0

    //Initialialize lighting
    this->InitializePointLights();
    this->InitializeDirLight();
    this->currentFramePointLightCount = 0;

    //Draw call vector
    this->drawCalls = {};

    //framebuffers/textures
    //NOTE: DO THIS AFTER THE POINT LIGHTS ARE INITIALIZED... facepalm
    this->SetupFramebuffers();

    this->usingSkybox = false;
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
    //glBindFramebuffer(GL_FRAMEBUFFER, this->finalImageFBO); //off screen render
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::EndRenderFrame()
{
    //RENDER SHADOW MAPS---
    //dir
    if (this->dirLight.castShadows) this->RenderDirShadowMap(); //Sets active FB to the shadow one in function
    //point
    for (int i = 0; i < currentFramePointLightCount; i++)
    {
        if (this->pointLights[i].castShadows)
        {
            this->RenderPointShadowMap(i);
        }
    }
        
    //BIND SHADOWMAP TEXTURES---
    //dir
    glActiveTexture(GL_TEXTURE2); //dirshadowmap is 2
    glBindTexture(GL_TEXTURE_2D, this->dirShadowMapTextureDepth); //global binding, not related to shader so cant do it in contructor.
    //point
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        glActiveTexture(GL_TEXTURE3 + i); //pointshadowmap is 3-6
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->pointLights[i].shadowMapTexture);
    }

    //DRAW INTO FINAL IMAGE---
    //bind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, this->finalImageFBO); //off screen render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //remove stuff from previous frames on the currently attached textures
    glViewport(0, 0, 800, 600); //temp hardcode
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

    glBindVertexArray(this->screenQuadVAO); //whole screen
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->finalImageTextureRGB); // main scene framebuffer texture to GL_TEXTURE0
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawSkybox()
{
    this->skyShader->use();
    glDepthFunc(GL_LEQUAL);

    glBindVertexArray(this->skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->currentSkyboxTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void Renderer::RenderDirShadowMap()
{
    //Simulate a position.
    //TODO: This should probably follow where the camera generally is.
    glm::vec3 lightPos = -(this->dirLight.direction) * 3.0f; 

    //Snapshot properties
    float nearPlane = 1.0f;
    float farPlane = 10.5f;
    float size = 10.0f; //side length of sqaure which is the shadowmap snapshot.

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
    glUniformMatrix4fv(glGetUniformLocation(this->lightingShader->ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

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
    glClear(GL_DEPTH_BUFFER_BIT); //clear the depth buffer
    glViewport(0, 0, P_SHADOW_WIDTH, P_SHADOW_HEIGHT); //make sure the window rectangle is the shadowmap size
    
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
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, face, this->pointLights[index].shadowMapTexture, 0);
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //fill it with that clear color.
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
    FramebufferSetup::SetupFinalImageFramebuffer(this->finalImageFBO, this->finalImageTextureRGB);

    FramebufferSetup::SetupDirShadowMapFramebuffer(this->dirShadowMapFBO, this->dirShadowMapTextureDepth, 
        this->D_SHADOW_WIDTH, this->D_SHADOW_HEIGHT);

    //Point shadows: one FBO, many textures
    FramebufferSetup::SetupPointShadowMapFramebuffer(this->pointShadowMapFBO);
    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
    {
        TextureSetup::SetupPointShadowMapTexture(pointLights[i].shadowMapTexture, this->P_SHADOW_WIDTH, this->P_SHADOW_HEIGHT);
    }
}

void Renderer::SetupVertexBuffers()
{
    this->triangleVAO = VertexBufferSetup::SetupTriangleBuffers();
    this->cubeVAO = VertexBufferSetup::SetupCubeBuffers();
    this->planeVAO = VertexBufferSetup::SetupPlaneBuffers();
    this->screenQuadVAO = VertexBufferSetup::SetupScreenQuadBuffers();
    this->skyboxVAO = VertexBufferSetup::SetupSkyboxBuffers();
}

