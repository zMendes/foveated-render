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
#include "constants.h"
#include "shadingrate.h"
#include "gaze_data.h"
#include "foveation.h"
#include "utils.h"
#include "gaze_predictor.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>
#include <cmath>
#include <utility>
#include <chrono>
#include <numeric>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void renderScene(Shader &shader, Model model);

float posX = 0.5;
float posY = 0.5;
bool isCursorEnabled = false;

// CAMERA
Camera camera(glm::vec3(-166.95f, 98.45f, -13.21f), glm::vec3(0.0f, 1.0f, 0.0f), 0.89f, -17.04f);
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool showShading = false;

// settings
float diagonal_in_inches = 17.0f;

float diag_px = std::sqrt(SCR_WIDTH * SCR_WIDTH + SCR_HEIGHT * SCR_HEIGHT);
float diag_mm = diagonal_in_inches * 25.4f;
float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;
float near = 0.1f;
float far = 10000.0f;
float INNER_R_DEG = 5.0f;
float MIDDLE_R_DEG = 10.0f;
float INNER_R = angleToNormRadius(INNER_R_DEG, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
float MIDDLE_R = angleToNormRadius(MIDDLE_R_DEG, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);

const float ALPHA = 1.0f;
const float BETA = 0.0f;

int main()
{
    // read gaze seq
    std::vector<Gaze> gazeSeq;
    std::deque<std::array<float, 2>> gaze_history;
    glm::vec2 predicted;
    std::pair<float, float> predicted_deg;

    loadGazeSequence("gazeseq.txt", gazeSeq);

   GazePredictor predictor("/home/loe/Downloads/saccade_predictor.onnx");


    std::vector<float> iouVector;
    std::vector<float> circleVector;

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
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1; // Correct return here, exiting if GLAD fails
    }

    initializeFoveation();
    using clock = std::chrono::high_resolution_clock;
    using ms = std::chrono::milliseconds;

    // OPENGL STATE
    glEnable(GL_DEPTH_TEST);
    glEnable(NVShadingRate::IMAGE);

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
    float quadVertices[] = {
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
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int count = 0;
    const int gazeHz = 33;                // target rate in Hz
    const int intervalMs = 1000 / gazeHz; // ~30ms
    auto lastUpdate = clock::now();
    glm::vec2 normGaze;
    bool isNewGaze = false;
    float pastError = 0.0f;
    float lastRadius = INNER_R;
    int totalFrameCount = 0;
    float avgpredError = 0.0f;

    GLuint queryFragment;
    glGenQueries(1, &queryFragment);
    //  glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, queryFragment);
    float averageInvocation = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        if (count >= (int)gazeSeq.size())
            break;

        isNewGaze = false;
        processInput(window);

        // ========== GET GAZE POINTS ==========
        auto now = clock::now();
        if (std::chrono::duration_cast<ms>(now - lastUpdate).count() >= intervalMs)
        {
            auto [gaze_deg_x, gaze_deg_y] = pixelsToDegreesFromNormalized(gazeSeq[count].x, gazeSeq[count].y);

            normGaze = glm::vec2(
                (gazeSeq[count].x + 1.0f) / 2.0f,
                (gazeSeq[count].y + 1.0f) / 2.0f);

            lastUpdate = now;
            count++;
            gaze_history.push_back({gaze_deg_x, gaze_deg_y});
            isNewGaze = true;
        }

        if (gaze_history.size() > 10)
            gaze_history.pop_front();

        if (gaze_history.size() == 10 && isNewGaze)
        {
            iouVector.push_back(computeCircleIoU(normGaze, INNER_R, predicted, lastRadius));
            circleVector.push_back(computeCircleCoverage(normGaze, INNER_R, predicted, lastRadius));
            auto &curr = gaze_history.back();

            float dx = predicted_deg.first - curr[0];
            float dy = predicted_deg.second - curr[1];
            float raw_error = std::sqrt(dx * dx + dy * dy);
            avgpredError += raw_error;
            float deltaError = raw_error - pastError;

            std::vector<float> input_tensor_values;
            std::pair<float, float> predicted_deg = predictor.predict(gaze_history);

            predicted = gazeAngleToNorm(predicted_deg.first, predicted_deg.second);
            float normRawE = angleToNormRadius(raw_error, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            float normDeltaE = angleToNormRadius(deltaError, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            // std::cout << "E_i" << raw_error << " DeltaError " << deltaError << std::endl;
            lastRadius = updateFoveationTexture(predicted, normRawE, normDeltaE);
            // lastRadius = updateFoveationTexture(predicted, 0.0, 0.0);
            lastRadius = updateFoveationTexture(predicted, angleToNormRadius(2.58, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT), 0.0);

            pastError = raw_error;
        }

        // ========== RENDER PASS ==========s
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.f, 0.0f));
        model = glm::scale(model, glm::vec3(0.20f));

        shader.use();
        glEnable(NVShadingRate::IMAGE);
        bindFoveationTexture();

        shader.setMat4("view", view);
        shader.setMat4("model", model);
        shader.setVec3("viewPos", camera.Position);
        shader.setMat4("projection", projection);
        // directional light
        shader.setVec3("dirLight.direction", -0.2f, -10.0f, -0.3f);
        shader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
        shader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
        shader.setVec3("dirLight.specular", 0.7f, 0.7f, 0.7f);
        shader.setBool("showShading", showShading);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, queryFragment);
        renderScene(shader, sponza);
        glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);

        GLuint fragmentsGenerated;
        glGetQueryObjectuiv(queryFragment, GL_QUERY_RESULT, &fragmentsGenerated);
        averageInvocation += fragmentsGenerated;
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        screenShader.setBool("showShading", showShading);
        screenShader.setInt("screenTexture", 0);
        screenShader.setVec2("predicted", predicted);
        if (gaze_history.size() > 0)
            screenShader.setVec2("true_gaze", normGaze);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // dt
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        while ((err = glGetError()) != GL_NO_ERROR)
            std::cout << "After process: GL Error " << err << std::endl;
        totalFrameCount++;
    }

    float mean = std::reduce(iouVector.begin(), iouVector.end()) / iouVector.size();
    float meanCircle = std::reduce(circleVector.begin(), circleVector.end()) / circleVector.size();
    std::cout << "Avg frags: " << averageInvocation / totalFrameCount << std::endl;
    std::cout << "Average IOU: " << mean << std::endl;
    std::cout << "Average Coverage: " << meanCircle << std::endl;
    std::cout << "Average pred error: " << avgpredError / gazeSeq.size();

    glfwTerminate();
    return 0;
}

void renderScene(Shader &shader, Model model)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    shader.use();
    glActiveTexture(GL_TEXTURE0);

    model.Draw(shader);
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
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        showShading = !showShading;
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
    float yoffset = lastY - ypos;
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