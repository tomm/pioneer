#include "libs.h"
#include "CityOnPlanet.h"
#include "Frame.h"
#include "SpaceStation.h"
#include "Planet.h"
#include "Pi.h"
#include "collider/Geom.h"
#include "Render.h"

#define START_SEG_SIZE CITY_ON_PLANET_RADIUS
#define MIN_SEG_SIZE 50.0

bool s_cityBuildingsInitted = false;
struct citybuilding_t {
	const char *modelname;
	float xzradius;
	LmrModel *resolvedModel;
	const LmrCollMesh *collMesh;
};

#define MAX_BUILDING_LISTS 3
struct citybuildinglist_t {
	const char *modelTagName;
	double minRadius, maxRadius;
	int numBuildings;
	citybuilding_t *buildings;
};

citybuildinglist_t s_buildingLists[MAX_BUILDING_LISTS] = {
	{ "city_building", 0.2, 0.9 },
	{ "city_power", 0.1, 0.2 },
	{ "city_starport_building", 0.3, 0.5 },
};

#define CITYFLAVOURS 5
struct cityflavourdef_t {
	int buildingListIdx;
	vector3d center;
	double size;
} cityflavour[CITYFLAVOURS];


static Plane planes[6];
LmrObjParams cityobj_params;

void CityOnPlanet::PutCityBit(MTRand &rand, const matrix4x4d &rot, vector3d p1, vector3d p2, vector3d p3, vector3d p4)
{
	double rad = (p1-p2).Length()*0.5;
	LmrModel *model = 0;
	double modelRadXZ;
	const LmrCollMesh *cmesh;
	vector3d cent = (p1+p2+p3+p4)*0.25;

	cityflavourdef_t *flavour;
	citybuildinglist_t *buildings;

	// pick a building flavour (city, windfarm, etc)
	for (int flv=0; flv<CITYFLAVOURS; flv++) {
		flavour = &cityflavour[flv];
		buildings = &s_buildingLists[flavour->buildingListIdx];
       
		bool tooDistant = ((flavour->center - cent).Length()*(1.0/flavour->size) > rand.Double());
		if (tooDistant) continue;
		
		int tries;
		for (tries=20; tries--; ) {
			const citybuilding_t &bt = buildings->buildings[rand.Int32(buildings->numBuildings)];
			if (bt.xzradius < rad) {
				// don't put building on excessive slope
				// XXX fucking slow
			/*	vector3d corner[4];
				corner[0] = cent + (p1-cent).Normalized()*bt.xzradius;
				corner[1] = cent + (p2-cent).Normalized()*bt.xzradius;
				corner[2] = cent + (p3-cent).Normalized()*bt.xzradius;
				corner[3] = cent + (p4-cent).Normalized()*bt.xzradius;

				double h_min = FLT_MAX;
				double h_max = 0;
				double h = m_planet->GetTerrainHeight(corner[0].Normalized());
				h_max = std::max(h_max, h); h_min = std::min(h_min, h);
				h = m_planet->GetTerrainHeight(corner[1].Normalized());
				h_max = std::max(h_max, h); h_min = std::min(h_min, h);
				h = m_planet->GetTerrainHeight(corner[2].Normalized());
				h_max = std::max(h_max, h); h_min = std::min(h_min, h);
				h = m_planet->GetTerrainHeight(corner[2].Normalized());
				h_max = std::max(h_max, h); h_min = std::min(h_min, h);
				// don't build it on an excessive gradient
				if ((h_max - h_min)/bt.xzradius < 0.1) { */
					model = bt.resolvedModel;
					modelRadXZ = bt.xzradius;
					cmesh = bt.collMesh;
					break;
				//}
			}
		}
		if (model) break;
	}

	if (model == 0) {
		if (rad > MIN_SEG_SIZE) goto always_divide;
		else return;
	}

	if (rad > modelRadXZ*2.0) {
always_divide:
		vector3d a = (p1+p2)*0.5;
		vector3d b = (p2+p3)*0.5;
		vector3d c = (p3+p4)*0.5;
		vector3d d = (p4+p1)*0.5;
		vector3d e = (p1+p2+p3+p4)*0.25;
		PutCityBit(rand, rot, p1, a, e, d);
		PutCityBit(rand, rot, a, p2, b, e);
		PutCityBit(rand, rot, e, b, p3, c);
		PutCityBit(rand, rot, d, e, c, p4);
	} else {
		double height_max = m_planet->GetTerrainHeight(cent.Normalized());
		double height_min = FLT_MAX;
		double h = m_planet->GetTerrainHeight(p1.Normalized());
		height_max = std::max(height_max, h); height_min = std::min(height_min, h);
		h = m_planet->GetTerrainHeight(p2.Normalized());
		height_max = std::max(height_max, h); height_min = std::min(height_min, h);
		h = m_planet->GetTerrainHeight(p3.Normalized());
		height_max = std::max(height_max, h); height_min = std::min(height_min, h);
		h = m_planet->GetTerrainHeight(p4.Normalized());
		height_max = std::max(height_max, h); height_min = std::min(height_min, h);
		// building clearance of 1 meter above terrain
		height_max += 1.0;

		p1 = cent + (p1-cent).Normalized()*modelRadXZ;
		p2 = cent + (p2-cent).Normalized()*modelRadXZ;
		p3 = cent + (p3-cent).Normalized()*modelRadXZ;
		p4 = cent + (p4-cent).Normalized()*modelRadXZ;

		/* don't position below sealevel! */
		if (height_max - m_planet->GetSBody()->GetRadius() == 0.0) return;
		cent = cent.Normalized() * height_max;

		Geom *geom = new Geom(cmesh->geomTree);
		int rotTimes90 = rand.Int32(4);
		matrix4x4d grot = rot * matrix4x4d::RotateYMatrix(M_PI*0.5*(double)rotTimes90);
		geom->MoveTo(grot, cent);
		geom->SetUserData(this);
//		f->AddStaticGeom(geom);

		BuildingDef def = { model, cmesh->GetBoundingRadius(), rotTimes90, cent, geom, false };
		def.rect[0] = p1.Normalized();
		def.rect[1] = p2.Normalized();
		def.rect[2] = p3.Normalized();
		def.rect[3] = p4.Normalized();
		def.height = height_max;
		m_buildings.push_back(def);
	}
}

