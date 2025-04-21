#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Color;

void main()
{
    Color = aColor;

   gl_Position = projection * view * model * vec4(aPos, 1.0);    
   gl_PointSize = 10.0f;    
}