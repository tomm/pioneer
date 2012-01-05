#ifndef _PARTICLEGROUP_H
#define _PARTICLEGROUP_H

class ParticleGroup {
public:
	ParticleGroup(int numParticles);
	virtual ~ParticleGroup();
	void Init(int numParticles);
	void Render();
	bool HasActiveParticles() const { return m_hasActiveParticles; }
	
	struct Vertex {
		vector3f pos;
		vector3f vel;
		float texTransform[2];
		float angVelocity;
		float birthTime;
		float duration;
		float pointSize;
	};
	
	void AddParticles(void (*vertexCallback)(Vertex &, int, void*), int num, void *data);
private:
	int m_numParticles;
	bool m_hasActiveParticles;
	Vertex *m_data;
	GLuint m_vbo;
};

#endif /* _PARTICLEGROUP_H */
