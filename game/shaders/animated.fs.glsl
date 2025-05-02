#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;

uniform float current_frame;
uniform float h_frames;
uniform float v_frames;

uniform bool is_hitflash;

// Output color
layout(location = 0) out  vec4 color;

vec2 getFrameTexcoords() {
	
	float current_h = float(int(current_frame) % int(h_frames));
	float current_v = (int(current_frame) / int(h_frames)); 

	float true_texcoord_x = (texcoord.x / h_frames) + current_h * (1.0 / h_frames);
	float true_texcoord_y = (texcoord.y / v_frames) + current_v * (1.0 / v_frames);

	return vec2(true_texcoord_x, true_texcoord_y);
}

void main()
{
	vec2 frame_coords = getFrameTexcoords();
	color = vec4(fcolor, 1.0) * texture(sampler0, frame_coords);

	if (is_hitflash) {
		color.xyz = vec3(1.0, 1.0, 1.0);
	}
}
