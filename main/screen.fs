#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texHigh;
uniform sampler2D texMedium;
uniform sampler2D texLow;

uniform vec2 gaze;
uniform vec2 predicted;
uniform bool showShading;
uniform float innerR;

void main() {
vec2 uvHigh = (TexCoords - (gaze - vec2(innerR))) / (2.0 * innerR);
    uvHigh = clamp(uvHigh, 0.0, 1.0);

    float dist = distance(TexCoords, gaze);

    if (dist < 0.2)
        FragColor = texture(texHigh, uvHigh);
    else if (dist < 0.5)
        FragColor = texture(texMedium, TexCoords);
    else
        FragColor = texture(texLow, TexCoords);

    if (showShading) {
        if (dist < 0.2)
            FragColor *= vec4(1.0, 0.0, 0.0, 1.0);
        else if (dist < 0.5)
            FragColor *= vec4(1.0, 1.0, 0.0, 1.0);
        else
            FragColor *= vec4(0.0, 1.0, 0.0, 1.0);

        float predicted_dist = distance(TexCoords, predicted);
        if (predicted_dist < 0.001)
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
}