#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inTexCoord2;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 fragTexCoord2;

layout (push_constant) uniform _push_constants {
	mat4 mvp;
} PushConstants;

mat4 mvp = PushConstants.mvp;

void main() {
	gl_Position = mvp * vec4(inPosition, 0.0, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	fragTexCoord2 = inTexCoord2;
}
