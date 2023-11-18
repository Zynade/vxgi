#version 440 core

in vec3 worldPositionFrag;
in vec2 texCoordFrag;

uniform vec3 worldCenter;
uniform float worldSizeHalf;

layout(RGBA8) uniform image3D voxelTexture;
uniform sampler2D tex;

void main() {
    vec4 fColor = texture(tex, texCoordFrag);
    vec3 voxel = (worldPositionFrag - worldCenter) / worldSizeHalf; // [-1, 1]
    voxel = 0.5 * voxel + vec3(0.5); // [0, 1]
    ivec3 dim = imageSize(voxelTexture);
    ivec3 coord = ivec3(dim * voxel);
    imageStore(voxelTexture, coord, fColor);
}
