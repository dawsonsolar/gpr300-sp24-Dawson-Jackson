#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 decalModel;
uniform mat4 view;
uniform mat4 projection;

out vec4 positionVS;
out vec4 positionCS;

void main()
{
    mat4 viewProj = projection * view;
    positionVS = decalModel * vec4(aPos, 1.0);
    positionCS = viewProj * positionVS;
    gl_Position = positionCS;
}
