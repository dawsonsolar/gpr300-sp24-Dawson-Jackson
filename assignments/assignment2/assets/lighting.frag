#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 lightDir;

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;

    // Simple directional light
    float diff = max(dot(normalize(Normal), -normalize(lightDir)), 0.0);

    vec3 lighting = Albedo * diff;

    FragColor = vec4(lighting, 1.0);
}
