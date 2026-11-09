#version 450 core

in vec3 v_Normal;
in vec3 v_WorldPos;
flat in vec3 v_Albedo;
flat in vec2 v_Params;

// ---------- UBO ----------
layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout (std140, binding = 1) uniform Environment
{
    vec4 u_Ambient;
};

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

out vec4 o_Color;

// ---------- Helper Functions ----------

vec3 BlinnPhong(in vec3 L, in vec3 N, in vec3 V, in vec3 radiance)
{
    vec3 H = normalize(L + V); // Half-vector

    vec3 diffus = radiance * v_Albedo * max(dot(N, L), 0);
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

void EvalDirectionalLight(in DirectionalLight light, in vec3 N, in vec3 V, inout vec3 color)
{
    vec3 L = normalize(-light.direction.xyz); // Light vector

    color += BlinnPhong(L, N, V, light.color.xyz);
}

void EvalPointLight(in PointLight light, in vec3 N, in vec3 V, inout vec3 color)
{
    vec3 toLight = light.positionRange.xyz - v_WorldPos;
    float d = max(length(toLight), 1e-4);
    vec3 L = toLight / d;

    float r = max(light.positionRange.w, 1e-4);

    color += DistanceAttenuation(d, r) * BlinnPhong(L, N, V, light.color.xyz);
}

void EvalSpotLight(in SpotLight light, in vec3 N, in vec3 V, inout vec3 color)
{
    vec3 toLight = light.positionRange.xyz - v_WorldPos;
    float d = max(length(toLight), 1e-4);
    vec3 L = toLight / d;

    float r = max(light.positionRange.w, 1e-4);

    // Cone attenuation
    float cosTheta = dot(-L, light.direction.xyz);
    float spotFactor = smoothstep(light.coneCos.y, light.coneCos.x, cosTheta);

    color += DistanceAttenuation(d, r) * spotFactor * BlinnPhong(L, N, V, light.color.xyz);
}

// ---------- Main ----------
void main()
{
    vec3 N = normalize(v_Normal); // Normal vector
    vec3 V = normalize(u_CameraPosition.xyz - v_WorldPos); // View vector (eye)

    vec3 color = u_Ambient.rgb * v_Albedo;

    for (uint i = 0; i < dirCount; i++) {
        EvalDirectionalLight(dirLights[i], N, V, color);
    }

    for (uint i = 0; i < pointCount; i++) {
        EvalPointLight(pointLights[i], N, V, color);
    }

    for (uint i = 0; i < spotCount; i++) {
        EvalSpotLight(spotLights[i], N, V, color);
    }

    color = color / (color + 1.0); // tonemap
    o_Color = vec4(pow(color, vec3(1.0 / 2.2)), 1.0); // Gamma correction
}
