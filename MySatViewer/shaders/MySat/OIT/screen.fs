#version 420 core

layout (location = 0) out vec4 frag;

in vec2 texture_coords;

uniform sampler2D screen;

void main()
{
    frag = vec4(texture(screen, texture_coords).rgb, 1.0f);
    // frag = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}