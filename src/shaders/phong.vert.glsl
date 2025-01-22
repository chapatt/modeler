#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

layout (push_constant) uniform _push_constants {
	mat4 mvp;
} PushConstants;

mat4 mvp = PushConstants.mvp;

void main() {
	gl_Position = mvp * vec4(inPosition, 1.0);
	fragColor = vec3(1.0, 0.0, 0.0);
}
