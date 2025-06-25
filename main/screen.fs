
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 predicted;
uniform vec2 true_gaze;
uniform bool showShading;
void main() {
    FragColor = texture(screenTexture, TexCoords);
    if (showShading) {
        if (distance(TexCoords, predicted) < 0.002)
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        if (distance(TexCoords, true_gaze) < 0.002)
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
}