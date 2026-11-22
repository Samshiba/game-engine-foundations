#include "common.glsl"

layout (std140, binding = 1) uniform Shadow
{
    mat4 u_LightViewProjection;
    vec4 u_Settings; // x = bias, y = normal bias, z = texelSize, w = unused
};

layout (std140, binding = 2) uniform Environment
{
    vec4 u_Ambient; // rgb = ambient color, a unused
    uvec4 u_Flags; // x = debug view mode, yzw unused
};