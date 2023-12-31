#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec3 aTex;

uniform mat4 M;
uniform mat4 lightSpaceMatrix;

void main() {
    gl_Position = lightSpaceMatrix * M * vec4(aPos, 1.0);
}  
