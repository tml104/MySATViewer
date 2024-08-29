#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox; // 注意类型，这里是samplerCube 而非 sampler2D

void main()
{    
    FragColor = texture(skybox, TexCoords);
}