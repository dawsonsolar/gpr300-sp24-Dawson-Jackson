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

// Globals
GLuint framebuffer, colorTexture, depthBuffer;
GLuint blurFBO[2], blurTexture[2];
GLuint quadVAO, quadVBO, quadEBO;
Shader* sceneShader;
Shader* gammaShader;
Shader* blurShader;
Camera camera;
Model* suzanneModel;

float gammaValue = 2.2f;
bool useBlur = false;

// Initialize framebuffer
void setupFramebuffer()
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Color texture
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Depth buffer
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Blur Framebuffers
    glGenFramebuffers(2, blurFBO);
    glGenTextures(2, blurTexture);
    for (unsigned int i = 0; i < 2; i++) 
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, blurTexture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTexture[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// fullscreen quad
void setupQuad()
{
    float quadVertices[] =
    {
        -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  0.0f,  1.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void renderScene()
{
    sceneShader->use();
    sceneShader->setVec3("lightPos", glm::vec3(3.0f, 5.0f, 2.0f));
    sceneShader->setVec3("viewPos", camera.position);
    sceneShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); 
    sceneShader->setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f)); 

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.viewMatrix();

    sceneShader->setMat4("projection", projection);
    sceneShader->setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -3.0f)); // Move back
    sceneShader->setMat4("model", model);

    suzanneModel->draw();
}

void renderPostProcess()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Render to screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (useBlur) {
        bool horizontal = true, first_iteration = true;
        int blurAmount = 10;

        blurShader->use();
        for (unsigned int i = 0; i < blurAmount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[horizontal]);
            blurShader->setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorTexture : blurTexture[!horizontal]);
            glBindVertexArray(quadVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, blurTexture[!horizontal]);
    }
    else {
        gammaShader->use();
        gammaShader->setFloat("gamma", gammaValue);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
    }

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Post Process", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    setupFramebuffer();
    setupQuad();
    
    suzanneModel = new Model("assets/Suzanne.obj");
    sceneShader = new Shader("assets/scene.vert", "assets/scene.frag");
    gammaShader = new Shader("assets/postprocess.vert", "assets/gamma.frag");
    blurShader = new Shader("assets/postprocess.vert", "assets/postprocess.frag");

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderScene();

        renderPostProcess();

        ImGui::Begin("Settings");
        ImGui::Checkbox("Use Blur", &useBlur);
        ImGui::SliderFloat("Gamma", &gammaValue, 0.1f, 5.0f);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