void CityOnPlanet::AddStaticGeomsToCollisionSpace()
{
	int skipMask;
	switch (Pi::detail.cities) {
		case 0: skipMask = 0x3f; break;
		case 1: skipMask = 0xf; break;
		case 2: skipMask = 0x7; break;
		case 3: skipMask = 0x3; break;
		default:
			skipMask = 0; break;
	}
	for (unsigned int i=0; i<m_buildings.size(); i++) {
		if (i & skipMask) {
			m_buildings[i].isEnabled = false;
		} else {
			m_frame->AddStaticGeom(m_buildings[i].geom);
			m_buildings[i].isEnabled = true;
		}
	}
	m_detailLevel = Pi::detail.cities;
}

void CityOnPlanet::MakeBuildingBaseGeometry()
{
	const int num_vertices = m_buildings.size()*60;
	vector3f * const vertices = new vector3f[num_vertices];
	vector3f *vtxpos = vertices;
	for (std::vector<BuildingDef>::const_iterator i = m_buildings.begin();
			i != m_buildings.end(); ++i) {
		// a is rectangle bounding building on planet surface
		// and b is same rectangle but at building base height
		vector3f a[4], b[4];
		for (int j=0; j<4; j++) {
			a[j] = (*i).rect[j] * m_planet->GetTerrainHeight((*i).rect[j]);
			b[j] = (*i).rect[j] * (*i).height;
		}

		vector3f norm = ((a[0]-b[0]).Cross(a[0]-a[1])).Normalized();
		(*vtxpos++) = a[0]; (*vtxpos++) = norm;
		(*vtxpos++) = b[0]; (*vtxpos++) = norm;
		(*vtxpos++) = a[1]; (*vtxpos++) = norm;

		(*vtxpos++) = a[1]; (*vtxpos++) = norm;
		(*vtxpos++) = b[0]; (*vtxpos++) = norm;
		(*vtxpos++) = b[1]; (*vtxpos++) = norm;

		norm = ((a[1]-b[1]).Cross(a[1]-a[2])).Normalized();
		(*vtxpos++) = a[1]; (*vtxpos++) = norm;
		(*vtxpos++) = b[1]; (*vtxpos++) = norm;
		(*vtxpos++) = a[2]; (*vtxpos++) = norm;

		(*vtxpos++) = a[2]; (*vtxpos++) = norm;
		(*vtxpos++) = b[1]; (*vtxpos++) = norm;
		(*vtxpos++) = b[2]; (*vtxpos++) = norm;

		norm = ((a[2]-b[2]).Cross(a[2]-a[3])).Normalized();
		(*vtxpos++) = a[2]; (*vtxpos++) = norm;
		(*vtxpos++) = b[2]; (*vtxpos++) = norm;
		(*vtxpos++) = a[3]; (*vtxpos++) = norm;

		(*vtxpos++) = a[3]; (*vtxpos++) = norm;
		(*vtxpos++) = b[2]; (*vtxpos++) = norm;
		(*vtxpos++) = b[3]; (*vtxpos++) = norm;

		norm = ((a[3]-b[3]).Cross(a[3]-a[0])).Normalized();
		(*vtxpos++) = a[3]; (*vtxpos++) = norm;
		(*vtxpos++) = b[3]; (*vtxpos++) = norm;
		(*vtxpos++) = a[0]; (*vtxpos++) = norm;

		(*vtxpos++) = a[0]; (*vtxpos++) = norm;
		(*vtxpos++) = b[3]; (*vtxpos++) = norm;
		(*vtxpos++) = b[0]; (*vtxpos++) = norm;

		norm = ((b[0]-b[2]).Cross(b[0]-b[1])).Normalized();
		(*vtxpos++) = b[0]; (*vtxpos++) = norm;
		(*vtxpos++) = b[2]; (*vtxpos++) = norm;
		(*vtxpos++) = b[1]; (*vtxpos++) = norm;

		(*vtxpos++) = b[0]; (*vtxpos++) = norm;
		(*vtxpos++) = b[3]; (*vtxpos++) = norm;
		(*vtxpos++) = b[2]; (*vtxpos++) = norm;
	}
	assert (vtxpos == vertices+num_vertices);

	if (!m_buildingBaseGeometryVBO) {
		glGenBuffersARB(1, &m_buildingBaseGeometryVBO);
	}
	Render::BindArrayBuffer(m_buildingBaseGeometryVBO);
	glBufferDataARB(GL_ARRAY_BUFFER, sizeof(vector3f)*num_vertices, &vertices[0].x, GL_STATIC_DRAW);
	Render::UnbindAllBuffers();

	delete vertices;
}

