#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
} fs_in;

uniform vec3 viewPos;

void main()
{           
    vec3 color = vec3(1.0, 1.0, 0.0);

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

    FragColor = vec4(ambient + diffuse, 1.0);
}