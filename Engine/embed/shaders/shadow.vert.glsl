#version 450 core

// ----- Vertex Data -----
layout (location = 0) in vec3 a_Position;

// ----- Instance Data -----
layout (location = 4) in mat4 a_Model;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
    uvec4 u_Flags;
    mat4 u_LightViewProjection;
};

void main()
{
    gl_Position = u_LightViewProjection * a_Model * vec4(a_Position, 1.0);
}