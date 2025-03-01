#version 330 core
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Normal;

uniform vec3 lightDir;
uniform sampler2D shadowMap;
uniform sampler2D texture1;
uniform mat4 lightSpaceMatrix;

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace) // Going to comment here to both get used to it and I remember what everything does.
{
    // Perform the division to get the normalized depth value
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform to (0, 1) range

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Check if we are in shadow
    float shadow = currentDepth > closestDepth ? 0.5 : 1.0;
    return shadow;
}

void main()
{
    // Texture color
    vec3 color = texture(texture1, TexCoords).rgb;

    // Apply lighting
    float brightness = max(dot(normalize(Normal), -lightDir), 0.0);

    // Calculate shadow
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace);

    // Final color, factoring in shadow
    FragColor = vec4(color * brightness * shadow, 1.0);
}
