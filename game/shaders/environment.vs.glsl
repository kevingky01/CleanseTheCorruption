#version 330

// Input attributes
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texcoord;
layout (location = 2) in mat3 instance_matrix;

in vec2 in_tilecoord;

// Passed to fragment shader
out vec2 texcoord;
out vec2 tilecoord;

// Application data
uniform mat3 projection;

uniform float h_tiles;
uniform float v_tiles;


void main()
{
	texcoord = in_texcoord;
	tilecoord = in_tilecoord;
	vec3 pos = projection * instance_matrix * vec3(in_position.x / h_tiles, in_position.y / v_tiles, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}