#version 420 core

layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} fs_in;

uniform vec3 viewPos;

float transparency = 0.5;

void main()
{
    vec3 color = vec3(1.0, 1.0, 0.0); // 黄

    // ambient
    vec3 ambient = 0.05 * color;

    // diffuse
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);
    // float diff = max(dot(viewDir, normal), 0.0);
    float diff = dot(viewDir, normal);
    if(diff < 0.0)
    {
        diff = -diff;
        color = vec3(1.0, 0.0, 0.0);
    }
    vec3 diffuse = diff * color;

    color = ambient + diffuse;

    float weight = clamp(
        pow(min(1.0, transparency * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
        1e-2,
        3e3
    ); // 这是一个和片段深度有关系的函数

    accum = vec4(color * transparency, transparency) * weight;
    reveal = transparency;
}