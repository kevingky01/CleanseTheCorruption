#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform vec2 texture_size;
uniform bool is_outline_active;
uniform bool colour_edge;


// Output color
layout(location = 0) out vec4 color;


bool is_outline() {

    // Munn: The following code was taken from: http://blogs.love2d.org/content/let-it-glow-dynamically-adding-outlines-characters
	bool is_transparent = true;
    float alpha = 4*texture( sampler0, texcoord ).a;
    if (alpha > 0.0) {
        is_transparent = false;
    }

    float outline_size_x = 1.0f / texture_size.x; // TODO: maybe also add an outline size parameter?
    float outline_size_y = 1.0f / texture_size.y;
    alpha += texture( sampler0, texcoord + vec2( outline_size_x, 0.0f ) ).a;
    alpha += texture( sampler0, texcoord + vec2( -outline_size_x, 0.0f ) ).a;
    alpha += texture( sampler0, texcoord + vec2( 0.0f, outline_size_y ) ).a;
    alpha += texture( sampler0, texcoord + vec2( 0.0f, -outline_size_y ) ).a;
    
    return is_transparent && alpha > 0.0;
}







void main()
{
    color = texture(sampler0, texcoord);


    if (is_outline() && is_outline_active) { // if it is part of the outline
        color = vec4(1.0, 1.0, 1.0, 1.0);

    } else if (colour_edge) {
        // describing rampage mode of an enemy
       color = vec4(1.0, 0, 0, 1.0);



    }
}
