#version 330 core
out vec4 FragColor;

uniform float outR;

void main()
{
	FragColor = vec4(outR, 0, 0, 1);
};