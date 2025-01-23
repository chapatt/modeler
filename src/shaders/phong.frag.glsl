#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

float Ka = 1.0;
float Kd = 1.0;
float Ks = 1.0;
float shininessVal = 0.8;
vec3 ambientColor = vec3(0.09, 0.01, 0.01);
vec3 diffuseColor = vec3(0.9, 0.1, 0.1);
vec3 specularColor = vec3(1.0, 1.0, 1.0);
vec3 lightPos = vec3(1.0, 1.0, 1.0);

void main() {
	vec3 N = normalize(fragNormal);
	vec3 L = normalize(lightPos - fragPosition);

	float lambertian = max(dot(N, L), 0.0);
	float specular = 0.0;
	if(lambertian > 0.0) {
		vec3 R = reflect(-L, N);
		vec3 V = normalize(-fragPosition);
		float specAngle = max(dot(R, V), 0.0);
		specular = pow(specAngle, shininessVal);
	}

	outColor = vec4(Ka * ambientColor +
		Kd * lambertian * diffuseColor +
		Ks * specular * specularColor, 1.0);
}
