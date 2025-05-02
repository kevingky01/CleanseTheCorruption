#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;

// Output color
uniform float h_tiles;
uniform float v_tiles;
uniform vec2 tilecoord;
uniform bool is_hitflash;

layout (location = 0) out vec4 color;

vec2 getTileTexcoords() {

	float rand_texcoord_x = (texcoord.x / h_tiles) + tilecoord.x * (1.0 / h_tiles);
	float rand_texcoord_y = (texcoord.y / v_tiles) + tilecoord.y * (1.0 / v_tiles);

	return vec2(rand_texcoord_x, rand_texcoord_y);
}


void main()
{
	vec2 tile_texcoord = getTileTexcoords();
	color = texture(sampler0, tile_texcoord);

	if (is_hitflash) {
		color.xyz = vec3(1.0, 1.0, 1.0);
	}
}
