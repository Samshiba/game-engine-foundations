#version 450 core

// ----- Vertex Data -----
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Tangent;
layout (location = 3) in vec2 a_UV;

// ----- Instance Data -----
layout (location = 4) in mat4 a_Model;
layout (location = 8) in mat3 a_NormalMatrix;
layout (location = 11) in vec3 a_Albedo;
layout (location = 12) in vec2 a_Params;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

out vec3 v_WorldPos;
out vec2 v_UV;
out mat3 v_TBN;
flat out vec3 v_Albedo;
flat out vec2 v_Params;

void main()
{
    vec3 N = normalize(a_NormalMatrix * a_Normal);
    vec3 T = normalize(mat3(a_Model) * a_Tangent.xyz);
    T = normalize(T - (dot(T, N) * N)); // Gram-Schmidt
    vec3 B = cross(N, T) * a_Tangent.w;
    v_TBN = mat3(T, B, N);

    v_WorldPos = vec3(a_Model * vec4(a_Position, 1.0));
    v_UV = a_UV;
    v_Albedo = a_Albedo;
    v_Params = a_Params;
    gl_Position = u_ViewProjection * a_Model * vec4(a_Position, 1.0);
}
