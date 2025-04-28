#version 330 core

in vec4 FragPosViewSpace;
out vec4 FragColor;

uniform mat4 decalModelInverse;

void main()
{
    // Same screen UV calculation (but preview ignores it)

    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Pure red wireframe box
}
