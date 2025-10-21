#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"
#include "stb_image.h"
#include "model.h"
#include "constants.h"
#include "shadingrate.h"
#include "foveation.h"
#include "utils.h"
#include "gaze_predictor.h"
#include "stb_image_write.h"
#include "video_encoder.h"
#include "gaze_sequence.h"

#include <vector>
#include <deque>
#include <chrono>
#include <numeric>

GLFWwindow *initGLFW();
bool initGLAD();
void initQuad();
void initFramebuffers();
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void renderScene(Shader &shader, Model model);
void checkGLError(std::string);
void updateGazeSeq();
std::pair<float, float> computeError(std::pair<float, float> predicted_deg, std::array<float, 2> current, float pastError);
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

// Object stuff
unsigned int quadVAO;
unsigned int fbo;
unsigned int texture;

// GAZE STUFF
std::vector<Gaze> gazeSeq;
std::deque<std::array<float, 2>> gaze_history;
glm::vec2 predicted;
std::pair<float, float> predicted_deg;
bool useGazeSequence = false;
glm::vec2 final;

int main()
{
    std::cout << "Initializing..." << std::endl;
    // Defining normalize radius for foveated rendering
    float INNER_R = angleToNormRadius(INNER_R_DEG, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
    float MIDDLE_R = angleToNormRadius(MIDDLE_R_DEG, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);

    GazePredictor predictor("/home/loe/Downloads/saccade_predictor.onnx");

    GazeSequence gazeSeqq("/home/loe/foveaflex/gaze_predictor/GazeBase_v2_0/Round_1/Subject_1001/S1/S1_Random_Saccades/S_1001_S1_RAN.csv", 1000.0, 60.0);

    GLFWwindow *window = initGLFW();
    if (!window)
        return -1;

    if (!initGLAD())
        return -1;

    initializeFoveation();
    using clock = std::chrono::high_resolution_clock;

    // OPENGL STATE
    glEnable(GL_DEPTH_TEST);
    glEnable(NVShadingRate::IMAGE);

    Shader shader("../shaders/vrs.vs", "../shaders/vrs.fs");
    Shader screenShader("../shaders/screen.vs", "../shaders/screen.fs");
    shader.use();

    std::string path = "../models/sponza/sponza.obj";
    Model sponza(path);

    checkGLError("Before main Loop");

    initQuad();
    initFramebuffers();

    auto init = clock::now();
    double elapsed;

    std::pair<float, float> predicted_deg;
    float pastError = 0.0f;
    int totalFrameCount = 0;
    float avgpredError = 0.0f;

    GLuint queryFragment;
    GLuint fragmentsGenerated;
    glGenQueries(1, &queryFragment);
    float averageInvocation = 0.0f;

    VideoEncoder encoder("output.mp4", SCR_WIDTH, SCR_HEIGHT, 20);

    while (!glfwWindowShouldClose(window))
    {
        auto now = clock::now();
        elapsed = std::chrono::duration<double, std::milli>(now - init).count();
        if (elapsed > gazeSeqq.getLastTimestamp())
            break;

        processInput(window);

        // ========== GET GAZE POINTS ==========
        Gazes lastGaze = gazeSeqq.getLatestGaze(elapsed);
        float gaze_deg_x = lastGaze.x;
        float gaze_deg_y = lastGaze.y; // pixelsToDegreesFromNormalized(lastGaze.x, lastGaze.y);
        final = gazeAngleToNorm(gaze_deg_x, gaze_deg_y);

        gaze_history.push_back({gaze_deg_x, gaze_deg_y});
        if (gaze_history.size() > 10)
            gaze_history.pop_front();

        if (gaze_history.size() == 10)
        {
            auto [rawError, deltaError] = computeError(predicted_deg, gaze_history.back(), pastError);
            predicted_deg = predictor.predict(gaze_history);
            predicted = gazeAngleToNorm(predicted_deg.first, predicted_deg.second);

            // normalize to screen space
            float normRawE = angleToNormRadius(rawError, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            float normDeltaE = angleToNormRadius(deltaError, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            avgpredError += rawError;

            // lastRadius = updateFoveationTexture(INNER_R, MIDDLE_R, glm::vec2(0.5f, 0.5f), normRawE, normDeltaE);
            //  lastRadius = updateFoveationTexture(INNER_R, MIDDLE_R, predicted, 0.0f, 0.0f);

            updateFoveationTexture(INNER_R, MIDDLE_R, predicted, normRawE, normDeltaE);
            pastError = rawError;
        }

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

        glGetQueryObjectuiv(queryFragment, GL_QUERY_RESULT, &fragmentsGenerated);
        averageInvocation += fragmentsGenerated;
        glDisable(GL_DEPTH_TEST);
        stbi_flip_vertically_on_write(1);
        unsigned char *pixelData = (unsigned char *)malloc(SCR_WIDTH * SCR_HEIGHT * 3);

        glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
        // flip vertically
        for (int y = 0; y < static_cast<int>(SCR_HEIGHT) / 2; y++)
        {
            uint8_t *rowTop = pixelData + y * SCR_WIDTH * 3;
            uint8_t *rowBottom = pixelData + (SCR_HEIGHT - 1 - y) * SCR_WIDTH * 3;
            for (int x = 0; x < static_cast<int>(SCR_WIDTH) * 3; x++)
            {
                std::swap(rowTop[x], rowBottom[x]);
            }
        }
        encoder.addFrame(pixelData);
        free(pixelData);

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
            screenShader.setVec2("true_gaze", final);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // dt
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        checkGLError("End Loop: ");
        totalFrameCount++;
    }

    std::cout << "Avg frags: " << averageInvocation / totalFrameCount << std::endl;
    std::cout << "Average pred error: " << avgpredError / totalFrameCount;

    glfwTerminate();
    encoder.finish();
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

void checkGLError(std::string message)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        std::cout << message << err << std::endl;
}

GLFWwindow *initGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *win = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VRS", nullptr, nullptr);
    if (!win)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetScrollCallback(win, scroll_callback);

    return win;
}

bool initGLAD()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    return true;
}

void initQuad()
{
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};
    unsigned int quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
}

void initFramebuffers()
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::pair<float, float> computeError(std::pair<float, float> predicted_deg, std::array<float, 2> current, float pastError)
{

    float dx = predicted_deg.first - current[0];
    float dy = predicted_deg.second - current[1];

    float raw_error = std::sqrt(dx * dx + dy * dy);
    float deltaError = raw_error - pastError;

    return {raw_error, deltaError};
}
