#version 450

layout(location = 1) out vec2 fragTexCoord;

vec2 positions[3] = vec2[](
	vec2(-1, 3),
	vec2(3, -1),
	vec2(-1, -1)
);

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragTexCoord = vec2(0.0, 0.0);
}
