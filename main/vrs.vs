#version 460 core

#extension GL_NV_primitive_shading_rate: require
#extension GL_NV_viewport_array2 : enable

in layout(location=0) vec3 aPos;
in layout(location=1) vec3 aNormal;
in layout(location=2) vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragPos;
out vec3 color;

void main() {
    fragPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    color = aColor;
}