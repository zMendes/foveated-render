#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texHigh;
uniform sampler2D texMedium;
uniform sampler2D texLow;

uniform vec2 gaze;

uniform bool showShading;

void main() {
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoords, gaze);

    if (dist < 0.2)
    FragColor = texture(texHigh, TexCoords);// * vec4(1.0, 0.0, 0.0, 1.0);
    else if (dist < 0.5)
    FragColor = texture(texMedium, TexCoords);// * vec4(1.0, 1.0, 0.0, 1.0);
    else
    FragColor = texture(texLow, TexCoords);// * vec4(0.0, 1.0, 0.0, 1.0) ;//vec4(1.0, 1.0, 0.0, 1.0);//

    if (showShading) {
        if (dist <0.2)
            FragColor *= vec4(1.0, 0.0, 0.0, 1.0);
        else if (dist <0.5)
            FragColor *= vec4(1.0, 1.0, 0.0, 1.0);
        else
            FragColor *= vec4(0.0, 1.0, 0.0, 1.0);
    }
}
