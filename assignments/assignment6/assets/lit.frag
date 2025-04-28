#version 450
out vec4 FragColor;

in Surface
{
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoord;
} fs_in;

uniform sampler2D _MainTex;
uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0, -1.0, 0.0);
uniform vec3 _LightColor = vec3(1.0);
uniform vec3 _AmbientColor = vec3(0.3, 0.4, 0.46);

struct Material
{
    float Ka;
    float Kd;
    float Ks;
    float Shininess;
};
uniform Material _Material;

void main()
{
    vec3 normal = normalize(fs_in.WorldNormal);
    vec3 toLight = normalize(-_LightDirection); // Ensure it's normalized
    vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
    vec3 h = normalize(toLight + toEye);

    float diffuseFactor = max(dot(normal, toLight), 0.0);
    float specularFactor = pow(max(dot(normal, h), 0.0), _Material.Shininess);

    vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
    lightColor += _AmbientColor * _Material.Ka;

    vec3 objectColor = texture(_MainTex, fs_in.TexCoord).rgb;
    FragColor = vec4(objectColor * lightColor, 1.0);
}
