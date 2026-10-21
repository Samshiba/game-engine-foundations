#version 450 core

in vec3 v_Normal;
in vec3 v_WorldPos;
flat in vec3 v_Albedo;
flat in vec2 v_Params;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout (std140, binding = 1) uniform Light
{
    vec4 u_Direction;
    vec4 u_Color;
    vec4 u_Ambient;
};

out vec4 o_Color;

void main()
{
    vec3 N = normalize(v_Normal);
    vec3 L = normalize(-u_Direction.xyz);
    vec3 V = normalize(u_CameraPosition.xyz - v_WorldPos);
    vec3 H = normalize(L + V);

    vec3 ambiant = u_Ambient.xyz * v_Albedo;
    vec3 diffus = u_Color.xyz * v_Albedo * max(dot(N, L), 0);
    vec3 specular = u_Color.xyz * v_Params.y * pow(max(dot(N, H), 0), v_Params.x) * step(0.0, dot(N, L));

    vec3 color = ambiant + diffus + specular;

    o_Color = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
