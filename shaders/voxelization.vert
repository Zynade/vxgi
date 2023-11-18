#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTex;

uniform mat4 M;

out vec3 worldPositionGeom;
out vec2 texCoordGeom;

void main() {
    worldPositionGeom = vec3(M * vec4(aPos, 1.0));
    texCoordGeom = aTex;
    gl_Position = vec4(worldPositionGeom, 1.0);
}
