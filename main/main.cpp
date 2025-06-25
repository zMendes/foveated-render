#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include "shader.h"
#include "camera.h"
#include "stb_image.h"
#include "model.h"
#include "constants.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>
#include <cmath>
#include <utility>
#include <chrono>
#include <numeric>

typedef void(APIENTRYP PFNGLBINDSHADINGRATEIMAGENVPROC)(GLuint texture);
PFNGLBINDSHADINGRATEIMAGENVPROC glBindShadingRateImageNV = nullptr;

typedef void(APIENTRYP PFNGLSHADINGRATEIMAGEPALETTENVPROC)(GLuint viewport, GLuint first, GLsizei count, const GLenum *rates);
PFNGLSHADINGRATEIMAGEPALETTENVPROC glShadingRateImagePaletteNV = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
float createFoveationTexture(glm::vec2 point, float error, float deltaError);
void uploadFoveationDataToTexture(GLuint texture);
void setupShadingRatePalette();
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);
void renderCube();
void createTexture(GLuint &glid);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void renderScene(Shader &shader, Model model);
float angleToNormRadius(float deg, float diagInInches, float distMM, int scrWidth, int scrHeight);
glm::vec2 gazeAngleToNorm(float predicted_x_deg, float predicted_y_deg);
std::pair<float, float> pixelsToDegreesFromNormalized(float norm_x, float norm_y);
float computeCircleIoU(glm::vec2 center1, float r1, glm::vec2 center2, float r2);
float computeCircleCoverage(glm::vec2 center1, float r1, glm::vec2 center2, float r2);

typedef struct
{
    double timestamp;
    float x;
    float y;
} Gaze;

void loadGazeSequence(std::string path, std::vector<Gaze> &gazeSeq);

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

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
float SCR_WIDTH_MM = diag_mm * (SCR_WIDTH / diag_px);
float SCR_HEIGHT_MM = diag_mm * (SCR_HEIGHT / diag_px);
float DIST_MM = 600.0f;
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

    // CONFIG MODEL
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "saccade_predictor");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    const char *model_path = "/home/loe/Downloads/saccade_predictor.onnx";
    Ort::Session session(env, model_path, session_options);
    std::array<int64_t, 3> input_shape = {1, 10, 2};
    const char *input_names[] = {"input"};
    const char *output_names[] = {"output"};

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
    glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)glfwGetProcAddress("glShadingRateImagePaletteNV");

    using clock = std::chrono::high_resolution_clock;
    using ms = std::chrono::milliseconds;

    // OPENGL STATE
    glEnable(GL_DEPTH_TEST);
    //glEnable(NVShadingRate::IMAGE);

    glGetIntegerv(NVShadingRate::TEXEL_HEIGHT, &m_shadingRateImageTexelHeight);
    glGetIntegerv(NVShadingRate::TEXEL_WIDTH, &m_shadingRateImageTexelWidth);

    m_shadingRateImageWidth = (SCR_WIDTH + m_shadingRateImageTexelWidth - 1) / m_shadingRateImageTexelWidth;
    m_shadingRateImageHeight = (SCR_HEIGHT + m_shadingRateImageTexelHeight - 1) / m_shadingRateImageTexelHeight;
    m_shadingRateImageData.resize(m_shadingRateImageWidth * m_shadingRateImageHeight);

    createTexture(fov_texture);
    setupShadingRatePalette();
    createFoveationTexture(glm::vec2(0.5, 0.5), 0.0, 0.0);

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

    // GLuint queryFragment;
    // glGenQueries(1, &queryFragment);
    //  glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, queryFragment);
    float averageInvocation = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        if (count >= (int) gazeSeq.size())
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

        // ========== TIME PROCESSING AND INFERENCE ==========
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
            for (const auto &pt : gaze_history)
            {
                input_tensor_values.push_back(pt[0]);
                input_tensor_values.push_back(pt[1]);
            }

            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

            auto output_tensors = session.Run(Ort::RunOptions{nullptr},
                                              input_names, &input_tensor, 1,
                                              output_names, 1);

            float *output = output_tensors.front().GetTensorMutableData<float>();
            predicted_deg = {output[0], output[1]};
            predicted = gazeAngleToNorm(predicted_deg.first, predicted_deg.second);
            float normRawE = angleToNormRadius(raw_error, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            float normDeltaE = angleToNormRadius(deltaError, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT);
            // std::cout << "E_i" << raw_error << " DeltaError " << deltaError << std::endl;
            lastRadius = createFoveationTexture(predicted, normRawE, normDeltaE);
            //lastRadius = createFoveationTexture(predicted, 0.0, 0.0);
            lastRadius = createFoveationTexture(predicted, angleToNormRadius(2.58, diagonal_in_inches, DIST_MM, SCR_WIDTH, SCR_HEIGHT), 0.0);

            pastError = raw_error;
        }

        // ========== RENDER PASS ==========s
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.f, 0.0f));
        model = glm::scale(model, glm::vec3(0.20f));

        shader.use();
        glEnable(NVShadingRate::IMAGE);
        createTexture(fov_texture);
        uploadFoveationDataToTexture(fov_texture);
        glBindShadingRateImageNV(fov_texture);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", model);
        shader.setVec3("viewPos", camera.Position);
        // directional light
        shader.setVec3("dirLight.direction", -0.2f, -10.0f, -0.3f);
        shader.setVec3("dirLight.ambient", 0.4f, 0.4f, 0.4f);
        shader.setVec3("dirLight.diffuse", 0.5f, 0.5f, 0.5f);
        shader.setVec3("dirLight.specular", 0.7f, 0.7f, 0.7f);
        shader.setMat4("view", view);
        shader.setMat4("model", model);
        shader.setVec3("viewPos", camera.Position);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        shader.setBool("showShading", showShading);
        GLuint queryFragment;
        glGenQueries(1, &queryFragment);
        glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, queryFragment);
        renderScene(shader, sponza);
        glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);

        GLuint fragmentsGenerated;
        glGetQueryObjectuiv(queryFragment, GL_QUERY_RESULT, &fragmentsGenerated);
        //std::cout << "Fragment invocations this frame: " << fragmentsGenerated << std::endl;
        averageInvocation+= fragmentsGenerated;
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
    std::cout << "Avg frags: " << averageInvocation/totalFrameCount <<std::endl;
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

