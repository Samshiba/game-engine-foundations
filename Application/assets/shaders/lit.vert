#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_UV;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout (std140, binding = 1) uniform Object
{
    mat4 u_Model;
    mat4 u_NormalMatrix;
};

layout (std140, binding = 2) uniform Light
{
    vec4 u_Direction;
    vec4 u_Color;
    vec4 u_Ambient;
};

layout (std140, binding = 3) uniform Material
{
    vec4 u_Albedo;
    vec4 u_Params;
};

out vec3 v_Normal;
out vec3 v_WorldPos;

void main()
{
    v_Normal = mat3(u_NormalMatrix) * a_Normal;
    v_WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}
