#version 130

#define M_PI 3.1415926535897932384626433832795

in vec3 out_Color;
in vec4 gl_FragCoord;
out vec4 frag_colour;	//final output color used to render the point

void main () {
	frag_colour = vec4 (gl_FragCoord.z, out_Color.y, out_Color.z, 1.0);
}