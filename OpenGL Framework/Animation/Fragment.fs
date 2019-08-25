#version 330 core

out vec4 FragColor;

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 WorldPos0;

uniform sampler2D gColorMap;

void main()
{
    vec4 diffuseColor = vec4(1.0,0.0,0.0,1.0);

	FragColor = mix(texture(gColorMap, TexCoord0).rgba, diffuseColor, 0.2);
};
