#version 450 core

#include "common.glsl"

// ----- Vertex Data -----
layout (location = 0) in vec3 a_Position;

// ----- Instance Data -----
layout (location = 4) in mat4 a_Model;


void main()
{
    gl_Position = u_ViewProjection * a_Model * vec4(a_Position, 1.0);
}