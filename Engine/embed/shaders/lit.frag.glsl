#version 450 core

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

layout (std140, binding = 0) uniform View
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

// #include "lit_common.glsl"

in vec3 v_WorldPos;
in vec2 v_UV;
in mat3 v_TBN;
flat in vec3 v_Albedo;
flat in vec2 v_Params;
in vec4 v_PosLightSpace;

out vec4 o_Color;

// ---------- UBO ----------
// Debug View Enum
const uint DEBUG_NONE = 0u;
const uint DEBUG_ALBEDO = 1u;
const uint DEBUG_NORMAL = 2u;
const uint DEBUG_UV = 3u;
const uint DEBUG_TANGENT = 4u;
const uint DEBUG_NORMAL_MAP = 5u;
const uint DEBUG_LIGHTING_ONLY = 6u;

// ---------- SSBO ----------
struct DirectionalLight
{
    vec4 direction;
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
    vec4 direction;
    vec4 color;
    vec4 coneCos;
};
layout (std430, binding = 2) readonly buffer SpotLights
{
    uint spotCount;
    SpotLight spotLights[];
};

// ---------- Textures ----------
layout (binding = 0) uniform sampler2D u_Albedo;
layout (binding = 1) uniform sampler2D u_Normal;
layout (binding = 2) uniform sampler2D u_ShadowMap;

// ---------- Helper Functions ----------

vec3 BlinnPhong(in vec3 L, in vec3 N, in vec3 V, in vec3 radiance, in vec3 albedo)
{
    vec3 H = normalize(L + V); // Half-vector

    vec3 diffus = radiance * albedo * max(dot(N, L), 0);
    vec3 specular = radiance * v_Params.y * pow(max(dot(N, H), 0), v_Params.x) * step(0.0, dot(N, L));

    return diffus + specular;
}

float DistanceAttenuation(in float d, in float r)
{
    // inverse square + a window function that forces the value to 0 at the limit of the range
    // the +1 avoids division by 0 and assumes units are in meters
    float x = d / r;
    float x2 = x * x;
    float window = max(0.0, 1.0 - x2 * x2);
    return (window * window) / (1.0 + d * d);
}

void EvalDirectionalLight(in DirectionalLight light, in vec3 N, in vec3 V, in vec3 albedo, inout vec3 color)
{
    vec3 L = normalize(-light.direction.xyz); // Light vector

    color += BlinnPhong(L, N, V, light.color.xyz, albedo);
}

void EvalPointLight(in PointLight light, in vec3 N, in vec3 V, in vec3 albedo, inout vec3 color)
{
    vec3 toLight = light.positionRange.xyz - v_WorldPos;
    float d = max(length(toLight), 1e-4);
    vec3 L = toLight / d;

    float r = max(light.positionRange.w, 1e-4);

    color += DistanceAttenuation(d, r) * BlinnPhong(L, N, V, light.color.xyz, albedo);
}

void EvalSpotLight(in SpotLight light, in vec3 N, in vec3 V, in vec3 albedo, inout vec3 color)
{
    vec3 toLight = light.positionRange.xyz - v_WorldPos;
    float d = max(length(toLight), 1e-4);
    vec3 L = toLight / d;

    float r = max(light.positionRange.w, 1e-4);

    // Cone attenuation
    float cosTheta = dot(-L, light.direction.xyz);
    float spotFactor = smoothstep(light.coneCos.y, light.coneCos.x, cosTheta);

    color += DistanceAttenuation(d, r) * spotFactor * BlinnPhong(L, N, V, light.color.xyz, albedo);
}

// ---------- Main ----------
void main()
{
    vec3 V = normalize(u_CameraPosition.xyz - v_WorldPos); // View vector (eye)

    vec3 albedo = v_Albedo * texture(u_Albedo, v_UV).rgb;

    if (u_Flags.x == DEBUG_LIGHTING_ONLY)
    {
        albedo = vec3(0.5);
    }

    vec3 lighting = vec3(0);

    vec3 n = texture(u_Normal, v_UV).xyz * 2.0 - 1.0;
    vec3 N = normalize(v_TBN * n);

    for (uint i = 0; i < dirCount; i++) {
        EvalDirectionalLight(dirLights[i], N, V, albedo, lighting);
    }

    for (uint i = 0; i < pointCount; i++) {
        EvalPointLight(pointLights[i], N, V, albedo, lighting);
    }

    for (uint i = 0; i < spotCount; i++) {
        EvalSpotLight(spotLights[i], N, V, albedo, lighting);
    }

    vec3 p = v_PosLightSpace.xyz / v_PosLightSpace.w;   // clip → NDC [-1, 1]
    p = p * 0.5 + 0.5;
    float shadow = (p.z > 1.0) ? 0.0 : (p.z - u_Settings.x > texture(u_ShadowMap, p.xy).r ? 1.0 : 0.0);
    vec3 color = u_Ambient.rgb * albedo + (1.0 - shadow) * lighting;

    color = color / (color + 1.0); // tonemap

    switch (u_Flags.x) {
        case DEBUG_ALBEDO:
            o_Color = vec4(pow(albedo, vec3(1.0 / 2.2)), 1.0);
            break;
        case DEBUG_NORMAL:
            o_Color = vec4(normalize(v_TBN[2]) * 0.5 + 0.5, 1.0);
            break;
        case DEBUG_UV:
            o_Color = vec4(v_UV, 0.0, 1.0);
            break;
        case DEBUG_TANGENT:
            o_Color = vec4(normalize(v_TBN[0]) * 0.5 + 0.5, 1.0);
            break;
        case DEBUG_NORMAL_MAP:
            o_Color = vec4(N * 0.5 + 0.5, 1.0);
            break;
        case DEBUG_NONE:
        default:
            o_Color = vec4(pow(color, vec3(1.0 / 2.2)), 1.0); // Gamma correction
            break;
    }
}
