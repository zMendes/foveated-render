#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include "shader.h"
#include "camera.h"
#include "stb_image.h"
#include "model.h"

#include <iostream>
#include <algorithm>
#include <vector>

#define GL_SHADING_RATE_IMAGE_NV 0x9563
#define GL_SHADING_RATE_IMAGE_PER_PRIMITIVE_NV 0x95B1
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV 0x955C
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV 0x955D
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV 0x955E
#define GL_SHADING_RATE_NO_INVOCATIONS_NV 0x9564
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV 0x9565
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV 0x955E
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV 0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV 0x956B

typedef void(APIENTRYP PFNGLBINDSHADINGRATEIMAGENVPROC)(GLuint texture);
PFNGLBINDSHADINGRATEIMAGENVPROC glBindShadingRateImageNV = nullptr;

typedef void(APIENTRYP PFNGLSHADINGRATEIMAGEPALETTENVPROC)(GLuint viewport, GLuint first, GLsizei count, const GLenum *rates);
PFNGLSHADINGRATEIMAGEPALETTENVPROC glShadingRateImagePaletteNV = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void createFoveationTexture(float centerX, float centerY);
void uploadFoveationDataToTexture(GLuint texture);
void setupShadingRatePalette();
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);
void renderCube();
void createTexture(GLuint &glid);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
glm::vec2 getJitterOffset(int frame);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

const unsigned int G_WIDTH = 1600 / 4;
const unsigned int G_HEIGHT = 900 / 4;

// VRS stuff
GLuint fov_texture;
std::vector<uint8_t> m_shadingRateImageData;
uint32_t m_shadingRateImageWidth = 0;
uint32_t m_shadingRateImageHeight = 0;
GLint m_shadingRateImageTexelWidth;
GLint m_shadingRateImageTexelHeight;
float posX = 0.5;
float posY = 0.5;
bool isCursorEnabled = false;

