#version 460
#extension GL_NV_shading_rate_image : enable
#extension GL_NV_primitive_shading_rate : enable

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform bool showShading;

out vec4 FragColor;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main() {
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    FragColor = vec4(result, 1.0);

    if (showShading) {
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
    }
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0),
        material.shininess);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1,
            texCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1,
            texCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1,
            texCoords));
    return (ambient + diffuse + specular);
}