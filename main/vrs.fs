#version 460
#extension GL_NV_shading_rate_image : enable
#extension GL_NV_primitive_shading_rate : enable

out vec4 FragColor;

in vec3 color;

void main() {
    int maxCoarse = max(gl_FragmentSizeNV.x, gl_FragmentSizeNV.y);

    if (maxCoarse == 1) {
        FragColor = vec4(1,0,0,1);
    }
    else if (maxCoarse == 2) {
        FragColor = vec4(1,1,0,1);
    }
    else if (maxCoarse == 4) {
        FragColor = vec4(0,1,0,1);
    }
    else {
        FragColor = vec4(1,1,1,1);
    }
    //FragColor = vec4(color, 1.0);
}