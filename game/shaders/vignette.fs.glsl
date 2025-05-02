#version 330

uniform sampler2D screen_texture;
uniform float time;

uniform float darken_screen_factor;
uniform float vignette_factor;

uniform bool is_paused;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec4 vignette(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE VIGNETTE EFFECT HERE
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	
	vec2 distance_from_center = vec2(0.5, 0.5) - texcoord;

	float vignette_distance = 0.4;
	float vignette_strength = 3.0;

	if (length(distance_from_center) > vignette_distance) { 
		float intensity = (length(distance_from_center) - vignette_distance) * vignette_strength;
		intensity *= vignette_factor;
		in_color = vec4(in_color.xyz, 1.0) * (1.0 - intensity) + vec4(1.0, 0.0, 0.0, 1.0) * intensity; // Replace red with whatever colour we would like
	}

	return in_color;
}

// darken the screen, i.e., fade to black
vec4 fade_color(vec4 in_color) 
{
	if (darken_screen_factor > 0)
		in_color = in_color * (1.0 - darken_screen_factor) + vec4(0.0, 0.0, 0.0, 0) * darken_screen_factor;
	return in_color;
}

vec4 darken_color(vec4 in_color)
{
	if (is_paused)
		in_color = in_color * 0.4;
	return in_color;
}

void main()
{
    vec4 in_color = texture(screen_texture, texcoord);
	in_color = vignette(in_color);
	in_color = fade_color(in_color);
	color = darken_color(in_color);
}