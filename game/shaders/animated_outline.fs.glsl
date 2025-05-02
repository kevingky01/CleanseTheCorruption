#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;

uniform float current_frame;
uniform float h_frames;
uniform float v_frames;

uniform vec2 texture_size;
uniform bool is_outline_active;

bool is_outline(vec2 frame_coords) {

    // Munn: The following code was taken from: http://blogs.love2d.org/content/let-it-glow-dynamically-adding-outlines-characters
	bool is_transparent = true;
    float alpha = 4*texture( sampler0, frame_coords ).a;
    if (alpha > 0.0) {
        is_transparent = false;
    }

    float outline_size_x = 1.0f / texture_size.x; // TODO: maybe also add an outline size parameter?
    float outline_size_y = 1.0f / texture_size.y;
    alpha += texture( sampler0, frame_coords + vec2( outline_size_x, 0.0f ) ).a;
    alpha += texture( sampler0, frame_coords + vec2( -outline_size_x, 0.0f ) ).a;
    alpha += texture( sampler0, frame_coords + vec2( 0.0f, outline_size_y ) ).a;
    alpha += texture( sampler0, frame_coords + vec2( 0.0f, -outline_size_y ) ).a;
    
    return is_transparent && alpha > 0.0;
}

// Output color
layout(location = 0) out  vec4 color;

vec2 getFrameTexcoords() {
	
	float current_h = float(int(current_frame) % int(h_frames));
	float current_v = (v_frames - 1.0) - (int(current_frame) / int(h_frames)); 

	float true_texcoord_x = (texcoord.x / h_frames) + current_h * (1.0 / h_frames);
	float true_texcoord_y = (texcoord.y / v_frames) + current_v * (1.0 / v_frames);

	return vec2(true_texcoord_x, true_texcoord_y);
}

void main()
{
	vec2 frame_coords = getFrameTexcoords();
	color = vec4(fcolor, 1.0) * texture(sampler0, frame_coords);

    if (is_outline(frame_coords) && is_outline_active) { // if it is part of the outline
        color = vec4(1.0, 1.0, 1.0, 1.0);
    } 
}
