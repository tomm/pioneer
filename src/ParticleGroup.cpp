#include "libs.h"
#include "ParticleGroup.h"
#include "render/Render.h"
#include "TextureManager.h"

#warning can remove when debug code gone
#include "Pi.h"

SHADER_CLASS_BEGIN(ParticleShader)
	SHADER_VERTEX_ATTRIB_FLOAT3(position)
	SHADER_VERTEX_ATTRIB_FLOAT3(velocity)
	SHADER_VERTEX_ATTRIB_FLOAT3(texTransform)
	SHADER_VERTEX_ATTRIB_FLOAT(birthTime)
	SHADER_UNIFORM_VEC3(acceleration)
	SHADER_UNIFORM_VEC4(color)
	SHADER_UNIFORM_FLOAT(time)
	SHADER_UNIFORM_SAMPLER(texture)
SHADER_CLASS_END()
	
static ParticleShader *s_shader;

#define DEBUG_EFFECT_DURATION	2.0
double debug_age = 0.0;

ParticleGroup::ParticleGroup(TYPE type, int numParticles)
{
	if (!s_shader) {
		s_shader = new ParticleShader("particle", "");
	}
	m_vbo = 0;
	Init(type, numParticles);
}

void ParticleGroup::Init(TYPE type, int numParticles)
{
	m_type = type;
	m_numParticles = numParticles;
	if (m_vbo) glDeleteBuffersARB(1, &m_vbo);

	Vertex *data = new Vertex[numParticles];

	for (int i=0; i<numParticles; i++) {
		data[i].pos = vector3f(0.0f);
		data[i].vel = vector3f(
				Pi::rng.Double(-50.0, 50.0),
				Pi::rng.Double(-50.0, 50.0),
				Pi::rng.Double(-50.0, 50.0));
		data[i].texTransform = vector3f(0.25f * (float)Pi::rng.Int32(4), 0.0f, 0.0f);
		data[i].birthTime = Pi::GetGameTime();
	}
	debug_age = Pi::GetGameTime();

	glGenBuffersARB(1, &m_vbo);
	Render::BindArrayBuffer(m_vbo);
	glBufferDataARB(GL_ARRAY_BUFFER, sizeof(Vertex)*numParticles, data, GL_STATIC_DRAW);
	Render::BindArrayBuffer(0);

	delete [] data;
}

void ParticleGroup::Render()
{
	if (Pi::GetGameTime() - debug_age > DEBUG_EFFECT_DURATION) {
		Init(m_type, m_numParticles);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glPointSize(10.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	
	Texture *tex = TextureManager::GetTexture(PIONEER_DATA_DIR"/textures/particles.png");
	tex->BindTexture();
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);


#warning FF fallback needed
	glEnable(GL_POINT_SPRITE_ARB);

	Render::State::UseProgram(s_shader);
	Render::BindArrayBuffer(m_vbo);
//	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
	// offsets in array buffer
	s_shader->set_color(1.0f, 1.0f, 0.0f, 1.0f);
	s_shader->set_texture(0);
	s_shader->set_time((float)Pi::GetGameTime());
	s_shader->set_acceleration(0.0f, -9.8f, 0.0f);
	s_shader->set_position((float*)0, sizeof(Vertex));
	s_shader->set_velocity((float*)(sizeof(float)*3), sizeof(Vertex));
	s_shader->set_texTransform((float*)(sizeof(float)*6), sizeof(Vertex));
	s_shader->set_birthTime((float*)(sizeof(float)*9), sizeof(Vertex));

	s_shader->enable_attrib_position();
	s_shader->enable_attrib_velocity();
	s_shader->enable_attrib_texTransform();
	s_shader->enable_attrib_birthTime();
	glDrawArrays(GL_POINTS, 0, m_numParticles);
	s_shader->disable_attrib_position();
	s_shader->disable_attrib_velocity();
	s_shader->disable_attrib_texTransform();
	s_shader->disable_attrib_birthTime();

	Render::BindArrayBuffer(0);
	Render::State::UseProgram(0);
	
	glDisable(GL_POINT_SPRITE_ARB);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisableClientState(GL_VERTEX_ARRAY);
}
