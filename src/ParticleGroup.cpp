#include "libs.h"
#include "ParticleGroup.h"
#include "render/Render.h"
#include "TextureManager.h"

#warning can remove when debug code gone
#include "Pi.h"

SHADER_CLASS_BEGIN(ParticleShader)
	SHADER_VERTEX_ATTRIB_FLOAT3(position)
	SHADER_VERTEX_ATTRIB_FLOAT3(velocity)
	SHADER_VERTEX_ATTRIB_FLOAT2(texTransform)
	SHADER_VERTEX_ATTRIB_FLOAT(angVelocity)
	SHADER_VERTEX_ATTRIB_FLOAT(birthTime)
	SHADER_VERTEX_ATTRIB_FLOAT(duration)
	SHADER_VERTEX_ATTRIB_FLOAT(pointSize)
	SHADER_UNIFORM_VEC3(acceleration)
	SHADER_UNIFORM_VEC4(color)
	SHADER_UNIFORM_FLOAT(time)
	SHADER_UNIFORM_SAMPLER(texture)
SHADER_CLASS_END()
	
static ParticleShader *s_shader;

#define DEBUG_EFFECT_DURATION	5.0
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
				Pi::rng.Double(-25.0, 25.0),
				Pi::rng.Double(-25.0, 25.0),
				Pi::rng.Double(-25.0, 25.0));
		data[i].texTransform[0] = 0.25f * (float)Pi::rng.Int32(4);
		data[i].texTransform[1] = 0.0f;
		data[i].angVelocity = Pi::rng.Double(-10.0, 10.0);
		data[i].birthTime = Pi::GetGameTime();
		data[i].duration = Pi::rng.Double(0.1, 5.0);
		data[i].pointSize = Pi::rng.Double(10.0, 5000.0);
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
	glDepthMask(GL_FALSE);
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
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);

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
	s_shader->set_angVelocity((float*)(sizeof(float)*8), sizeof(Vertex));
	s_shader->set_birthTime((float*)(sizeof(float)*9), sizeof(Vertex));
	s_shader->set_duration((float*)(sizeof(float)*10), sizeof(Vertex));
	s_shader->set_pointSize((float*)(sizeof(float)*11), sizeof(Vertex));

	s_shader->enable_attrib_position();
	s_shader->enable_attrib_velocity();
	s_shader->enable_attrib_texTransform();
	s_shader->enable_attrib_angVelocity();
	s_shader->enable_attrib_birthTime();
	s_shader->enable_attrib_duration();
	s_shader->enable_attrib_pointSize();
	glDrawArrays(GL_POINTS, 0, m_numParticles);
	s_shader->disable_attrib_position();
	s_shader->disable_attrib_velocity();
	s_shader->disable_attrib_texTransform();
	s_shader->disable_attrib_angVelocity();
	s_shader->disable_attrib_birthTime();
	s_shader->disable_attrib_duration();
	s_shader->disable_attrib_pointSize();

	Render::BindArrayBuffer(0);
	Render::State::UseProgram(0);
	
	glDepthMask(GL_TRUE);
	glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_ARB);
	glDisable(GL_POINT_SPRITE_ARB);
	glEnable(GL_LIGHTING);
	glDisableClientState(GL_VERTEX_ARRAY);
}
