#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
// uniform mat4 projection;

out VS_OUT
{
   vec3 normal;
}vs_out;

void main()
{
   // 将Normal变换到观察空间
   mat3 normalMatrix = mat3(transpose(inverse(view * model)));
   vs_out.normal = normalize(normalMatrix * aNormal); 

   // 只传变换到观察空间这一步的坐标给几何着色器，之后再在几何着色器中做透视投影变换
   gl_Position = view * model * vec4(aPos, 1.0); 
}