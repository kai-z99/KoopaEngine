#include "pch.h"
#include "framework.h"
#include "KoopaEngine.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Renderer.h"

#include <iostream>

KoopaEngine::KoopaEngine() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "KoopaEngine Window", NULL, NULL);
    if (!window) {
        std::cerr << "Window initialization failed.\n";
        glfwTerminate();
    }
    glfwSetWindowUserPointer(window, this); // <--- Add this line!

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, KoopaEngine::framebuffer_size_callback_static);
    glfwSetCursorPosCallback(window, KoopaEngine::mouse_callback_static);
    glfwSetScrollCallback(window, KoopaEngine::scroll_callback_static);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    this->camera = new Camera(glm::vec3(0.0f, 0.0f, 4.0f));
    this->renderer = new Renderer();
}

KoopaEngine::~KoopaEngine() 
{
    glfwDestroyWindow(window);
    glfwTerminate();
    delete this->renderer;
    delete this->camera;
    //delete this->window; glfwDestroyWindow
    std::cout << "Engine destroyed." << std::endl;
}

bool KoopaEngine::shouldCloseWindow() const
{
    return glfwWindowShouldClose(this->window);
}

void KoopaEngine::BeginFrame()
{
    this->processInput();
    this->currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = this->currentFrame - lastFrame;
    lastFrame = this->currentFrame;

    glm::mat4 view = this->camera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(this->camera->zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    this->renderer->SetCameraMatrices(view, projection, this->camera->position);

    this->renderer->BeginRenderFrame(); //sets to screen FB
}

void KoopaEngine::EndFrame()
{
    //Draw whats in final framebuffer
    this->renderer->EndRenderFrame();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

float KoopaEngine::GetCurrentFrame()
{
    return this->currentFrame;
}

void KoopaEngine::SetCurrentDiffuseTexture(const char* path)
{
    this->renderer->SetCurrentDiffuse(path);
}

void KoopaEngine::ClearScreen(Vec4 color)
{
    this->renderer->ClearScreen(color);
}

void KoopaEngine::DrawTriangle(Vec3 pos, Vec4 rotation)
{
    this->renderer->DrawTriangle(pos, rotation);
}

void KoopaEngine::DrawCube(Vec3 pos, Vec3 size, Vec4 rotation)
{
    this->renderer->DrawCube(pos, size, rotation);
}

void KoopaEngine::DrawPlane(Vec3 pos, Vec3 size, Vec4 rotation)
{
    this->renderer->DrawPlane(pos, size, rotation);
}

void KoopaEngine::DrawPointLight(Vec3 pos, Vec3 col, float intensity)
{
    this->renderer->AddPointLightToFrame(pos, col, intensity);
}

void KoopaEngine::DrawDirLight(Vec3 dir, Vec3 col, float intensity)
{
    this->renderer->AddDirLightToFrame(dir, col, intensity);
}

void KoopaEngine::DrawLightsDebug()
{
    this->renderer->DrawLightsDebug();
}

void KoopaEngine::DrawFinalQuad()
{
    //set framebuffer to 0
    //draw quad on screen with final image, bloom, ssao, etc
}

void KoopaEngine::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void KoopaEngine::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    this->camera->ProcessMouseMove(xoffset, yoffset);
}

void KoopaEngine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    //pass
}

void KoopaEngine::processInput()
{
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        this->camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        this->camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        this->camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        this->camera->ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        this->camera->ProcessKeyboard(UP, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        this->camera->ProcessKeyboard(DOWN, deltaTime);
    }
}

// Static callback wrappers
void KoopaEngine::framebuffer_size_callback_static(GLFWwindow* window, int width, int height) {
    // Retrieve the instance pointer and forward the call
    KoopaEngine* engine = static_cast<KoopaEngine*>(glfwGetWindowUserPointer(window));
    if (engine)
        engine->framebuffer_size_callback(window, width, height);
}

void KoopaEngine::mouse_callback_static(GLFWwindow* window, double xposIn, double yposIn) {
    KoopaEngine* engine = static_cast<KoopaEngine*>(glfwGetWindowUserPointer(window));
    if (engine)
        engine->mouse_callback(window, xposIn, yposIn);
}

void KoopaEngine::scroll_callback_static(GLFWwindow* window, double xoffset, double yoffset) {
    KoopaEngine* engine = static_cast<KoopaEngine*>(glfwGetWindowUserPointer(window));
    if (engine)
        engine->scroll_callback(window, xoffset, yoffset);
}