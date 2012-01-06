#include "libs.h"
#include "Pi.h"
#include "Game.h"
#include "Player.h"
#include "Sfx.h"
#include "Frame.h"
#include "StarSystem.h"
#include "Space.h"
#include "TextureManager.h"
#include "render/Render.h"
#include "ParticleGroup.h"

#define MAX_SFX_PER_FRAME 4
#define MAX_PARTICLES_PER_GROUP 65536
// Distance from the player (in meters) at which the ParticleGroup is expired
// This gives ~1cm precision (particle positions are 32-bit floats in VBO)
// This is necessary to avoid jitter that positions relative to frame origin would cause
// Once a ParticleGroup is expired (m_dead = true), it is still rendered until all particles
// have 'died' of old age, then it is freed.
// See Sfx::GetActiveParticleGroup()
#define EXPIRY_DISTANCE_FROM_PLAYER 1e3 //  change to 1e5!!!

Sfx::Sfx()
{
	m_particles = 0;
}

Sfx::~Sfx()
{
	if (m_particles) delete m_particles;
}

void Sfx::Save(Serializer::Writer &wr)
{
	wr.Vector3d(m_pos);
	wr.Vector3d(m_oldPos);
	wr.Vector3d(m_vel);
	wr.Bool(m_dead);
}

void Sfx::Load(Serializer::Reader &rd)
{
	m_pos = rd.Vector3d();
	m_oldPos = rd.Vector3d();
	m_vel = rd.Vector3d();
	m_dead = rd.Bool();
#warning FINISH THIS!
}

void Sfx::Serialize(Serializer::Writer &wr, const Frame *f)
{
	// how many sfx turds are active in frame?
	int numActive = 0;
	if (f->m_sfx) {
		for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
			if (f->m_sfx[i].m_particles) numActive++;
		}
	}
	wr.Int32(numActive);

	if (numActive) for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
		if (f->m_sfx[i].m_particles) {
			f->m_sfx[i].Save(wr);
		}
	}
}

void Sfx::Unserialize(Serializer::Reader &rd, Frame *f)
{
	int numActive = rd.Int32();
	if (numActive) {
		f->m_sfx = new Sfx[MAX_SFX_PER_FRAME];
		for (int i=0; i<numActive; i++) {
			f->m_sfx[i].Load(rd);
		}
	}
}

void Sfx::TimeStepUpdate(const float timeStep)
{
	m_oldPos = m_pos;
	m_pos += m_vel * double(timeStep);

	if ((m_pos - Pi::player->GetPosition()).Length() > EXPIRY_DISTANCE_FROM_PLAYER) {
		m_dead = true;

		if (!m_particles->HasActiveParticles()) {
			printf("Expired dead group %p\n", this);
			delete m_particles;
			m_particles = 0;
		}
	}
}

void Sfx::Render(const matrix4x4d &ftransform)
{
	if (m_particles) {
		const double alpha = Pi::GetGameTickAlpha();
		const vector3d fpos = ftransform * (m_pos*alpha + m_oldPos*(1.0-alpha));
		matrix4x4d tran = ftransform;
		tran[12] = fpos.x;
		tran[13] = fpos.y;
		tran[14] = fpos.z;
		
		glPushMatrix();
		glLoadMatrixd(&tran[0]);
		m_particles->Render();
		glPopMatrix();
	}
/*
	Texture *smokeTex = 0;
	float col[4];

	switch (m_type) {
		case TYPE_NONE: break;
		case TYPE_EXPLOSION:
			glPushMatrix();
			glTranslatef(fpos.x, fpos.y, fpos.z);
			glPushAttrib(GL_LIGHTING_BIT | GL_COLOR_BUFFER_BIT);
			glDisable(GL_LIGHTING);
			glColor3f(1,1,0.5);
			gluSphere(Pi::gluQuadric, 1000*m_age, 20,20);
			glEnable(GL_BLEND);
			glColor4f(1,0.5,0,0.66);
			gluSphere(Pi::gluQuadric, 1500*m_age, 20,20);
			glColor4f(1,0,0,0.33);
			gluSphere(Pi::gluQuadric, 2000*m_age, 20,20);
			glPopAttrib();
			glPopMatrix();
			break;
		case TYPE_DAMAGE:
			col[0] = 1.0f;
			col[1] = 1.0f;
			col[2] = 0.0f;
			col[3] = 1.0f-(m_age/2.0f);
			vector3f pos(&fpos.x);
			smokeTex = TextureManager::GetTexture(PIONEER_DATA_DIR"/textures/smoke.png");
			smokeTex->BindTexture();
			Render::PutPointSprites(1, &pos, 20.0f, col);
			break;
	}*/
}

