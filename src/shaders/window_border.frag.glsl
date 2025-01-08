#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform _push_constants {
	vec2 extent;
	vec2 offset;
	float cornerRadius;
} PushConstants;

float sdfRoundedRectangle(vec2 p, vec2 b, float r) {
	vec2 q = abs(p) - b + r;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float sigmoid(float t) {
	return 1.0 / (1.0 + exp(-t));
}

vec2 extent = PushConstants.extent;
vec2 offset = PushConstants.offset;
float cornerRadius = PushConstants.cornerRadius;
float blurRadius = 20.0;
vec3 fragColor = subpassLoad(inputColor).rgb;
vec3 shadowPaint = vec3(0.0);

void main()
{
	vec2 center = (extent.xy / 2.0) + offset;
	vec2 size = extent.xy / 2;

	float boxDistance = sdfRoundedRectangle(gl_FragCoord.xy - center, size, cornerRadius);
	float shadowDistance = sdfRoundedRectangle((gl_FragCoord.xy - center) - vec2(0, 3), size, cornerRadius);

	float boxAlpha = clamp(0.5 - boxDistance, 0.0, 1.0);
	// float shadowAlpha = (1.0 - smoothstep(-blurRadius, blurRadius, shadowDistance)) / 2;
	float shadowAlpha = (1.0 - clamp(sigmoid(shadowDistance / 5), 0.0, 1.0)) / 1.6;

	vec3 boxColor = fragColor * boxAlpha;
	vec3 shadowColor = shadowPaint * shadowAlpha;

	float outAlpha = boxAlpha + (shadowAlpha * (1.0 - boxAlpha));
	vec3 outRgb = boxColor + (shadowColor * (1.0 - boxAlpha));
	outColor = vec4(outRgb * outAlpha, outAlpha);
}
