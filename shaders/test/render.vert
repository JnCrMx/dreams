#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

layout(set = 0, binding = 0, std140) uniform UBO
{
	mat4 projection;
	mat4 view;
} global;
layout(set = 0, binding = 1, std140) uniform UBO2
{
	mat4 transformation;
} model;

void main()
{
	vec4 pos = global.projection * global.view * model.transformation * vec4(inPosition, 1.0);
	outPosition = pos;
	gl_Position = pos;

	mat3x4 normalViewMatrix;
	normalViewMatrix[0] = global.view[0];
	normalViewMatrix[1] = global.view[1];
	normalViewMatrix[2] = global.view[2];

	mat3x4 normalModelMatrix;
	normalModelMatrix[0] = model.transformation[0];
	normalModelMatrix[1] = model.transformation[1];
	normalModelMatrix[2] = model.transformation[2];

	outNormal = ((normalModelMatrix * inNormal.xyz).xyz).xyz;
	
	outTexCoord = inTexCoord;
}
