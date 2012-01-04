#ifndef _PARTICLEGROUP_H
#define _PARTICLEGROUP_H

class ParticleGroup {
public:
	enum TYPE { TYPE_SMOKE };
	ParticleGroup(TYPE type, int numParticles);
	void Init(TYPE type, int numParticles);
	void Render();
private:
	struct Vertex {
		vector3f pos;
		vector3f vel;
		float texTransform[2];
		float angVelocity;
		float birthTime;
		float pointSize;
	};
	TYPE m_type;
	int m_numParticles;
	GLuint m_vbo;
};

#endif /* _PARTICLEGROUP_H */
