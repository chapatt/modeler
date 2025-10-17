#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 backgroundColor;
layout(location = 3) in flat int drawTexture;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main() {
	if (drawTexture == 1) {
		vec4 texture = texture(texSampler, fragTexCoord);
		outColor = mix(backgroundColor, fragColor, texture.a);
	} else {
		outColor = backgroundColor;
	}
}