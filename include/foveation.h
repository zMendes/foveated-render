#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class FoveationMode
{
    STATIC,
    ADAPTIVE,
    PADDED,
    NO,
};

class Foveation
{
public:
    Foveation(FoveationMode mode, float inner_r, float middle_r);
    void initializeFoveation();
    float updateFoveationTexture(const glm::vec2 &point, float error, float deltaError);
    void bindFoveationTexture();
    void cleanupFoveation();

private:
    FoveationMode MODE;
    float INNER_R;
    float MIDDLE_R;
    GLuint foveationTexture = 0;
    std::vector<uint8_t> shadingRateImageData;
    uint32_t shadingRateImageWidth = 0;
    uint32_t shadingRateImageHeight = 0;
    GLint shadingRateImageTexelWidth = 0;
    GLint shadingRateImageTexelHeight = 0;
    void loadNVShadingRateImageFunctions();
    void createTexture(GLuint &texture);
    void setupShadingRatePalette();
};