#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_UV;
layout (location = 3) in mat4 a_Model;
layout (location = 7) in mat3 a_NormalMatrix;
layout (location = 10) in vec3 a_Albedo;
layout (location = 11) in vec2 a_Params;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout (std140, binding = 1) uniform Environment
{
    vec4 u_Ambient;
};

struct DirectionalLight
{
    vec4 color;
};
layout (std430, binding = 0) readonly buffer DirectionalLights
{
    uint dirCount;
    DirectionalLight dirLights[];
};

struct PointLight
{
    vec4 positionRange;
    vec4 color;
};
layout (std430, binding = 1) readonly buffer PointLights
{
    uint pointCount;
    PointLight pointLights[];
};

struct SpotLight
{
    vec4 positionRange;
    vec4 color;
    vec4 coneAngle;
};
layout (std430, binding = 2) readonly buffer SpotLights
{
    uint spotCount;
    SpotLight spotLights[];
};

out vec3 v_Normal;
out vec3 v_WorldPos;
flat out vec3 v_Albedo;
flat out vec2 v_Params;

void main()
{
    v_Normal = mat3(a_NormalMatrix) * a_Normal;
    v_WorldPos = vec3(a_Model * vec4(a_Position, 1.0));
    v_Albedo = a_Albedo;
    v_Params = a_Params;
    gl_Position = u_ViewProjection * a_Model * vec4(a_Position, 1.0);
}
