#include "constants.h"
#include <cmath>

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const float DIAGONAL_IN_INCHES = 24.0f;

const float DIST_MM = 600.0f;

const float diag_px = std::sqrt(SCR_WIDTH * SCR_WIDTH + SCR_HEIGHT * SCR_HEIGHT);
const float diag_mm = DIAGONAL_IN_INCHES * 25.4f;

const float SCR_WIDTH_MM = diag_mm * (SCR_WIDTH / diag_px);
const float SCR_HEIGHT_MM = diag_mm * (SCR_HEIGHT / diag_px);

float diagonal_in_inches = 24.0f;
float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;
float near = 0.1f;
float far = 10000.0f;
float INNER_R_DEG = 7.5f;
float MIDDLE_R_DEG = 12.0f;

float AVERAGE_PREDICTION_ERROR = 1.5f;

float ALPHA = 0.2f;
float BETA = 0.5;
