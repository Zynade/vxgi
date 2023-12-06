#version 440 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTex;

layout(RGBA8) uniform image3D voxelTexture;

uniform mat4 V;
uniform mat4 P;
uniform vec3 worldCenter;
uniform float worldSizeHalf;

out vec4 fragColor;

//void main() {
    //worldPositionFrag = vec3(M * vec4(aPos, 1.0));
    //gl_Position = P*V*vec4(worldPositionFrag, 1.0);
//}

void main() {
    int voxel_texture_size = imageSize(voxelTexture).x;
    int x = gl_InstanceID % voxel_texture_size;
    int y = (gl_InstanceID / voxel_texture_size) % voxel_texture_size;
    int z = gl_InstanceID / (voxel_texture_size * voxel_texture_size);
    vec4 color;
    color = imageLoad(voxelTexture, ivec3(x, y, z));

    if (color.a < 0.0001) {
        // move the voxel outside the clipping space
        gl_Position = vec4(vec3(-99999), 1.0);
    } else {
        float world_size = 2.0 * worldSizeHalf;
        float voxel_size = world_size / float(voxel_texture_size);
        vec3 coord_pos   = vec3(float(x), float(y), float(z)) /
                         float(voxel_texture_size);         // [0, 1]
        coord_pos           = 2.0 * coord_pos - vec3(1.0);  // [-1, 1]
        vec3 offset         = worldSizeHalf * coord_pos + worldCenter + 0.5 * vec3(voxel_size);
        vec3 world_position = voxel_size * aPos + offset;
        gl_Position         = P*V * vec4(world_position, 1.0);
        fragColor = color;
    }
}
