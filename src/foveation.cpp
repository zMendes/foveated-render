#include "foveation.h"
#include "constants.h"
#include "shadingrate.h"
#include <cmath>
#include <cassert>

#include <iostream>

typedef void(APIENTRYP PFNGLBINDSHADINGRATEIMAGENVPROC)(GLuint texture);
PFNGLBINDSHADINGRATEIMAGENVPROC glBindShadingRateImageNV = nullptr;

typedef void(APIENTRYP PFNGLSHADINGRATEIMAGEPALETTENVPROC)(GLuint viewport, GLuint first, GLsizei count, const GLenum *rates);
PFNGLSHADINGRATEIMAGEPALETTENVPROC glShadingRateImagePaletteNV = nullptr;

// Internal variables (file-local static)
static GLuint foveationTexture = 0;
static std::vector<uint8_t> shadingRateImageData;
static uint32_t shadingRateImageWidth = 0;
static uint32_t shadingRateImageHeight = 0;
static GLint shadingRateImageTexelWidth = 0;
static GLint shadingRateImageTexelHeight = 0;

// Constants for foveation radii and parameters (could also be configurable)
static const float INNER_RADIUS = 0.03f; // example normalized value
static const float MIDDLE_RADIUS = 0.06f;

static const float ALPHA = 1.0f; // weight for error
static const float BETA = 0.0f;  // weight for delta error

void loadNVShadingRateImageFunctions()
{
    glBindShadingRateImageNV = (PFNGLBINDSHADINGRATEIMAGENVPROC)glfwGetProcAddress("glBindShadingRateImageNV");
    glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)glfwGetProcAddress("glShadingRateImagePaletteNV");
}

void createTexture(GLuint &texture)
{
    if (texture)
        glDeleteTextures(1, &texture);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, foveationTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, shadingRateImageWidth, shadingRateImageHeight);
}

void setupShadingRatePalette()
{
    GLint palSize = 0;
    glGetIntegerv(NVShadingRate::PALETTE_SIZE, &palSize);
    assert(palSize >= 4);

    std::vector<GLenum> palette(palSize);

    palette[0] = NVShadingRate::NO_INVOCATIONS;
    palette[1] = NVShadingRate::ONE_INVOCATION_PER_PIXEL;
    palette[2] = NVShadingRate::ONE_INVOCATION_PER_2X2;
    palette[3] = NVShadingRate::ONE_INVOCATION_PER_4X4;

    for (int i = 4; i < palSize; ++i)
        palette[i] = NVShadingRate::ONE_INVOCATION_PER_PIXEL;

    glShadingRateImagePaletteNV(0, 0, palSize, palette.data());
}

void initializeFoveation()
{
    loadNVShadingRateImageFunctions();
    glGetIntegerv(NVShadingRate::TEXEL_HEIGHT, &shadingRateImageTexelHeight);
    glGetIntegerv(NVShadingRate::TEXEL_WIDTH, &shadingRateImageTexelWidth);

    shadingRateImageWidth = (SCR_WIDTH + shadingRateImageTexelWidth - 1) / shadingRateImageTexelWidth;
    shadingRateImageHeight = (SCR_HEIGHT + shadingRateImageTexelHeight - 1) / shadingRateImageTexelHeight;
    shadingRateImageData.resize(shadingRateImageWidth * shadingRateImageHeight);

    createTexture(foveationTexture);
    setupShadingRatePalette();
}

float updateFoveationTexture(const glm::vec2 &point, float error, float deltaError)
{

    float centerX = point.x;
    float centerY = point.y;

    float dynamicError = ALPHA * error + BETA * deltaError;

    float innerR = INNER_RADIUS + dynamicError;
    float middleR = MIDDLE_RADIUS + dynamicError;

    for (uint32_t y = 0; y < shadingRateImageHeight; ++y)
    {
        for (uint32_t x = 0; x < shadingRateImageWidth; ++x)
        {
            float fx = x / static_cast<float>(shadingRateImageWidth);
            float fy = y / static_cast<float>(shadingRateImageHeight);

            float dist = std::sqrt((fx - centerX) * (fx - centerX) + (fy - centerY) * (fy - centerY));
            if (dist < innerR)
                shadingRateImageData[x + y * shadingRateImageWidth] = 1;
            else if (dist < middleR)
                shadingRateImageData[x + y * shadingRateImageWidth] = 2;
            else
                shadingRateImageData[x + y * shadingRateImageWidth] = 3;
        }
    }


    glBindTexture(GL_TEXTURE_2D, foveationTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, shadingRateImageWidth, shadingRateImageHeight,
                    GL_RED_INTEGER, GL_UNSIGNED_BYTE, shadingRateImageData.data());

    return innerR;
}

void bindFoveationTexture()
{
    glBindShadingRateImageNV(foveationTexture);
}

void cleanupFoveation()
{
    if (foveationTexture)
        glDeleteTextures(1, &foveationTexture);
    foveationTexture = 0;
    shadingRateImageData.clear();
}