void CityOnPlanet::RemoveStaticGeomsFromCollisionSpace()
{
	for (unsigned int i=0; i<m_buildings.size(); i++) {
		m_frame->RemoveStaticGeom(m_buildings[i].geom);
		m_buildings[i].isEnabled = false;
	}
}

static void lookupBuildingListModels(citybuildinglist_t *list)
{
	//const char *modelTagName;
	std::vector<LmrModel*> models;
	LmrGetModelsWithTag(list->modelTagName, models);
	printf("Got %d buildings of tag %s\n", models.size(), list->modelTagName);
	list->buildings = new citybuilding_t[models.size()];
	list->numBuildings = models.size();

	int i = 0;
	for (std::vector<LmrModel*>::iterator m = models.begin(); m != models.end(); ++m, i++) {
		list->buildings[i].resolvedModel = *m;
		const LmrCollMesh *collMesh = new LmrCollMesh(*m, &cityobj_params);
		list->buildings[i].collMesh = collMesh;
		float maxx = std::max(fabs(collMesh->GetAabb().max.x), fabs(collMesh->GetAabb().min.x));
		float maxy = std::max(fabs(collMesh->GetAabb().max.z), fabs(collMesh->GetAabb().min.z));
		list->buildings[i].xzradius = sqrt(maxx*maxx + maxy*maxy);
		//printf("%s: %f\n", list->buildings[i].modelname, list->buildings[i].xzradius);
	}
}

