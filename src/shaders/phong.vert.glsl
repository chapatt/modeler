#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;

layout (push_constant) uniform _push_constants {
	mat4 mvp;
} PushConstants;

mat4 mvp = PushConstants.mvp;

void main(){
	mat4 normalMat = transpose(inverse(mvp));
	vec4 vertPos4 = mvp * vec4(inPosition, 1.0);
	fragPosition = vec3(vertPos4) / vertPos4.w;
	fragNormal = vec3(normalMat * vec4(inNormal, 0.0));
	gl_Position = vertPos4;
}
