#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;

layout (push_constant) uniform _push_constants {
	mat4 MV;
	mat4 P;
	mat4 normalMatrix;
} PushConstants;

mat4 MV = PushConstants.MV;
mat4 P = PushConstants.P;
mat4 normalMatrix = PushConstants.normalMatrix;

void main(){
	vec4 vertPos4 = MV * vec4(inPosition, 1.0);
	fragPosition = vec3(vertPos4) / vertPos4.w;
	fragNormal = vec3(normalMatrix * vec4(inNormal, 0.0));
	gl_Position = P * vertPos4;
}
