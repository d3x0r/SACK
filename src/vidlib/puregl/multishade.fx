
//!!ARBfp1.0
uniform sampler2D input;
uniform vec4 multishade_r;
uniform vec4 multishade_g;
uniform vec4 multishade_b;

void main(void) 
{
    vec4 color = texture2D(input, gl_TexCoord[0].st); 
    vec4 result;
	gl_FragColor = vec4( (color.b * multishade_b.r) + (color.g * multishade_g.r) + (color.r * multishade_r.r),
		(color.b * multishade_b.g) + (color.g * multishade_g.g) + (color.r * multishade_r.g),
		(color.b * multishade_b.b) + (color.g * multishade_g.b) + (color.r * multishade_r.b),
		  color.r?( color.a * multishade_r.a) :0
                + color.g?( color.a * multishade_g.a) :0
                + color.b?( color.a * multishade_b.a) :0
                )
		;
}