CityOnPlanet::~CityOnPlanet()
{
	// frame may be null (already removed from 
	for (unsigned int i=0; i<m_buildings.size(); i++) {
		m_frame->RemoveStaticGeom(m_buildings[i].geom);
		delete m_buildings[i].geom;
	}
	if (m_buildingBaseGeometryVBO) glDeleteBuffersARB(1, &m_buildingBaseGeometryVBO);
}

CityOnPlanet::CityOnPlanet(const Planet *planet, const SpaceStation *station, Uint32 seed)
{
	m_buildings.clear();
	m_planet = planet;
	m_frame = planet->GetFrame();
	m_detailLevel = Pi::detail.cities;
	m_buildingBaseGeometryVBO = 0;

	/* Resolve city model numbers since it is a bit expensive */
	if (!s_cityBuildingsInitted) {
		s_cityBuildingsInitted = true;
		for (int i=0; i<MAX_BUILDING_LISTS; i++) {
			lookupBuildingListModels(&s_buildingLists[i]);
		}
	}

	Aabb aabb;
	station->GetAabb(aabb);
	
	matrix4x4d m;
	station->GetRotMatrix(m);

	vector3d mx = m*vector3d(1,0,0);
	vector3d mz = m*vector3d(0,0,1);
		
	MTRand rand;
	rand.seed(seed);

	vector3d p = station->GetPosition();

	vector3d p1, p2, p3, p4;
	const double sizex = START_SEG_SIZE;// + rand.Int32((int)START_SEG_SIZE);
	const double sizez = START_SEG_SIZE;// + rand.Int32((int)START_SEG_SIZE);
	
	// always have random shipyard buildings around the space station
	cityflavour[0].buildingListIdx = 2;
	cityflavour[0].center = p;
	cityflavour[0].size = 500;

	for (int i=1; i<CITYFLAVOURS; i++) {
		cityflavour[i].buildingListIdx = rand.Int32(MAX_BUILDING_LISTS-1);
		citybuildinglist_t *blist = &s_buildingLists[cityflavour[i].buildingListIdx];
		int a, b;
		a = rand.Double(-0.5*sizex,0.5*sizex);
		b = rand.Double(-0.5*sizez,0.5*sizez);
		cityflavour[i].center = p + (double)a*mx + (double)b*mz;
		cityflavour[i].size = (double)CITY_ON_PLANET_RADIUS * rand.Double(blist->minRadius, blist->maxRadius);
	}
	
	for (int side=0; side<4; side++) {
		/* put buildings on all sides of spaceport */
		switch(side) {
			case 3:
				p1 = p + mx*(aabb.min.x) + mz*aabb.min.z;
				p2 = p + mx*(aabb.min.x) + mz*(aabb.min.z-sizez);
				p3 = p + mx*(aabb.min.x+sizex) + mz*(aabb.min.z-sizez);
				p4 = p + mx*(aabb.min.x+sizex) + mz*(aabb.min.z);
				break;
			case 2:
				p1 = p + mx*(aabb.min.x-sizex) + mz*aabb.max.z;
				p2 = p + mx*(aabb.min.x-sizex) + mz*(aabb.max.z-sizez);
				p3 = p + mx*(aabb.min.x) + mz*(aabb.max.z-sizez);
				p4 = p + mx*(aabb.min.x) + mz*(aabb.max.z);
				break;
			case 1:
				p1 = p + mx*(aabb.max.x-sizex) + mz*aabb.max.z;
				p2 = p + mx*(aabb.max.x) + mz*aabb.max.z;
				p3 = p + mx*(aabb.max.x) + mz*(aabb.max.z+sizez);
				p4 = p + mx*(aabb.max.x-sizex) + mz*(aabb.max.z+sizez);
				break;
			default:
			case 0:
				p1 = p + mx*aabb.max.x + mz*aabb.min.z;
				p2 = p + mx*(aabb.max.x+sizex) + mz*aabb.min.z;
				p3 = p + mx*(aabb.max.x+sizex) + mz*(aabb.min.z+sizez);
				p4 = p + mx*aabb.max.x + mz*(aabb.min.z+sizez);
				break;
		}

		vector3d center = (p1+p2+p3+p4)*0.25;
		PutCityBit(rand, m, p1, p2, p3, p4);
	}
	AddStaticGeomsToCollisionSpace();
	MakeBuildingBaseGeometry();
}

