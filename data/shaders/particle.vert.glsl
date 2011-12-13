attribute vec3 position;
attribute vec3 velocity;
attribute float birthTime;
uniform float time;
uniform vec3 acceleration;
varying float age;

void main(void)
{
	age = time - birthTime;
	vec3 pos = position + velocity*age + 0.5*acceleration*age*age;
	// Uniform accel. s = s0 + u t + 1/2 a t squared
#ifdef ZHACK
	logarithmicTransform(gl_ModelViewProjectionMatrix * vec4(pos, 1.0));
#else
	gl_Position = gl_ModelViewProjectionMatrix * pos;
#endif
}

