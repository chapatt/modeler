#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 fragTexCoord2;
layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 background = vec4(fragColor, 1.0);
	vec4 texture1 = texture(texSampler, fragTexCoord);
	vec4 texture2 = texture(texSampler, fragTexCoord2);
	vec4 firstTextureApplied = mix(background, vec4(texture1.rgb, 1.0), texture1.a);
	outColor = mix(firstTextureApplied, vec4(texture2.rgb, 1.0), texture2.a);
}