float createFoveationTexture(glm::vec2 point, float error, float deltaError)
{
    float centerX = point[0];
    float centerY = point[1];
    const int width = m_shadingRateImageWidth;
    const int height = m_shadingRateImageHeight;

    float dynamicError = ALPHA * error + BETA * deltaError;

    float innerR = INNER_R + dynamicError;
    float middleR = MIDDLE_R + dynamicError;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float fx = x / (float)width;
            float fy = y / (float)height;

            float d = std::sqrt((fx - centerX) * (fx - centerX) + (fy - centerY) * (fy - centerY));
            if (d < (innerR))
            {
                m_shadingRateImageData[x + y * width] = 1;
            }
            else if (d < (middleR))
            {
                m_shadingRateImageData[x + y * width] = 2;
            }
            else
            {
                m_shadingRateImageData[x + y * width] = 3;
            }
        }
    }
    return innerR;
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
    glGetIntegerv(NVShadingRate::PALETTE_SIZE, &palSize);
    assert(palSize >= 4);

    GLenum *palette = new GLenum[palSize];

    palette[0] = NVShadingRate::NO_INVOCATIONS;
    palette[1] = NVShadingRate::ONE_INVOCATION_PER_PIXEL;
    palette[2] = NVShadingRate::ONE_INVOCATION_PER_2X2;
    palette[3] = NVShadingRate::ONE_INVOCATION_PER_4X4;

    for (int i = 4; i < palSize; ++i)
    {
        palette[i] = NVShadingRate::ONE_INVOCATION_PER_PIXEL;
    }

    glShadingRateImagePaletteNV(0, 0, palSize, palette);
    delete[] palette;
}

void createTexture(GLuint &glid)
{
    if (glid)
    {
        glDeleteTextures(1, &glid);
    }
    glGenTextures(1, &glid);
}

void loadGazeSequence(std::string path, std::vector<Gaze> &gazeSeq)
{
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        double t;
        float x, y;
        char comma;
        ss >> t >> comma >> x >> comma >> y;
        gazeSeq.push_back({t, x, y});
    }
};

