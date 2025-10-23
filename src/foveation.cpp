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

void Foveation::loadNVShadingRateImageFunctions()
{
    glBindShadingRateImageNV = (PFNGLBINDSHADINGRATEIMAGENVPROC)glfwGetProcAddress("glBindShadingRateImageNV");
    glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)glfwGetProcAddress("glShadingRateImagePaletteNV");
}

void Foveation::createTexture(GLuint &texture)
{
    if (texture)
        glDeleteTextures(1, &texture);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, foveationTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, shadingRateImageWidth, shadingRateImageHeight);
}

void Foveation::setupShadingRatePalette()
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

Foveation::Foveation(FoveationMode mode, float inner_r, float middle_r)
{
    MODE = mode;
    INNER_R = inner_r;
    MIDDLE_R = middle_r;
    if (MODE != FoveationMode::NO)
        glEnable(NVShadingRate::IMAGE);
    loadNVShadingRateImageFunctions();
    glGetIntegerv(NVShadingRate::TEXEL_HEIGHT, &shadingRateImageTexelHeight);
    glGetIntegerv(NVShadingRate::TEXEL_WIDTH, &shadingRateImageTexelWidth);

    shadingRateImageWidth = (SCR_WIDTH + shadingRateImageTexelWidth - 1) / shadingRateImageTexelWidth;
    shadingRateImageHeight = (SCR_HEIGHT + shadingRateImageTexelHeight - 1) / shadingRateImageTexelHeight;
    shadingRateImageData.resize(shadingRateImageWidth * shadingRateImageHeight);

    createTexture(foveationTexture);
    setupShadingRatePalette();
}

float Foveation::updateFoveationTexture(const glm::vec2 &point, float error, float deltaError)
{

    float centerX = point.x;
    float centerY = point.y;

    float innerR = INNER_R;
    float middleR = MIDDLE_R;

    if (MODE == FoveationMode::ADAPTIVE)
    {
        float dynamicError = ALPHA * error + BETA * deltaError;
        innerR += dynamicError;
        middleR += dynamicError;
    }

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

void Foveation::bindFoveationTexture()
{
    glBindShadingRateImageNV(foveationTexture);
}

void Foveation::cleanupFoveation()
{
    if (foveationTexture)
        glDeleteTextures(1, &foveationTexture);
    foveationTexture = 0;
    shadingRateImageData.clear();
}
