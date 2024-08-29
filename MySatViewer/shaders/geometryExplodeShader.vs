#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
   vec3 normal;
   vec3 fragPos;
   vec2 texCoords;
}vs_out;

void main()
{
   vs_out.fragPos = vec3(model*vec4(aPos,1.0));
   vs_out.normal = mat3(transpose(inverse(model))) * aNormal;
   vs_out.texCoords = aTexCoords; // 纹理映射坐标不随model矩阵变换

   gl_Position = projection * view * model * vec4(aPos, 1.0);
}