void CityOnPlanet::Render(const SpaceStation *station, const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	const int num_buildings = m_buildings.size();
	matrix4x4d rot[4];
	station->GetRotMatrix(rot[0]);

	// change detail level if necessary
	if (m_detailLevel != Pi::detail.cities) {
		RemoveStaticGeomsFromCollisionSpace();
		AddStaticGeomsToCollisionSpace();
	}
	
	rot[0] = viewTransform * rot[0];
	for (int i=1; i<4; i++) {
		rot[i] = rot[0] * matrix4x4d::RotateYMatrix(M_PI*0.5*(double)i);
	}

	GetFrustum(planes);
	
	memset(&cityobj_params, 0, sizeof(LmrObjParams));
	// this fucking rubbish needs to be moved into a function
	cityobj_params.argFloats[1] = (float)Pi::GetGameTime();
	cityobj_params.argFloats[2] = (float)(Pi::GetGameTime() / 60.0);
	cityobj_params.argFloats[3] = (float)(Pi::GetGameTime() / 3600.0);
	cityobj_params.argFloats[4] = (float)(Pi::GetGameTime() / (24*3600.0));

	// building bases (square foundation things)
	Pi::statSceneTris += 10*num_buildings;
	glPushMatrix();
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glMultMatrixd(&viewTransform[0]);
		const float col[4] = {0.5,0.5,0.5,1.0};
		const float black[4] = {0,0,0,0};
		glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, col);
		glMaterialfv (GL_FRONT, GL_SPECULAR, black);
		glMaterialfv (GL_FRONT, GL_EMISSION, black);
		Render::BindArrayBuffer(m_buildingBaseGeometryVBO);
		glVertexPointer(3, GL_FLOAT, sizeof(float)*6, 0);
		glNormalPointer(GL_FLOAT, sizeof(float)*6, (void*)(3*sizeof(float)));
		glDrawArrays(GL_TRIANGLES, 0, num_buildings*30);
		Render::UnbindAllBuffers();
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	glPopMatrix();

	// and render the buildings themselves
	for (std::vector<BuildingDef>::const_iterator i = m_buildings.begin();
			i != m_buildings.end(); ++i) {

		if (!(*i).isEnabled) continue;

		vector3d pos = viewTransform * (*i).pos;
		/* frustum cull */
		bool cull = false;
		for (int j=0; j<6; j++) {
			if (planes[j].DistanceToPoint(pos)+(*i).clipRadius < 0) {
				cull = true;
				break;
			}
		}
		if (cull) continue;
		matrix4x4f _rot;
		for (int e=0; e<16; e++) _rot[e] = rot[(*i).rotation][e];
		_rot[12] = pos.x;
		_rot[13] = pos.y;
		_rot[14] = pos.z;
		(*i).model->Render(_rot, &cityobj_params);
	}
}

