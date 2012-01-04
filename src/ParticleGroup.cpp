#include "libs.h"
#include "ParticleGroup.h"
#include "render/Render.h"
#include "TextureManager.h"
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

ParticleGroup::ParticleGroup(TYPE type, int numParticles)
{
	if (!s_shader) {
		s_shader = new ParticleShader("particle", "");
	}
	m_vbo = 0;
	m_data = 0;
	Init(type, numParticles);
}

ParticleGroup::~ParticleGroup()
{
	if (m_vbo) glDeleteBuffersARB(1, &m_vbo);
	if (m_data) delete [] m_data;
}

void ParticleGroup::_TestAddSomeParticles(int num)
{
	int min = m_numParticles;
	int max = 0;
	float now = Pi::GetGameTime();
	for (int i=0; num && (i<m_numParticles); i++) {
		if (m_data[i].birthTime + m_data[i].duration < now) {
			num--;
			min = std::min(min, i);
			max = std::max(max, i);

			m_data[i].pos = vector3f(0.0f);
			m_data[i].vel = vector3f(
					Pi::rng.Double(-25.0, 25.0),
					Pi::rng.Double(-25.0, 25.0),
					Pi::rng.Double(-25.0, 25.0));
			m_data[i].texTransform[0] = 0.25f * (float)Pi::rng.Int32(4);
			m_data[i].texTransform[1] = 0.0f;
			m_data[i].angVelocity = Pi::rng.Double(-10.0, 10.0);
			m_data[i].birthTime = Pi::GetGameTime();
			m_data[i].duration = Pi::rng.Double(1.0, 5.0);
			m_data[i].pointSize = Pi::rng.Double(100.0, 5000.0);
		}
	}
	if (max >= min) {
		//printf("%d KiB update\n", (max-min)*sizeof(Vertex)/1024);
		Render::BindArrayBuffer(m_vbo);
		glBufferSubDataARB(GL_ARRAY_BUFFER, min * sizeof(Vertex), (1+max-min) * sizeof(Vertex), &m_data[min]);
		Render::BindArrayBuffer(0);
	}
}

void ParticleGroup::Init(TYPE type, int numParticles)
{
	m_type = type;
	m_numParticles = numParticles;
	if (m_data) delete [] m_data;
	if (m_vbo) glDeleteBuffersARB(1, &m_vbo);

	m_data = new Vertex[numParticles];

	for (int i=0; i<numParticles; i++) {
		m_data[i].birthTime = 0;
		m_data[i].duration = 0;
	}

	glGenBuffersARB(1, &m_vbo);
	Render::BindArrayBuffer(m_vbo);
	glBufferDataARB(GL_ARRAY_BUFFER, sizeof(Vertex)*numParticles, m_data, GL_DYNAMIC_DRAW);
	Render::BindArrayBuffer(0);
}

void ParticleGroup::Render()
{
	int maxActive = -1;
	const float now = (float)Pi::GetGameTime();
	for (int i=m_numParticles-1; i>=0; i--) {
		if (m_data[i].birthTime + m_data[i].duration > now) {
			maxActive = i;
			break;
		}
	}
	if (maxActive == -1) return;
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
	s_shader->set_time(now);
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
	glDrawArrays(GL_POINTS, 0, maxActive+1);
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
