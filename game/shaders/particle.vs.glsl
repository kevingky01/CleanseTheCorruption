// Munn: The following code is inspired by https://learnopengl.com/In-Practice/2D-Game/Particles
#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;
in mat3 in_transform_matrix;
in vec4 in_color;

// Passed to fragment shader
out vec2 texcoord;
out vec4 fcolor;

// Application data
uniform mat3 projection;

void main()
{
    texcoord = in_texcoord;
	fcolor = in_color;
	vec3 pos = projection * in_transform_matrix * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}

// End copied code