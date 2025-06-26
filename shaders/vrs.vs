#version 460 core

in layout(location=0) vec3 aPos;
in layout(location=1) vec3 aNormal;
in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normal;
out vec3 fragPos;
out vec2 texCoords;

void main() {
    fragPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    normal  = mat3(transpose(inverse(model))) * aNormal;
    texCoords = aTexCoords;
}