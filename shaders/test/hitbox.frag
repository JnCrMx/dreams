#version 450

layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outShade;

const vec3 color = vec3(1.0, 1.0, 0.0);

void main()
{
	outColor = vec4(color, 1.0);
	outShade = vec4(0.0);
}
