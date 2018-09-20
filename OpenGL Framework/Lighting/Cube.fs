#version 330 core
out vec4 FragColor;

uniform vec4 objectColor;
uniform vec4 lightColor;
uniform vec3 lightPos;

uniform vec3 viewPos;

in vec3 Normal;
in vec3 FragPos;

void main()
{
	float ambientStrength = 0.1;
	float specularStrength = 0.5;

	vec3 ambient = ambientStrength * lightColor.rgb;

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);

	float diff = max(dot(norm, lightDir), 0.0);

	vec3 diffuse = (lightColor * diff).rgb;

	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor.rgb;

	vec3 result = ambient * objectColor.rgb + diffuse * objectColor.rgb + specular * objectColor.rgb;

	FragColor = vec4(result, 1.0f);
};