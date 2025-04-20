#version 420 core
out vec4 FragColor;

uniform vec3 subcolor;

void main()
{           
    // vec3 color = vec3(0.0, 1.0, 0.0);

    FragColor = vec4(subcolor, 1.0);
}