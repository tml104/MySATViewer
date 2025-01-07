#version 420 core

// shader inputs
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords; // uv

// shader outputs
out vec2 texture_coords;

void main()
{
	texture_coords = aTexCoords;

	gl_Position = vec4(aPos, 1.0f);
}