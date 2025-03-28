#version 450

layout (push_constant) uniform _push_constants {
	vec4 minimizeColor;
	vec4 maximizeColor;
	vec4 closeColor;
	float height;
} PushConstants;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

float height = PushConstants.height;
float width = height * 2;

vec2 positions[] = vec2[](
	vec2(-1, -1), vec2(-1, height - 1.0), vec2(1, height - 1.0), vec2(1, -1),
	vec2(1.0 - width, -1), vec2(1.0 - width, height - 1.0), vec2(1, height - 1.0), vec2(1, -1),
	vec2(1.0 - width * 2, -1), vec2(1.0 - width * 2, height - 1.0), vec2(1 - width, height - 1.0), vec2(1 - width, -1),
	vec2(1.0 - width * 3, -1), vec2(1.0 - width * 3, height - 1.0), vec2(1 - width * 2, height - 1.0), vec2(1 - width * 2, -1)
);

int indices[] = int[](
	0, 1, 2, 0, 2, 3,
	4, 5, 6, 4, 6, 7,
	8, 9, 10, 8, 10, 11,
	12, 13, 14, 12, 14, 15
);

void main() {
	gl_Position = vec4(positions[indices[gl_VertexIndex]], 0.0, 1.0);
	if (gl_VertexIndex < 6) {
		fragColor = vec4(1.0, 1.0, 1.0, 0.2);
	} else if (gl_VertexIndex < 12) {
		fragColor = vec4(0.0, 1.0, 0.0, 1.0);
	} else if (gl_VertexIndex < 18) {
		fragColor = vec4(0.0, 0.0, 1.0, 1.0);
	} else if (gl_VertexIndex < 24) {
		fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	}
	fragTexCoord = vec2(0.0, 0.0);
}
