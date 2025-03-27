#version 450

// layout(location = 1) in vec2 fragTexCoord;
// layout(binding = 1) uniform sampler2D texSampler;

layout (push_constant) uniform _push_constants {
	vec4 minimizeColor;
	vec4 maximizeColor;
	vec4 closeColor;
} PushConstants;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 background = vec4(1.0, 0.0, 0.0, 0.2);
	// vec4 textureColor = texture(texSampler, fragTexCoord);
	//outColor = mix(background, vec4(textureColor.rgb, 1.0), textureColor.a);
	outColor = background;
}
