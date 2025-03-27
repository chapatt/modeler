#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inTexCoord2;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec2 fragTexCoord2;

layout (binding = 0) uniform _transform_uniform {
	mat4 MV;
	mat4 P;
	mat4 normalMatrix;
} TransformUniform;

mat4 MV = TransformUniform.MV;
mat4 P = TransformUniform.P;
mat4 normalMatrix = TransformUniform.normalMatrix;

void main() {
	gl_Position = P * MV * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	fragTexCoord2 = inTexCoord2;
}
