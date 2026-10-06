#version 450 core

// Matches GEF::Renderer::Vertex (position, normal, uv) and UploadMesh's layout
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_UV;

layout (std140, binding = 0) uniform Frame
{
    mat4 u_ViewProjection;
    mat4 u_Model;
};

out vec3 v_Normal;

void main()
{
    // Normals need the "normal matrix" (inverse transpose of the model's 3x3):
    // with a non-uniform scale, mat3(u_Model) would tilt them. Computed per
    // vertex here for simplicity; GEF-41 moves it to the CPU, once per object.
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}
