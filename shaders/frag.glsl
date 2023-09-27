#version 330 core

out vec4 FragColor;

in vec2 uv;
in vec3 color;

uniform sampler2D diffuse; 

void main() {

    FragColor = texture(diffuse, uv) * vec4(color, 1.0);

}
