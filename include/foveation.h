#ifndef FOVEATION_H
#define FOVEATION_H

#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void initializeFoveation();

float updateFoveationTexture(float INNER_R, float MIDDLE_R, const glm::vec2 &point, float error, float deltaError);

void bindFoveationTexture();

void cleanupFoveation();
#endif // FOVEATION_H
