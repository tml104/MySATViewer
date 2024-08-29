#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;

uniform vec3 cameraPos;
uniform samplerCube skybox;

vec3 calExerciseLight(){

    // 反射
    vec3 I = normalize(FragPos - cameraPos);
    vec3 R = reflect(I, normalize(Normal));

    vec3 ambient = vec3(0.2f, 0.2f, 0.2f) * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse = vec3(0.5f, 0.5f, 0.5f) * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = vec3(1.0f, 1.0f, 1.0f) * vec3(texture(texture_specular1, TexCoords));
    vec3 reflection =  vec3(texture(texture_height1, TexCoords)) * texture(skybox, R).rgb;

    return (ambient + diffuse + reflection);
    // return (reflection);
}

void main()
{    
    vec3 result = calExerciseLight();
    FragColor = vec4(result, 1.0);
}