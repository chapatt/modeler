#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

layout(push_constant) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

void main()
{
    mat2 rotation = {{0, 1}, {-1, 0}};
    Out.Color = aColor;
    Out.UV = aUV;
    gl_Position = vec4((aPos * vec2(pc.uScale.y, pc.uScale.x) + vec2(pc.uTranslate.y, pc.uTranslate.x)) * rotation, 0, 1);
}
