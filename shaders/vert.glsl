#version 330 core

layout (location = 0) in vec3 aVert;
layout (location = 1) in vec2 aUvs;
layout (location = 2) in vec3 aColor;

layout (location = 3) in ivec4 aBonesIds;
layout (location = 4) in vec4  aWeigths;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 bone_matrix[MAX_BONES];

out vec2 uv;
out vec3 color;

void main() {
    
    uv = aUvs;
    color = aColor;

    vec4 total_position = vec4(0.0);
    for(int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if(aBonesIds[i] == -1) {
            continue;
        }
        if(aBonesIds[i] >= MAX_BONES) {
            total_position = vec4(aVert, 1.0);
            break;
        }
        vec4 local_position = bone_matrix[aBonesIds[i]] * vec4(aVert, 1.0);
        total_position += local_position * aWeigths[i];
    }

    gl_Position = projection * view * model * total_position;
} 
