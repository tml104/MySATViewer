#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords; // 注意这里是vec3

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos; // 注意此处直接将cube坐标作为纹理坐标输入
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // 令z = w = 1.0， 此时其深度将始终是最大值1.0
}  