#version 450

layout(location=0) out vec4 outColor;

layout (push_constant) uniform _push_constants {
    vec2 extent;
} PushConstants;

float sdfRoundedRectangle(vec2 p, vec2 b, float r) {
	vec2 q = abs(p) - b + r;
	return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

float cornerRadius = 10.0;
float blurRadius = 15.0;
vec4 fragColor = vec4(0.0, 0.0, 1.0, 1.0);

void main()
{
	vec2 center = PushConstants.extent.xy / 2.0;
	vec2 size = (PushConstants.extent.xy / 2.0) - blurRadius;

	float boxDistance = sdfRoundedRectangle(gl_FragCoord.xy - center, size, cornerRadius);
	float shadowDistance = sdfRoundedRectangle(gl_FragCoord.xy - center, size, cornerRadius);

	float boxAlpha = 1.0 - smoothstep(0.0, 2.0, boxDistance);
	float shadowAlpha = 1.0 - smoothstep(-blurRadius, blurRadius, shadowDistance);

	vec4 boxColor = mix(vec4(0.0), fragColor, boxAlpha);
	vec4 shadowColor = vec4(vec3(0.0), 0.6);

	outColor = mix(boxColor, shadowColor, shadowAlpha - boxAlpha);
}