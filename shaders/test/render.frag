#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outShade;

const vec3 color = vec3(0.7, 0.7, 0.7);

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

void main()
{
    vec3 normal = normalize(inNormal);

    outColor = texture(albedoTexture, inTexCoord);
    outShade = vec4(normal, inPosition.z/inPosition.w);
}
