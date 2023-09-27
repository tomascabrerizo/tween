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

    gl_Position = projection * view * model * vec4(aVert, 1.0);
} 
