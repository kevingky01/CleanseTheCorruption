#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;

uniform float h_tiles;
uniform float v_tiles;

void main()
{
	texcoord = in_texcoord;
	vec3 pos = projection * transform * vec3(in_position.x / h_tiles, in_position.y / v_tiles, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}

