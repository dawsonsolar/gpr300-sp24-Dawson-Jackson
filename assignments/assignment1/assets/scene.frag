#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture1;

void main() 
{
    vec3 texColor = texture(texture1, TexCoords).rgb;
    FragColor = vec4(texColor, 1.0);
}