float angleToNormRadius(float deg, float diagInInches, float distMM, int scrWidth, int scrHeight)
{
    float diagPx = std::sqrt(scrWidth * scrWidth + scrHeight * scrHeight);
    float diagMM = diagInInches * 25.4f;
    float pixelSizeMM = diagMM / diagPx;

    float rad = glm::radians(deg);
    float sizeMM = 2.0f * distMM * std::tan(rad / 2.0f);
    float sizePx = sizeMM / pixelSizeMM;

    float screenSizePx = std::min(scrWidth, scrHeight);
    return (sizePx / screenSizePx) / 2.0f; // radius
}

float deg2rad(float deg)
{
    return deg * 3.1415 / 180.0f;
}

glm::vec2 gazeAngleToNorm(float predicted_x_deg, float predicted_y_deg)
{

    float x_rad = deg2rad(predicted_x_deg);
    float y_rad = deg2rad(predicted_y_deg);

    float x_mm = std::tan(x_rad) * DIST_MM;
    float y_mm = std::tan(y_rad) * DIST_MM;

    float px_x = (x_mm / SCR_WIDTH_MM) * SCR_WIDTH + SCR_WIDTH / 2.0f;
    float px_y = (y_mm / SCR_HEIGHT_MM) * SCR_HEIGHT + SCR_HEIGHT / 2.0f;

    float norm_x = px_x / SCR_WIDTH;
    float norm_y = px_y / SCR_HEIGHT;

    return glm::vec2(norm_x, norm_y);
}
std::pair<float, float> pixelsToDegreesFromNormalized(float norm_x, float norm_y)
{
    float px_x = norm_x * SCR_WIDTH / 2.0;
    float px_y = norm_y * SCR_HEIGHT / 2.0;

    float px_per_mm_x = SCR_WIDTH / SCR_WIDTH_MM;
    float px_per_mm_y = SCR_HEIGHT / SCR_HEIGHT_MM;

    float dx_mm = (px_x) / px_per_mm_x;
    float dy_mm = (px_y) / px_per_mm_y;

    float deg_x = std::atan2(dx_mm, DIST_MM) * (180.0f / static_cast<float>(3.1415));
    float deg_y = std::atan2(dy_mm, DIST_MM) * (180.0f / static_cast<float>(3.1415));

    return {deg_x, deg_y};
}

float computeCircleIoU(glm::vec2 center1, float r1, glm::vec2 center2, float r2)
{
    float d = glm::distance(center1, center2);

    // No overlap
    if (d >= r1 + r2)
        return 0.0f;

    // One circle completely inside the other
    if (d <= std::abs(r1 - r2))
    {
        float min_r = std::min(r1, r2);
        float max_r = std::max(r1, r2);
        return (min_r * min_r) / (max_r * max_r);
    }

    float r1_sq = r1 * r1;
    float r2_sq = r2 * r2;

    float alpha = std::acos((r1_sq + d * d - r2_sq) / (2.0f * r1 * d));
    float beta = std::acos((r2_sq + d * d - r1_sq) / (2.0f * r2 * d));

    float intersection_area =
        r1_sq * alpha + r2_sq * beta -
        0.5f * std::sqrt(
                   (-d + r1 + r2) *
                   (d + r1 - r2) *
                   (d - r1 + r2) *
                   (d + r1 + r2));

    float union_area = M_PI * (r1_sq + r2_sq) - intersection_area;

    return intersection_area / union_area;
}

float computeCircleCoverage(glm::vec2 center1, float r1, glm::vec2 center2, float r2)
{
    float d = glm::distance(center1, center2);

    // No overlap
    if (d >= r1 + r2)
        return 0.0f;

    // Circle 1 fully inside Circle 2
    if (d <= std::abs(r2 - r1) && r2 > r1)
        return 1.0f;

    // Circle 2 fully inside Circle 1
    if (d <= std::abs(r2 - r1) && r1 > r2)
        return (M_PI * r2 * r2) / (M_PI * r1 * r1);

    float r1_sq = r1 * r1;
    float r2_sq = r2 * r2;

    float alpha = std::acos((r1_sq + d * d - r2_sq) / (2.0f * r1 * d));
    float beta = std::acos((r2_sq + d * d - r1_sq) / (2.0f * r2 * d));

    float intersection_area =
        r1_sq * alpha + r2_sq * beta -
        0.5f * std::sqrt(
                   (-d + r1 + r2) *
                   (d + r1 - r2) *
                   (d - r1 + r2) *
                   (d + r1 + r2));

    float circle1_area = M_PI * r1_sq;

    return intersection_area / circle1_area;
}
