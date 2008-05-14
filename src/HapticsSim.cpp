// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "HapticsSim.h"

bool HapticsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("HapticsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrismCHAI *obj = new OscPrismCHAI(simulation()->world(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool HapticsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

/****** HapticsSim ******/

HapticsSim::HapticsSim(const char *port)
    : Simulation(port, ST_HAPTICS)
{
    m_pPrismFactory = new HapticsPrismFactory(this);
    m_pSphereFactory = new HapticsSphereFactory(this);

    m_fTimestep = HAPTICS_TIMESTEP_MS/1000.0;
    printf("CHAI timestep: %f\n", m_fTimestep);
}

HapticsSim::~HapticsSim()
{
}

void HapticsSim::initialize()
{
    // create the world object
    m_chaiWorld = new cWorld();
    m_chaiWorld->setBackgroundColor(0.0f,0.0f,0.0f);

    // create the cursor object
    m_chaiCursor = new cMeta3dofPointer(m_chaiWorld, 0);

    // initialize visual step count
    m_nVisualStepCount = 0;

    Simulation::initialize();
}

void HapticsSim::step()
{
    m_chaiCursor->updatePose();

    if (++m_nVisualStepCount >= (VISUAL_TIMESTEP_MS/HAPTICS_TIMESTEP_MS))
    {
        sendtotype(Simulation::ST_VISUAL, true,
                   "/world/cursor/position","fff",
                   m_chaiCursor->getPos().x,
                   m_chaiCursor->getPos().y,
                   m_chaiCursor->getPos().z);
        m_nVisualStepCount = 0;
    }

    m_chaiCursor->computeForces();
    m_chaiCursor->applyForces();
}

/****** CHAIObject ******/

CHAIObject::CHAIObject(cWorld *world)
{
}

CHAIObject::~CHAIObject()
{
}

/****** OscSphereCHAI ******/

OscSphereCHAI::OscSphereCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent), CHAIObject(world)
{
    m_pSphere = new cShapeSphere(0.01);
    world->addChild(m_pSphere);
}

OscSphereCHAI::~OscSphereCHAI()
{
    if (m_pSphere)
        m_pSphere->getParent()->deleteChild(m_pSphere);
}

void OscSphereCHAI::on_radius()
{
    printf("OscSphereCHAI::on_radius(). radius = %f\n", m_radius.m_value);

    if (!m_pSphere)
        return;

    m_pSphere->setRadius(m_radius.m_value);
}

/****** OscPrismCHAI ******/

OscPrismCHAI::OscPrismCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscPrism(NULL, name, parent), CHAIObject(world)
{
    m_pPrism = new cMesh(world);
    m_size.set(0.01, 0.01, 0.01);
    createPrism();
    world->addChild(m_pPrism);
}

OscPrismCHAI::~OscPrismCHAI()
{
    if (m_pPrism)
        m_pPrism->getParent()->deleteChild(m_pPrism);
}

