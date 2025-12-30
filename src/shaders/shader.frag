#version 330 core
out vec4 FragColor;

in vec3 vertColor;

uniform float timeValue;

void main()
{
    float opacity = (sin(timeValue) / 2.0) + 0.5;
    FragColor = vec4(vertColor * opacity, 1.0);
}
