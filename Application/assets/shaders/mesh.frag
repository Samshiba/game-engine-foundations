#version 450 core

in vec3 v_Normal;

out vec4 o_Color;

void main()
{
    // Debug view: the world-space normal as a color. A normal is in [-1, 1],
    // a color in [0, 1]: n * 0.5 + 0.5 maps one onto the other.
    // +X = red, +Y = green, +Z = blue. Normalized again because the
    // interpolation between vertices shortens it.
    o_Color = vec4(normalize(v_Normal) * 0.5 + 0.5, 1.0);
}