// This function borrowed from dynamic_ode example in CHAI.
void OscPrismCHAI::createPrism(bool openbox)
{
    int n;
    int cur_index = 0;
    int start_index = 0;

    // +x face
    m_pPrism->newVertex( m_size.x/2,  m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex( m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex( m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex( m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex( m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex( m_size.x/2, -m_size.y/2,  m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        cVertex* curVertex = m_pPrism->getVertex(n);
        curVertex->setTexCoord(
            (curVertex->getPos().y + m_size.x/2) / (2.0 * m_size.z/2),
            (curVertex->getPos().z + m_size.x/2) / (2.0 * m_size.y/2)
            );
        curVertex->setNormal(1,0,0);
    }

    start_index += 6;

    // -x face
    m_pPrism->newVertex(-m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2,  m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(-m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2,  m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        cVertex* curVertex = m_pPrism->getVertex(n);
        curVertex->setTexCoord(
            (curVertex->getPos().y + m_size.x/2) / (2.0 * m_size.z/2),
            (curVertex->getPos().z + m_size.x/2) / (2.0 * m_size.y/2)
            );
        curVertex->setNormal(-1,0,0);
    }

    start_index += 6;

    // +y face
    m_pPrism->newVertex(m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(m_size.x/2,  m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(m_size.x/2,  m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, m_size.y/2,  m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        cVertex* curVertex = m_pPrism->getVertex(n);
        curVertex->setTexCoord(
            (curVertex->getPos().x + m_size.y/2) / (2.0 * m_size.z/2),
            (curVertex->getPos().z + m_size.x/2) / (2.0 * m_size.y/2)
            );
        curVertex->setNormal(0,1,0);
    }

    start_index += 6;

    // -y face
    m_pPrism->newVertex(m_size.x/2,  -m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(m_size.x/2,  -m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(m_size.x/2,  -m_size.y/2,  m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2,  m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        cVertex* curVertex = m_pPrism->getVertex(n);
        curVertex->setTexCoord(
            (curVertex->getPos().x + m_size.y/2) / (2.0 * m_size.z/2),
            (curVertex->getPos().z + m_size.x/2) / (2.0 * m_size.y/2)
            );
        curVertex->setNormal(0,-1,0);
    }

    start_index += 6;

    // -z face
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(m_size.x/2,   m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(m_size.x/2,  -m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex( m_size.x/2,  m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, -m_size.z/2);
    m_pPrism->newVertex(-m_size.x/2,  m_size.y/2, -m_size.z/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        cVertex* curVertex = m_pPrism->getVertex(n);
        curVertex->setTexCoord(
            (curVertex->getPos().x + m_size.y/2) / (2.0 * m_size.z/2),
            (curVertex->getPos().y + m_size.x/2) / (2.0 * m_size.y/2)
            );
        curVertex->setNormal(0,0,-1);
    }

    start_index += 6;

    if (!openbox) {

        // +z face
        m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, m_size.z/2);
        m_pPrism->newVertex(m_size.x/2,  -m_size.y/2, m_size.z/2);
        m_pPrism->newVertex(m_size.x/2,  m_size.y/2,  m_size.z/2);
        m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
        cur_index+=3;

        m_pPrism->newVertex(-m_size.x/2, -m_size.y/2, m_size.z/2);
        m_pPrism->newVertex( m_size.x/2,  m_size.y/2, m_size.z/2);
        m_pPrism->newVertex(-m_size.x/2,  m_size.y/2, m_size.z/2);
        m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
        cur_index+=3;

        for(n=start_index; n<cur_index; n++) {
            cVertex* curVertex = m_pPrism->getVertex(n);
            curVertex->setTexCoord(
                (curVertex->getPos().x + m_size.y/2) / (2.0 * m_size.z/2),
                (curVertex->getPos().y + m_size.x/2) / (2.0 * m_size.y/2)
                );
            curVertex->setNormal(0,0,1);
        }

        start_index += 6;
    }

    // Give a color to each vertex
    for (unsigned int i=0; i<m_pPrism->getNumVertices(); i++) {

        cVertex* nextVertex = m_pPrism->getVertex(i);

        cColorb color;
        color.set(
            GLuint(0xff*(m_size.x + nextVertex->getPos().x ) / (2.0 * m_size.x)),
            GLuint(0xff*(m_size.x + nextVertex->getPos().y ) / (2.0 * m_size.y)),
            GLuint(0xff* nextVertex->getPos().z / 2*m_size.z)
            );

        nextVertex->setColor(color);
    }

    // Set object settings.  The parameters tell the object
    // to apply this alpha level to his textures and to his
    // children (of course he has neither right now).
    // object->setTransparencyLevel(0.5, true, true);

    // Give him some material properties...
    m_pPrism->m_material.m_ambient.set( 0.6, 0.3, 0.3, 1.0 );
    m_pPrism->m_material.m_diffuse.set( 0.8, 0.6, 0.6, 1.0 );
    m_pPrism->m_material.m_specular.set( 0.9, 0.9, 0.9, 1.0 );
    m_pPrism->m_material.setShininess(100);
}

void OscPrismCHAI::on_size()
{
    // reposition vertices
    int i,n;
    n = m_pPrism->getNumVertices();
    for (i=0; i<n; i++) {
		 cVector3d pos = m_pPrism->getVertex(i)->getPos();
		 pos.elementMul(cVector3d(1.0/fabs(pos.x), 1.0/fabs(pos.y), 1.0/fabs(pos.z)));
		 pos.elementMul(m_size/2.0);
		 m_pPrism->getVertex(i)->setPos(pos);
    }
}
