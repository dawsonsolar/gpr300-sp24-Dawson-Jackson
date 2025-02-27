#include <iostream>
#include <ew/external/glad.h>
#include <GLFW/glfw3.h>

#include "ew/shader.h"
#include "ew/camera.h"
#include "ew/texture.h"
#include "ew/mesh.h"
#include "ew/model.h"
#include "ew/cameraController.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace ew;

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

GLuint shadowFBO, shadowMap;
GLuint planeVAO, planeVBO;
Shader* shadowShader, * lightingShader;
Camera camera;
Model* suzanneModel;
GLuint brickTexture;

glm::vec3 lightDirection(-0.5f, -1.0f, -0.5f);
glm::mat4 lightSpaceMatrix;
bool showShadowMap = false;

void setupShadowFramebuffer() 
{
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);

    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Setup ground plane
void setupPlane() 
{
    float planeVertices[] = 
    {
        -5.0f,  0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         5.0f,  0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         5.0f,  0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -5.0f,  0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void computeLightSpaceMatrix() 
{
    glm::mat4 lightProjection = glm::ortho(-7.5f, 7.5f, -7.5f, 7.5f, 1.0f, 10.0f);
    glm::mat4 lightView = glm::lookAt(-lightDirection * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpaceMatrix = lightProjection * lightView;
}

void renderScene(Shader& shader)
{
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader.setVec3("lightDir", lightDirection);
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
    shader.setMat4("model", model);
    suzanneModel->draw();
}


void renderShadowPass() 
{
    shadowShader->use();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    renderScene(*shadowShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderLightingPass()
{
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    lightingShader->use();
    lightingShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brickTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    renderScene(*lightingShader);
}

void renderGUI() 
{
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Shadow Settings");
    ImGui::Checkbox("Show Shadow Map", &showShadowMap);
    ImGui::End();

    if (showShadowMap) 
    {
        ImGui::Begin("Shadow Map");
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::Image((ImTextureID)shadowMap, windowSize, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }
}


int main() 
{
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Shadow Mapping", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);
    setupShadowFramebuffer();
    setupPlane();
    suzanneModel = new Model("assets/Suzanne.obj");
    if (!suzanneModel)
    {
        std::cerr << "failed to load suzanne" << std::endl;
        return -1;
    }
    shadowShader = new Shader("shadow.vert", "shadow.frag");
    lightingShader = new Shader("lighting.vert", "lighting.frag");
    brickTexture = ew::loadTexture("assets/brick_color.jpg");

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(window))
    {
        computeLightSpaceMatrix();
        renderShadowPass();
        renderLightingPass();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderGUI();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
