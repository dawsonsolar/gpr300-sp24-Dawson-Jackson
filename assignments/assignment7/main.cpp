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

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


using namespace ew;

int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;
int gBufferWidth = 800;
int gBufferHeight = 600;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

GLuint shadowFBO, shadowMap;
GLuint planeVAO, planeVBO, planeEBO;
GLuint quadVAO = 0;
GLuint quadVBO;
GLuint decalVAO = 0;
GLuint decalVBO;
GLuint decalTexture;


Shader* decalShader;
Shader* shadowShader, * lightingShader;
Shader* geometryShader;
Shader* decalPreviewShader;

Camera camera;
CameraController cameraController;
Model* suzanneModel;
GLuint brickTexture;

GLuint gBuffer;
GLuint gPosition, gNormal, gAlbedo;
GLuint gDepth;

GLFWwindow* window;

glm::vec3 lightDirection(-0.5f, -1.0f, -0.5f);
glm::mat4 lightSpaceMatrix;
bool showShadowMap = false;
static bool showDecalPreview = true;



struct Decal
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // In degrees (Euler angles)
    glm::vec3 color = glm::vec3(1.0f);
};

std::vector<Decal> decals;

GLuint decalTextures[3]; // Array for 3 decals
const char* decalNames[] = { "Bullet Hole", "Blood", "Esports" }; // Names for UI
int selectedDecal = 0; // Selected decal index

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;

    gBufferWidth = width;
    gBufferHeight = height;
}


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

