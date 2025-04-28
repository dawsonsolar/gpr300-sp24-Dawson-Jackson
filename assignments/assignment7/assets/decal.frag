// this made me want to give up good LORD the amount of times i had to change this
#version 330 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;

in vec2 TexCoords;

uniform sampler2D depthTex;
uniform sampler2D gPositionTex;
uniform sampler2D gNormalTex;
uniform sampler2D gAlbedoTex;
uniform sampler2D decalTex;

uniform mat4 inverseView;
uniform mat4 inverseProjection;
uniform mat4 decalModelInverse;
uniform vec2 screenSize;
uniform float nearPlane;
uniform float farPlane;

void main()
{
    vec2 uv = gl_FragCoord.xy / screenSize;

    vec3 origPosition = texture(gPositionTex, uv).rgb;
    vec3 origNormal = texture(gNormalTex, uv).rgb;
    vec3 origAlbedo = texture(gAlbedoTex, uv).rgb;

    // Default: write back unmodified
    gPosition = origPosition;
    gNormal = normalize(origNormal);
    gAlbedo = origAlbedo;

    float depth = texture(depthTex, uv).r;
    if(depth >= 1.0)
        discard; // Don't touch background

    // Convert screen UV + sampled depth back to world space
    float z_ndc = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, z_ndc, 1.0);
    vec4 viewPos = inverseProjection * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = inverseView * viewPos;

    // Move into decal's local space
    vec4 localPos = decalModelInverse * worldPos;

    // If we're outside the decal box, skip
    if (abs(localPos.x) > 0.5 || abs(localPos.y) > 0.5 || abs(localPos.z) > 0.5)
        discard;

    // Compute decal UVs
    vec2 decalUV = localPos.xz + 0.5;
    vec4 decalColor = texture(decalTex, decalUV);

    if (decalColor.a < 0.01)
        discard; // Ignore if fully transparent

    // Blend the decal color onto the surface
    gAlbedo = mix(origAlbedo, decalColor.rgb, decalColor.a);
}
