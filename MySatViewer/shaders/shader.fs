#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform vec3 objectColor;
uniform vec3 viewPos;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  
uniform DirLight dirLight;

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform PointLight pointLight;

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight;

vec3 calDirLight(DirLight light, vec3 normal, vec3 viewDir){
    vec3 lightDir = normalize(-light.direction);

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);

    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    return (ambient + diffuse + specular);
}

vec3 calPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);

    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    // 衰减
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                 light.quadratic * (distance * distance));    
    
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1,TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1,TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1,TexCoords));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

vec3 calSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir){

    vec3 lightDir = normalize(light.position - fragPos);

    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);

    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    // 衰减
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
                 light.quadratic * (distance * distance));    
    
    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1,TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse1,TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1,TexCoords));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;  

    // 聚光灯判定
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon   = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);    

    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular);
}

float near = 0.1; 
float far  = 100.0; 

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));    
}

void main()
{    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = calDirLight(dirLight, norm, viewDir);
    result += calPointLight(pointLight, norm, FragPos, viewDir);
    result += calSpotLight(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);

    // float depth = LinearizeDepth(gl_FragCoord.z) / far; // 为了演示除以 far
    // FragColor = vec4(vec3(depth), 1.0);
}