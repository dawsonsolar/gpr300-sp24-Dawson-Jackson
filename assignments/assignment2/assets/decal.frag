#version 330 core

in vec4 positionVS;
in vec4 positionCS;

out vec4 FragColor0; // gNormal
out vec4 FragColor1; // gAlbedoSpec

uniform sampler2D depthMap;
uniform sampler2D decalTexture;
uniform mat4 invViewProj;

uniform mat4 decalModel;

// Resolution of the screen
uniform vec2 screenSize = vec2(800.0, 600.0); // Hardcoded, you can pass this as uniform if you want

void main()
{
    // Step 1: Screen pos -> texture coords
    vec2 screenPos = positionCS.xy / positionCS.w;
    vec2 texCoords = screenPos * 0.5 + 0.5;

    // Step 2: Sample depth buffer
    float sceneDepth = texture(depthMap, texCoords).r;
    float currentDepth = (positionCS.z / positionCS.w) * 0.5 + 0.5;

    // Step 3: Early depth test
    if(currentDepth - 0.0005 > sceneDepth) discard;
    if(sceneDepth == 1.0) discard; // Skybox or empty space

    // Step 4: Reconstruct world position
    vec4 clipPos = vec4(screenPos * 2.0 - 1.0, sceneDepth * 2.0 - 1.0, 1.0);
    vec4 worldPos = invViewProj * clipPos;
    worldPos /= worldPos.w;

    // Step 5: Transform world pos to decal local space
    vec4 localPos = inverse(decalModel) * worldPos;

    // Step 6: Clip outside decal volume
    if(abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5)
        discard;

    // Step 7: Sample decal
    vec2 decalUV = localPos.xz + vec2(0.5);

    vec4 decalColor = texture(decalTexture, decalUV);

    // Step 8: Output modified GBuffer
    FragColor0 = vec4(0.0, 0.0, 1.0, 0.0); // Just a dummy normal update (could be improved)
    FragColor1 = vec4(decalColor.rgb, 1.0); // Write decal color
}
