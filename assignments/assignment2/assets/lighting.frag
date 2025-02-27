#version 330 core
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;

uniform vec3 lightDir;
uniform sampler2D shadowMap;
uniform sampler2D texture1;

out vec4 FragColor;

void main()
{
    vec3 color = texture(texture1, TexCoords).rgb;
    float brightness = max(dot(normalize(Normal), -lightDir), 0.0);
    FragColor = vec4(color * brightness, 1.0);
}
