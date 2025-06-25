#ifndef FOVEATION_H
#define FOVEATION_H

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void loadNVShadingRateImageFunctions();

// Initialize foveation system: create texture, setup palette, etc.
void initializeFoveation();

// Update the foveation texture based on gaze position and error metrics
// Returns the inner radius used (normalized screen coords)
float updateFoveationTexture(const glm::vec2& gazePos, float error, float deltaError);

// Bind the shading rate image texture for use
void bindFoveationTexture();

// Cleanup any resources
void cleanupFoveation();
#endif // FOVEATION_H
