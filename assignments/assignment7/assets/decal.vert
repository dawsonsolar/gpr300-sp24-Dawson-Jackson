#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 FragPosVS;
out vec4 FragPosCS;

void main()
{
    FragPosVS = view * model * vec4(aPos, 1.0);
    FragPosCS = projection * FragPosVS;
    gl_Position = FragPosCS;
}
