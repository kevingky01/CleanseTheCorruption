#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;

#define MAX_WALLS 1000
uniform vec2 revealed_walls[MAX_WALLS];
uniform float num_x_tiles;
uniform float num_y_tiles;
uniform float num_revealed_walls;

// Output color
layout(location = 0) out vec4 color;


const float RANGE_DIFF = 0.0075;

void main()
{
	color = vec4(fcolor, 0.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));

	for (int i = 0; i < num_revealed_walls; i++) {
		vec2 wall = revealed_walls[i];

		vec2 tex_pos = vec2(wall.x / num_x_tiles, wall.y / num_y_tiles);

		if (distance(tex_pos, texcoord) < RANGE_DIFF) {
			color.a = 1.0;
		}
	}
}