Sfx *Sfx::GetActiveParticleGroup(Frame *f)
{
	Sfx *newPg = 0;
	if (!f->m_sfx) {
		f->m_sfx = new Sfx[MAX_SFX_PER_FRAME];
	}

	/* Try to find active particle group */
	for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
		if (f->m_sfx[i].m_particles && !f->m_sfx[i].m_dead) {
			return &f->m_sfx[i];
		}
	}

	/* failing that, create one */
	for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
		if (!f->m_sfx[i].m_particles) {
			newPg = &f->m_sfx[i];
			newPg->m_particles = new ParticleGroup(MAX_PARTICLES_PER_GROUP);
			printf("Creating new active group %d\n", i);
			break;
		}
	}

	/* failing that, there must be a lot of dead particle groups
	   taking too long to expire. Nuke one */
	if (!newPg) {
		newPg = &f->m_sfx[0];
		delete newPg->m_particles;
		newPg->m_particles = new ParticleGroup(MAX_PARTICLES_PER_GROUP);
		printf("Replacing dead group group %d\n", 0);
	}

	/* Put new particle groups near the player, travelling at player velocity */
	newPg->m_pos = Pi::player->GetPosition();
	newPg->m_vel = Pi::player->GetVelocity();
	newPg->m_oldPos = newPg->m_pos;
	newPg->m_dead = false;

	return newPg;
}

static void myCallback(ParticleGroup::Vertex &v, int num, void *sfx)
{
	v.pos = vector3f(Pi::player->GetPosition() - ((Sfx*)sfx)->GetPosition());
	v.vel = vector3f(Pi::player->GetVelocity() - ((Sfx*)sfx)->GetVelocity());
	v.vel += vector3f(
			Pi::rng.Double(-10.0,10.0),
			Pi::rng.Double(-10.0,10.0),
			Pi::rng.Double(-10.0,10.0));
	v.texTransform[0] = 0.25f * (float)Pi::rng.Int32(4);
	v.texTransform[1] = 0.0f;
	v.angVelocity = Pi::rng.Double(-10.0, 10.0);
	v.birthTime = Pi::game->GetTime();
	v.duration = Pi::rng.Double(1.0, 5.0);
	v.pointSize = Pi::rng.Double(100.0, 5000.0);
}

void Sfx::Add(const Body *b, TYPE t)
{
	Sfx *sfx = GetActiveParticleGroup(b->GetFrame());

	sfx->m_particles->AddParticles(myCallback, 50, sfx);
	//		vector3f(b->GetPosition() - sfx->m_pos), 1);
}

void Sfx::TimeStepAll(const float timeStep, Frame *f)
{
	if (f->m_sfx) {
		for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
			if (f->m_sfx[i].m_particles) {
				f->m_sfx[i].TimeStepUpdate(timeStep);
			}
		}
	}
	
	for (std::list<Frame*>::iterator i = f->m_children.begin();
			i != f->m_children.end(); ++i) {
		TimeStepAll(timeStep, *i);
	}
}

void Sfx::RenderAll(const Frame *f, const Frame *camFrame)
{
	if (f->m_sfx) {
		matrix4x4d ftran;
		Frame::GetFrameTransform(f, camFrame, ftran);

		for (int i=0; i<MAX_SFX_PER_FRAME; i++) {
			if (f->m_sfx[i].m_particles) {
				f->m_sfx[i].Render(ftran);
			}
		}
	}
	
	for (std::list<Frame*>::const_iterator i = f->m_children.begin();
			i != f->m_children.end(); ++i) {
		RenderAll(*i, camFrame);
	}
}