void setupGBuffer()
{
    if (gBuffer != 0)
    {
        glDeleteFramebuffers(1, &gBuffer);
        glDeleteTextures(1, &gPosition);
        glDeleteTextures(1, &gNormal);
        glDeleteTextures(1, &gAlbedo);
    }

    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // Create Position texture
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, gBufferWidth, gBufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // Create Normal texture
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, gBufferWidth, gBufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // Create Albedo texture
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, gBufferWidth, gBufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    // Create and attach depth texture
    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, gBufferWidth, gBufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "GBuffer not complete!" << std::endl;
    }

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

    glGenBuffers(1, &planeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void setupDecalCube()
{
    float cubeVertices[] = 
    {
        // positions
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    glGenVertexArrays(1, &decalVAO);
    glGenBuffers(1, &decalVBO);
    glBindVertexArray(decalVAO);

    glBindBuffer(GL_ARRAY_BUFFER, decalVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
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
    shader.setMat4("view", camera.viewMatrix());
    shader.setMat4("projection", camera.projectionMatrix());

    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lightingShader->use();
    lightingShader->setVec3("lightDir", lightDirection);
    lightingShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    lightingShader->setMat4("projection", camera.projectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    lightingShader->setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    lightingShader->setInt("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    lightingShader->setInt("gAlbedo", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    lightingShader->setInt("shadowMap", 3);

    renderQuad();
}




void renderGeometryPass()
{
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryShader->use();
    geometryShader->setMat4("view", camera.viewMatrix());
    geometryShader->setMat4("projection", camera.projectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brickTexture);

    renderScene(*geometryShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderDecalPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    decalShader->use();
    decalShader->setMat4("view", camera.viewMatrix());
    decalShader->setMat4("projection", camera.projectionMatrix());
    decalShader->setMat4("inverseView", glm::inverse(camera.viewMatrix()));
    decalShader->setMat4("inverseProjection", glm::inverse(camera.projectionMatrix()));
    decalShader->setVec2("screenSize", glm::vec2(gBufferWidth, gBufferHeight));
    decalShader->setFloat("nearPlane", 0.1f);
    decalShader->setFloat("farPlane", 100.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    decalShader->setInt("depthTex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    decalShader->setInt("gPositionTex", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    decalShader->setInt("gNormalTex", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    decalShader->setInt("gAlbedoTex", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, decalTexture);
    decalShader->setInt("decalTex", 4);

    glBindVertexArray(decalVAO);

    for (auto& decal : decals)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, decal.position);
        model = glm::rotate(model, glm::radians(decal.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(decal.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(decal.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, decal.scale);

        decalShader->setMat4("model", model);
        decalShader->setMat4("decalModelInverse", glm::inverse(model));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void renderDecalPreviews()
{
    if (!showDecalPreview)
        return;

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_DEPTH_TEST);

    decalPreviewShader->use();
    decalPreviewShader->setMat4("view", camera.viewMatrix());
    decalPreviewShader->setMat4("projection", camera.projectionMatrix());

    glBindVertexArray(decalVAO);

    for (auto& decal : decals)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, decal.position);
        model = glm::rotate(model, glm::radians(decal.rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(decal.rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(decal.rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, decal.scale);

        decalPreviewShader->setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
}


void renderGUI()
{

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    computeLightSpaceMatrix();
    renderShadowPass();
    renderGeometryPass();
    renderDecalPass();
    renderLightingPass();
    renderDecalPreviews();

    ImGui::Begin("Decal Controls");

    ImGui::Checkbox("Show Decal Preview", &showDecalPreview);
    if (showDecalPreview)
    {
        renderDecalPreviews();
    }

    if (ImGui::Button("Add Decal"))
    {
        Decal newDecal;
        newDecal.position = glm::vec3(0.0f, 1.0f, 0.0f); 
        newDecal.scale = glm::vec3(0.5f, 0.5f, 0.5f); 
        newDecal.rotation = glm::vec3(0.0f); 
        newDecal.color = glm::vec3(1.0f); 
        decals.push_back(newDecal);
    }

    ImGui::Separator();

    for (int i = 0; i < decals.size(); ++i)
    {
        ImGui::PushID(i);
        ImGui::Text("Decal %d", i);
        ImGui::DragFloat3("Position", &decals[i].position.x, 0.1f);
        ImGui::DragFloat3("Scale", &decals[i].scale.x, 0.05f, 0.05f, 5.0f);
        ImGui::DragFloat3("Rotation", &decals[i].rotation.x, 1.0f, -180.0f, 180.0f);
        if (ImGui::Button("Delete"))
        {
            decals.erase(decals.begin() + i);
            ImGui::PopID();
            break; 
        }
        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::End();

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

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Post Process", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    setupShadowFramebuffer();
    setupGBuffer();
    setupPlane();
    setupDecalCube();
    std::cout << "Decal VAO = " << decalVAO << std::endl;

    decalShader = new Shader("assets/decal.vert", "assets/decal.frag");
    decalPreviewShader = new Shader("assets/decal_preview.vert", "assets/decal_preview.frag");
    geometryShader = new Shader("assets/geometry.vert", "assets/geometry.frag");
    lightingShader = new Shader("assets/lighting.vert", "assets/lighting.frag");
    shadowShader = new Shader("assets/shadow.vert", "assets/shadow.frag");
    suzanneModel = new Model("assets/suzanne.obj");
    brickTexture = ew::loadTexture("assets/brick_color.jpg");
    decalTextures[0] = ew::loadTexture("assets/bullethole.png");
    decalTextures[1] = ew::loadTexture("assets/bloodsplatter.png");
    decalTextures[2] = ew::loadTexture("assets/logo-cc-esports.png");

    // Initialize camera
    camera.position = (glm::vec3(0.0f, 0.0f, 3.0f));

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    int lastWidth = SCREEN_WIDTH;
    int lastHeight = SCREEN_HEIGHT;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);

        if (currentWidth != lastWidth || currentHeight != lastHeight)
        {
            SCREEN_WIDTH = currentWidth;
            SCREEN_HEIGHT = currentHeight;
            gBufferWidth = currentWidth;
            gBufferHeight = currentHeight;

            setupGBuffer();
            camera.aspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

            lastWidth = currentWidth;
            lastHeight = currentHeight;
        }
        cameraController.move(window, &camera, 0.016f);

        renderGUI();
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
