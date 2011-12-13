
varying float age;
uniform vec4 color;
uniform sampler2D texture;

void main(void)
{
	gl_FragColor = max(0.0, 1.0 - age*0.5) * color * texture2D(texture, gl_PointCoord.st);
#ifdef ZHACK
	SetFragDepth();
#endif
}