// CAMERA
Camera camera(glm::vec3(0.0f, 2.0f, 8.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool showShading = false;
int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VRS", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // IMGUI
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1; // Correct return here, exiting if GLAD fails
    }

    glBindShadingRateImageNV = (PFNGLBINDSHADINGRATEIMAGENVPROC)glfwGetProcAddress("glBindShadingRateImageNV");

    if (!glBindShadingRateImageNV)
    {
        std::cerr << "Failed to load glBindShadingRateImageNV!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded glBindShadingRateImageNV!" << std::endl;
    }

    glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)glfwGetProcAddress("glShadingRateImagePaletteNV");

    if (!glShadingRateImagePaletteNV)
    {
        std::cerr << "Failed to load glShadingRateImagePaletteNV!" << std::endl;
    }
    else
    {
        std::cout << "Successfully loaded glShadingRateImagePaletteNV!" << std::endl;
    }

    if (!glfwExtensionSupported("GL_NV_shading_rate_image"))
    {
        std::cerr << "GL_NV_shading_rate_image not supported!" << std::endl;
    }

    // OPENGL STATE
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SHADING_RATE_IMAGE_NV);

    glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV, &m_shadingRateImageTexelHeight);
    glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV, &m_shadingRateImageTexelWidth);

    m_shadingRateImageWidth = (SCR_WIDTH + m_shadingRateImageTexelWidth - 1) / m_shadingRateImageTexelWidth;
    m_shadingRateImageHeight = (SCR_HEIGHT + m_shadingRateImageTexelHeight - 1) / m_shadingRateImageTexelHeight;
    m_shadingRateImageData.resize(m_shadingRateImageWidth * m_shadingRateImageHeight);

    createTexture(fov_texture);
    setupShadingRatePalette();
    createFoveationTexture(0.5, 0.5);

    Shader ("geometry.vs", "geometry.fs");
    Shader shader("vrs.vs", "vrs.fs");
    Shader screenShader("screen.vs", "screen.fs");
    shader.use();

    std::string path = "../resources/sponza/sponza.obj";
    Model sponza(path);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error before main loop: " << err << std::endl;
    }

    // QUAD VAO
    float quadVertices[] = {// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
                            // positions   // texCoords
                            -1.0f, 1.0f, 0.0f, 1.0f,
                            -1.0f, -1.0f, 0.0f, 0.0f,
                            1.0f, -1.0f, 1.0f, 0.0f,

                            -1.0f, 1.0f, 0.0f, 1.0f,
                            1.0f, -1.0f, 1.0f, 0.0f,
                            1.0f, 1.0f, 1.0f, 1.0f};
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    // FRAMEBUFFER
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);           // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // QUAD FBO
    unsigned int quadFbo;
    glGenFramebuffers(1, &quadFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, quadFbo);

    unsigned int quadTexture;
    glGenTextures(1, &quadTexture);
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           quadTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // HIST TEXTURE
    unsigned int histFbo;
    glGenFramebuffers(1, &histFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, histFbo);

    unsigned int histTexture;
    glGenTextures(1, &histTexture);
    glBindTexture(GL_TEXTURE_2D, histTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           histTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f, 2.2f, 2.0f),
        glm::vec3(22.3f, 3.3f, -4.0f),
        glm::vec3(-14.0f, 6.0f, -12.0f),
        glm::vec3(10.0f, 50.0f, -3.0f)};

    int frameCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glm::mat4 view = camera.GetViewMatrix();
        // projection matrix
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        glm::mat4 jitteredProj = projection;

        glm::vec2 jitter = getJitterOffset(frameCount % 16); // in [~ -0.5, 0.5]

        // scale jitter to pixel fraction (e.g., 1 pixel in normalized coords)
        float jitterScaleX = 1.0f / SCR_WIDTH;
        float jitterScaleY = 1.0f / SCR_HEIGHT;
        glm::vec2 jitterNDC = glm::vec2((jitter.x - 0.5) * jitterScaleX, (jitter.y - 0.5) * jitterScaleY);

        std::cout << "Jitter " << jitterNDC.x << " - " << jitterNDC.y << std::endl;

        // apply jitter
        jitteredProj[2][0] += jitterNDC.x;
        jitteredProj[2][1] += jitterNDC.y;

        // SPONZA
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.f, 0.0f));
        model = glm::scale(model, glm::vec3(.4f, .4f, .4f));

        // LIGHT FRAMEBUFFER
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_SHADING_RATE_IMAGE_NV);
        createFoveationTexture(posX, posY);
        createTexture(fov_texture);
        uploadFoveationDataToTexture(fov_texture);
        glBindShadingRateImageNV(fov_texture);

        // view matrix
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.Position);
        shader.setMat4("projection", jitteredProj);
        // directional light
        shader.setVec3("dirLight.direction", -0.2f, 10.0f, -0.3f);
        shader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
        shader.setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.6f);
        shader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        shader.setBool("showShading", showShading);

        // SPONZA
        shader.setMat4("model", model);
        sponza.Draw(shader);

        // ON SCREEN FRAMEBUFFER
        glBindFramebuffer(GL_FRAMEBUFFER, quadFbo);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, histTexture);
        screenShader.setInt("screenTexture", 0);
        screenShader.setInt("previousFrame", 1);
        screenShader.setFloat("blendFactor", 0.2);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. Blit result to screen (default framebuffer)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, quadFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // screen
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
                          0, 0, SCR_WIDTH, SCR_HEIGHT,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // 3. Also blit to histTexture (for reuse in next frame)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, histFbo); // now update history
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT,
                          0, 0, SCR_WIDTH, SCR_HEIGHT,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);


        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Back to default

        glDisable(GL_DEPTH_TEST);
        ImGui::Begin("Options");
        ImGui::Checkbox("Shading rate view", &showShading);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        frameCount++;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        isCursorEnabled = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        isCursorEnabled = false;
        firstMouse = true;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    const float sensitivity = 0.3f;

    if (!isCursorEnabled)
    {
        float xoffset_fov = ((xpos / SCR_WIDTH) - posX) * sensitivity;
        float yoffset_fov = ((1 - ypos / SCR_HEIGHT) - posY) * sensitivity;
        posX += xoffset_fov;
        posY += yoffset_fov;
        posX = std::clamp(posX, 0.f, 1.f);
        posY = std::clamp(posY, 0.f, 1.f);
        return;
    }
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed: y ranges bottom to top
    lastX = xpos;
    lastY = ypos;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffest, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void createFoveationTexture(float centerX, float centerY)
{
    const int width = m_shadingRateImageWidth;
    const int height = m_shadingRateImageHeight;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float fx = x / (float)width;
            float fy = y / (float)height;

            float d = std::sqrt((fx - centerX) * (fx - centerX) + (fy - centerY) * (fy - centerY));
            if (d < 0.15f)
            {
                m_shadingRateImageData[x + y * width] = 1;
            }
            else if (d < 0.3f)
            {
                m_shadingRateImageData[x + y * width] = 2;
            }
            else
            {
                m_shadingRateImageData[x + y * width] = 3;
            }
        }
    }
}

void uploadFoveationDataToTexture(GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, m_shadingRateImageWidth, m_shadingRateImageHeight);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_shadingRateImageWidth, m_shadingRateImageHeight, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &m_shadingRateImageData[0]);
}

void setupShadingRatePalette()
{
    GLint palSize;
    glGetIntegerv(GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV, &palSize);
    assert(palSize >= 4);

    GLenum *palette = new GLenum[palSize];

    palette[0] = GL_SHADING_RATE_NO_INVOCATIONS_NV;
    palette[1] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
    palette[2] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
    palette[3] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;

    for (int i = 4; i < palSize; ++i)
    {
        palette[i] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
    }

    glShadingRateImagePaletteNV(0, 0, palSize, palette);
    delete[] palette;
}
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,   // top-right
            1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,  // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 1.0f,   // top-right
            -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
            // front face
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,  // bottom-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,   // top-right
            -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-left
            -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            // left face
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // top-right
            -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // top-right
                                                                      // right face
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,     // top-left
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // bottom-right
            1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    // top-right
            1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // bottom-right
            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,     // top-left
            1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,    // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // bottom-left
            1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // bottom-left
            -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
            -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right
            // top face
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // bottom-right
            1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-right
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // bottom-right
            -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
            -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f   // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << err << std::endl;
    }
}

void createTexture(GLuint &glid)
{
    if (glid)
    {
        glDeleteTextures(1, &glid);
    }
    glGenTextures(1, &glid);
}

glm::vec2 getJitterOffset(int frame)
{
    auto halton = [](int index, int base)
    {
        float f = 1.0f, r = 0.0f;
        while (index > 0)
        {
            f = f / base;
            r = r + f * (index % base);
            index = index / base;
        }
        return r;
    };
    return glm::vec2(halton(frame % 1024, 2), halton(frame % 1024, 3));
}
