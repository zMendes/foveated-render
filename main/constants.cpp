#include "constants.h"
#include <cmath>

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

const float DIAGONAL_IN_INCHES = 17.0f;

const float DIST_MM = 600.0f;

// Calculate diagonal pixels and physical screen dimensions in mm
const float diag_px = std::sqrt(SCR_WIDTH * SCR_WIDTH + SCR_HEIGHT * SCR_HEIGHT);
const float diag_mm = DIAGONAL_IN_INCHES * 25.4f;

const float SCR_WIDTH_MM = diag_mm * (SCR_WIDTH / diag_px);
const float SCR_HEIGHT_MM = diag_mm * (SCR_HEIGHT / diag_px);
