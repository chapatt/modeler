#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform _push_constants {
	vec4 diffuseColor;
	vec4 ambientColor;
} PushConstants;


const float Ka = 1.0;
const float Kd = 1.0;
const float Ks = 1.0;
const float shininessVal = 5.0;
const vec3 specularColor = vec3(1.0, 1.0, 1.0);
const vec3 lightPos = vec3(10.0, -5.0, -10.0);

vec3 diffuseColor = vec3(PushConstants.diffuseColor);
vec3 ambientColor = vec3(PushConstants.ambientColor);

void main() {
	vec3 N = normalize(fragNormal);
	vec3 L = normalize(lightPos - fragPosition);

	float lambertian = max(dot(N, L), 0.0);
	float specular = 0.0;
	if (lambertian > 0.0) {
		vec3 R = reflect(-L, N);
		vec3 V = normalize(-fragPosition);
		float specAngle = max(dot(R, V), 0.0);
		specular = pow(specAngle, shininessVal);
	}

	outColor = vec4(Ka * ambientColor +
		Kd * lambertian * diffuseColor +
		Ks * specular * specularColor, 1.0);
}
