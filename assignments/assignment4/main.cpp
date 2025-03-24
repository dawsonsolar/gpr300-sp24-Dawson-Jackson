#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>
#include <ew/procGen.h>
#include <dawslib/animation.h>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void AnimatorUI();
void resetCamera(ew::Camera* camera, ew::CameraController* controller);

// Global Variables (dont forget the camera this time)
ew::Camera camera;
ew::Transform monkeyTransform;
ew::CameraController cameraController;
dawslib::Animator animator;

int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime = 0.0f;
float deltaTime = 0.0f;

struct Material 
{
    float Ka = 1.0f;
    float Kd = 0.5f;
    float Ks = 0.5f;
    float Shininess = 128.0f;
} material;

void drawUI() 
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");

    if (ImGui::Button("Reset Camera")) 
    {
        resetCamera(&camera, &cameraController);
    }

    if (ImGui::CollapsingHeader("Material")) {
        ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
        ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
        ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
        ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
    }

    ImGui::End();
    AnimatorUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void AnimatorUI() 
{
    const char* easingNames[] = { "Lerp", "In Out Sine", "In Out Quart", "In Out Back" };

    ImGui::Begin("Animator Settings");
    ImGui::Checkbox("Playing", &animator.isPlaying);
    ImGui::Checkbox("Looping", &animator.isLooping);
    ImGui::SliderFloat("Playback Speed", &animator.playbackSpeed, -5.0f, 5.0f);
    ImGui::SliderFloat("Playback Time", &animator.playbackTime, 0.0f, animator.clip->duration);
    ImGui::DragFloat("Duration", &animator.clip->duration);

    auto drawKeyframes = [&](const char* label, std::vector<dawslib::Vec3Key>& keys, const glm::vec3& defaultValue) 
        {
        if (ImGui::CollapsingHeader(label)) 
        {
            for (size_t i = 0; i < keys.size(); ++i) 
            {
                ImGui::PushID((std::string(label) + std::to_string(i)).c_str());  
                ImGui::SliderFloat("Time", &keys[i].mTime, 0.0f, animator.clip->duration);
                ImGui::DragFloat3("Value", &keys[i].mValue.x, 0.1f);
                ImGui::Combo("Interpolation Method", &keys[i].mMethod, easingNames, IM_ARRAYSIZE(easingNames));
                ImGui::PopID();
            }

            ImGui::PushID(label);
            if (ImGui::Button(std::string("Add " + std::string(label)).c_str()))
            {
                glm::vec3 lastValue = keys.empty() ? defaultValue : keys.back().mValue;
                keys.emplace_back(animator.clip->duration, lastValue);
            }
            ImGui::SameLine();
            if (ImGui::Button(std::string("Remove " + std::string(label)).c_str()) && !keys.empty())
            {
                keys.pop_back();
            }
            ImGui::PopID();
        }
        };

    drawKeyframes("Position Keys", animator.clip->positionKeys, glm::vec3(0.0f));
    drawKeyframes("Rotation Keys", animator.clip->rotationKeys, glm::vec3(0.0f));
    drawKeyframes("Scale Keys", animator.clip->scaleKeys, glm::vec3(1.0f));

    ImGui::End();
}





/// Handles framebuffer size change events
void framebufferSizeCallback(GLFWwindow* window, int width, int height) 
{
    glViewport(0, 0, width, height);
    screenWidth = width;
    screenHeight = height;
}

/// Initializes GLFW, GLAD, and IMGUI
GLFWwindow* initWindow(const char* title, int width, int height) 
{
    printf("Initializing...\n");

    if (!glfwInit()) 
    {
        printf("GLFW failed to initialize!\n");
        return nullptr;
    }

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) 
    {
        printf("GLFW failed to create window!\n");
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) 
    {
        printf("GLAD failed to load OpenGL!\n");
        return nullptr;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    return window;
}

/// Resets the camera position and controller settings
void resetCamera(ew::Camera* camera, ew::CameraController* controller) 
{
    camera->position = glm::vec3(0, 0, 5.0f);
    camera->target = glm::vec3(0);
    controller->yaw = 0;
    controller->pitch = 0;
}


int main() 
{
    GLFWwindow* window = initWindow("Assignment 4", screenWidth, screenHeight);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");
    ew::Model monkeyModel = ew::Model("assets/suzanne.obj");
    GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");
    ew::Mesh planeMesh(ew::createPlane(5.0f, 5.0f, 10.0f));
    ew::Transform planeTransform;
    planeTransform.position = glm::vec3(0, -2.0, 0);

    camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.aspectRatio = (float)screenWidth / screenHeight;
    camera.fov = 60.0f;

    animator.clip = new dawslib::AnimationClip();
    animator.clip->duration = 5;
    animator.isPlaying = true;
    animator.isLooping = true;

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();

        float time = (float)glfwGetTime();
        deltaTime = time - prevFrameTime;
        prevFrameTime = time;

        cameraController.move(window, &camera, deltaTime);

        animator.Update(deltaTime);

        // Ensure transformations are applied independently
        glm::vec3 newPosition = animator.GetValue(animator.clip->positionKeys, glm::vec3(0));
        glm::vec3 newRotation = animator.GetValue(animator.clip->rotationKeys, glm::vec3(0));
        glm::vec3 newScale = animator.GetValue(animator.clip->scaleKeys, glm::vec3(1));

        // Apply position
        monkeyTransform.position = newPosition;

        // Apply rotation (converting degrees to radians)
        monkeyTransform.rotation = glm::radians(newRotation);

        // Apply scale
        monkeyTransform.scale = newScale;

        glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindTextureUnit(0, brickTexture);

        shader.use();
        shader.setVec3("_EyePos", camera.position);
        shader.setInt("_MainTex", 0);
        shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
        shader.setFloat("_Material.Ka", material.Ka);
        shader.setFloat("_Material.Kd", material.Kd);
        shader.setFloat("_Material.Ks", material.Ks);
        shader.setFloat("_Material.Shininess", material.Shininess);

        shader.setMat4("_Model", planeTransform.modelMatrix());
        planeMesh.draw();


        shader.setMat4("_Model", monkeyTransform.modelMatrix());
        monkeyModel.draw();

        drawUI();

        glfwSwapBuffers(window);
    }
    printf("Shutting down...");
}
