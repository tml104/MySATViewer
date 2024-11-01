#version 330 core

out vec4 FragColor;

uniform vec3 subcolor;

void main()
{           
    FragColor = vec4(subcolor, 1.0);
}