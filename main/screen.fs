
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D previousFrame;
uniform float blendFactor; // e.g., 0.1 for fast convergence, 0.05 for smoother

void main() {
    vec4 current = texture(screenTexture, TexCoords);
    vec4 history = texture(previousFrame, TexCoords);
    FragColor = mix(current, history, blendFactor);
}