attribute vec3 position;
attribute vec3 velocity;
attribute vec3 texTransform;
attribute float birthTime;
uniform float time;
uniform vec3 acceleration;
varying float age;

varying mat2 texRot;
varying vec2 texTrans;

void main(void)
{
	age = time - birthTime;
	{
		float sin_theta = sin(4.0*age);
		float cos_theta = cos(4.0*age);
		texRot = mat2(cos_theta, -sin_theta, sin_theta, cos_theta);
		texTrans = vec2(texTransform);
	}
	vec3 pos = position + velocity*age + 0.5*acceleration*age*age;
	// Uniform accel. s = s0 + u t + 1/2 a t squared
#ifdef ZHACK
	logarithmicTransform(gl_ModelViewProjectionMatrix * vec4(pos, 1.0));
#else
	gl_Position = gl_ModelViewProjectionMatrix * pos;
#endif
}

