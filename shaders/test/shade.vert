#version 450

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, +1.0),
    vec2(+1.0, +1.0),

    vec2(+1.0, -1.0),
    vec2(-1.0, -1.0),
    vec2(+1.0, +1.0)
);

layout(location = 0) out vec2 outPosition;

void main()
{
    outPosition = positions[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
