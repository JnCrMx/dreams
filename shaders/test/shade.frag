#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec4 outColor;

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput inputShading;

layout(set = 0, binding = 2, std140) uniform UBO
{
    vec4 position;
    vec4 direction;
    vec4 color;

    mat4 lightMatrix;
    mat4 globalInverse;
} light;

layout(set = 1, binding = 0) uniform sampler2D shadowMap;
const int pcfCount = 2;

void main()
{
    vec3 color = subpassLoad(inputColor).rgb;
    vec4 shading = subpassLoad(inputShading);

    vec4 position = vec4(inPosition, shading.w, 1.0);

    vec4 originalPosition = light.globalInverse * position;
    originalPosition = originalPosition / originalPosition.w;

    vec4 lightPosition = light.lightMatrix * originalPosition;
    vec3 lightCoords = lightPosition.xyz / lightPosition.w;
    lightCoords.xy = lightCoords.xy * 0.5 + vec2(0.5);
    float currentDepth = min(lightCoords.z, 1.0);

    vec3 normal = normalize(shading.xyz);
    vec3 lightDir = normalize(light.position.xyz - originalPosition.xyz);

    //vec3 ambient = 0.15 * color;
    vec3 ambient = 0.05 * color;

    float diff = max(dot(normal, lightDir), 0);
    vec3 diffuse = diff * light.color.rgb;

    //float bias = max(0.05 * (1.0 - dot(normal, light.position.xyz - originalPosition.xyz)), 0.005);
    float bias = 0.0005;
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0) / textureSize(shadowMap, 0);
    for(int x = -pcfCount; x <= pcfCount; ++x)
    {
        for(int y = -pcfCount; y <= pcfCount; ++y)
        {
            float pcfDepth = texture(shadowMap, lightCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((2*pcfCount+1)*(2*pcfCount+1));

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse)) * color;
    outColor = vec4(lighting, 1.0);

    if(shading == vec4(0.0))
        outColor = vec4(color, 1.0);
}
