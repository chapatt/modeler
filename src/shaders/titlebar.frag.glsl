#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 background = vec4(fragColor, 1.0);
	vec4 texture = texture(texSampler, fragTexCoord);
	outColor = mix(background, vec4(1.0, 1.0, 1.0, 1.0), texture.a);
}
