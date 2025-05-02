// Munn: The following code is inspired by https://learnopengl.com/In-Practice/2D-Game/Particles
#version 330
in vec2 texcoord;
in vec4 fcolor;

// Application data
uniform sampler2D sampler0;


// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = texture(sampler0, texcoord) * fcolor;
}
