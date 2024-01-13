#version 450

layout(location = 0) out vec4 outPosition;

layout(set = 0, binding = 0, std140) uniform UBO
{
    mat4 projection;
    mat4 view;
} global;
layout(set = 0, binding = 1, std140) uniform UBO2
{
    mat4 transformation;
    vec4 min;
    vec4 max;
} model;

vec3 positions[24] = vec3[](
    vec3(0.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),

    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 1.0),

    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 0.0, 1.0),

    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, 0.0),


    vec3(0.0, 1.0, 0.0),
    vec3(1.0, 1.0, 0.0),

    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 1.0, 1.0),

    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 1.0, 1.0),

    vec3(0.0, 1.0, 1.0),
    vec3(0.0, 1.0, 0.0),


    vec3(0.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),

    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),

    vec3(1.0, 0.0, 1.0),
    vec3(1.0, 1.0, 1.0),

    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0)
);

void main()
{
    vec3 inPosition = mix(model.min.xyz, model.max.xyz, positions[gl_VertexIndex]);

    vec4 pos = global.projection * global.view * vec4(inPosition, 1.0);
    outPosition = pos;
    gl_Position = pos;
}
