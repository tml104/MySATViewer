#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
} fs_in;

uniform vec3 viewPos;

void main()
{           
    vec3 color = vec3(1.0, 1.0, 0.0);

    FragColor = vec4(color, 1.0);
}