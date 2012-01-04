
varying mat2 texRot;
varying vec2 texTrans;
varying float age;
uniform vec4 color;
uniform sampler2D texture;

void main(void)
{
	vec2 texpos = 0.25*((gl_PointCoord.st-vec2(0.5,0.5))*texRot + vec2(0.5,0.5)) + texTrans;
	gl_FragColor = max(0.0, 1.0 - age*0.25) * color * texture2D(texture, texpos);
#ifdef ZHACK
	SetFragDepth();
#endif
}
