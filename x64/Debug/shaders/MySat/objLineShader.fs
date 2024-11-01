#version 330 core

// uniform vec3 subcolor;
in vec3 flatColor;
out vec4 FragColor;

void main()
{           
    // vec3 color = vec3(0.0, 1.0, 0.0);

    FragColor = vec4(flatColor, 1.0);
}