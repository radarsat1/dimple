// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include "dimple.h"
#include "HapticsSim.h"
#include "CODEPotentialProxy.h"

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

bool HapticsMeshFactory::create(const char *name, const char *filename,
                                float x, float y, float z)
{
    printf("HapticsMeshFactory (%s) is creating a mesh "
           "object called '%s' (%s)\n", m_parent->c_name(), name, filename);

    OscMeshCHAI *obj = new OscMeshCHAI(simulation()->world(),
                                       name, filename, m_parent);

    if (!obj->object()) {
        delete obj;
        obj = NULL;
    }

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
    m_pMeshFactory = new HapticsMeshFactory(this);

    m_fTimestep = haptics_timestep_ms/1000.0;
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

    // create an OscObject to point to the cursor
    m_cursor = new OscCursorCHAI(m_chaiWorld, "cursor", this);
    m_chaiWorld->addChild(m_cursor->object());

    // special case:
    // we know that the libnifalcon driver times itself, so don't
    // allow the Simulation to time itself before each step().
#ifdef DEVICE_LIBNIFALCON
    if (m_cursor->object()->getPhysicalDevice() == DEVICE_LIBNIFALCON)
        m_bSelfTimed = false;
#endif

    // quit the haptics simulation if the cursor couldn't be initialized.
    if (!m_cursor->is_initialized())
        m_bDone = true;

    // ensure workspace is recalibrated
    m_resetWorkspace = true;

    // initialize step count
    m_counter = 0;

    m_pGrabbedObject = NULL;

    Simulation::initialize();
}

void HapticsSim::updateWorkspace(cVector3d &pos)
{
    int i;
    if (m_resetWorkspace) {
        m_workspace[0] = pos;
        m_workspace[1] = pos;
        m_resetWorkspace = false;
    }

    for (i=0; i<3; i++) {
        // Update workspace boundaries
        if (pos[i] < m_workspace[0][i])
            m_workspace[0][i] = pos[i];
        if (pos[i] > m_workspace[1][i])
            m_workspace[1][i] = pos[i];

        float dif = (m_workspace[1][i] - m_workspace[0][i]);
        if (dif != 0.0)
            m_workspaceScale[i] = 2.0/(m_workspace[1][i] - m_workspace[0][i]);
        else
            m_workspaceScale[i] = 1;
        m_workspaceOffset[i] = -(m_workspace[1][i] + m_workspace[0][i]) / 2.0;

        // Normalize position to [-1, 1] within workspace.
        pos[i] = (pos[i] + m_workspaceOffset[i]) * m_workspaceScale[i];
    }
}

void HapticsSim::step()
{
    cMeta3dofPointer *cursor = m_cursor->object();
    cursor->updatePose();

    cVector3d pos = cursor->m_deviceGlobalPos;
    updateWorkspace(pos);

    pos.copyto(cursor->m_deviceGlobalPos);
    pos.copyto(m_cursor->m_position);
    cursor->m_deviceGlobalVel.copyto(m_cursor->m_velocity);

    if (m_pGrabbedObject) {
        cursor->m_lastComputedGlobalForce.set(0,0,0);
        m_cursor->addCursorGrabbedForce(m_pGrabbedObject);
    } else {
        cursor->computeForces();
        m_cursor->addCursorMassForce();
    }

    m_cursor->addCursorExtraForce();

    cursor->applyForces();

    m_counter++;

    int update_sim = Simulation::ST_VISUAL;
    if (m_pGrabbedObject)
        update_sim |= Simulation::ST_PHYSICS;

    if (update_sim)
    {
        /* If in contact with an object, display the cursor at the
         * proxy location instead of the device location, so that it
         * does not show it penetrating the object. */
        cProxyPointForceAlgo *algo =
            (cProxyPointForceAlgo*) cursor->m_pointForceAlgos[0];
        if (algo->getContactObject())
            pos = algo->getProxyGlobalPosition();

        sendtotype(update_sim, true,
                   "/world/cursor/position","fff",
                   pos.x, pos.y, pos.z);
    }

    findContactObject();

    if (m_pContactObject) {
        sendtotype(Simulation::ST_PHYSICS, true,
                   (m_pContactObject->path()+"/push").c_str(), "ffffff",
                   -m_lastForce.x,
                   -m_lastForce.y,
                   -m_lastForce.z,
                   m_lastContactPoint.x,
                   m_lastContactPoint.y,
                   m_lastContactPoint.z);

        bool co1 = m_pContactObject->collidedWith(m_cursor, m_counter);
        bool co2 = m_cursor->collidedWith(m_pContactObject, m_counter);
        if ( (co1 || co2) && m_collide.m_value ) {
            lo_send(address_send, "/world/collide", "ssf",
                    m_pContactObject->c_name(), m_cursor->c_name(),
                    (double)(m_pContactObject->m_velocity
                             - m_cursor->m_velocity).length());
        }
    }
}

void HapticsSim::findContactObject()
{
    m_pContactObject = NULL;
    cGenericObject *obj = NULL;

    cMeta3dofPointer *cursor = m_cursor->object();
    for (unsigned int i=0; i<cursor->m_pointForceAlgos.size(); i++)
    {
        cProxyPointForceAlgo* pointforce_proxy =
            dynamic_cast<cProxyPointForceAlgo*>(cursor->m_pointForceAlgos[i]);
        if ((pointforce_proxy != NULL)
            && (pointforce_proxy->getContactObject() != NULL))
        {
            m_lastContactPoint = pointforce_proxy->getContactPoint();
            m_lastForce = cursor->m_lastComputedGlobalForce;
            obj = pointforce_proxy->getContactObject();
            break;
        }

        cODEPotentialProxy* potential_proxy =
            dynamic_cast<cODEPotentialProxy*>(cursor->m_pointForceAlgos[i]);
        if ((potential_proxy != NULL)
            && (potential_proxy->getContactObject() != NULL))
        {
            m_lastContactPoint = potential_proxy->getContactPoint();
            m_lastForce = cursor->m_lastComputedGlobalForce;
            obj = potential_proxy->getContactObject();
            break;
        }
    }

    // User data is set in the Osc*CHAI constructors
    if (obj)
        m_pContactObject = (OscObject*)obj->m_userData;
}

void HapticsSim::set_grabbed(OscObject *pGrabbed)
{
    Simulation::set_grabbed(pGrabbed);

    CHAIObject *ob;

    // return previous object to normal state
    if (m_pGrabbedObject) {
        ob = dynamic_cast<CHAIObject*>(m_pGrabbedObject->special());
        if (ob) ob->chai_object()->setHapticEnabled(true, true);
    }

    m_pGrabbedObject = pGrabbed;

    // remove new object from haptic contact
    if (m_pGrabbedObject) {
        ob = dynamic_cast<CHAIObject*>(m_pGrabbedObject->special());
        if (ob) ob->chai_object()->setHapticEnabled(false, true);
    }

    // set cursor visibility
    sendtotype(Simulation::ST_VISUAL, 0,
               "/world/cursor/visible", "i",
               ob ? 0 : 1);
}

/****** CHAIObject ******/

CHAIObject::CHAIObject(OscObject *obj, cGenericObject *chai_obj, cWorld *world)
{
    m_object = obj;
    m_chai_object = chai_obj;

    if (!obj || !chai_obj)
        return;

    obj->m_position.setSetCallback(CHAIObject::on_set_position, this);
    obj->m_rotation.setSetCallback(CHAIObject::on_set_rotation, this);
    obj->m_visible.setSetCallback(CHAIObject::on_set_visible, this);
}

CHAIObject::~CHAIObject()
{
}

/****** OscSphereCHAI ******/

OscSphereCHAI::OscSphereCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent)
{
    m_pSphere = new cShapeSphere(m_radius.m_value);
    world->addChild(m_pSphere);
    m_pSphere->computeGlobalPositions();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pSphere->setUserData(this, 1);

    m_pSpecial = new CHAIObject(this, m_pSphere, world);
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

void OscSphereCHAI::on_grab()
{
    simulation()->set_grabbed(this);
}

/****** OscPrismCHAI ******/

OscPrismCHAI::OscPrismCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscPrism(NULL, name, parent)
{
    m_pPrism = new cMesh(world);
    createPrism();
    world->addChild(m_pPrism);
    m_pPrism->computeGlobalPositions();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pPrism->setUserData(this, 1);

    m_pSpecial = new CHAIObject(this, m_pPrism, world);
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
    }
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

void OscPrismCHAI::on_grab()
{
    simulation()->set_grabbed(this);
}

/****** OscMeshCHAI ******/

OscMeshCHAI::OscMeshCHAI(cWorld *world, const char *name, const char *filename,
                         OscBase *parent)
    : OscMesh(NULL, name, filename, parent)
{
    m_pMesh = new cMesh(world);

    if (!m_pMesh->loadFromFile(filename)) {
        printf("[%s] Unable to load %s for object %s.\n",
               simulation()->type_str(), filename, name);
        delete m_pMesh;
        m_pMesh = NULL;
        return;
    }

    printf("[%s] Loaded %s for object %s.\n",
           simulation()->type_str(), filename, name);

    // center the mesh
    m_pMesh->computeBoundaryBox();
    cVector3d vmin = m_pMesh->getBoundaryMin();
    cVector3d vmax = m_pMesh->getBoundaryMax();
    m_pMesh->translate((vmax-vmin*3)/2);

    // size it to 0.1 without changing proportions
    float size = (vmax-vmin).length();
    m_size.set(0.1/size, 0.1/size, 0.1/size);
    on_size();

    /* setup collision detector */
    m_pMesh->createAABBCollisionDetector(true, true);

    world->addChild(m_pMesh);
    m_pMesh->computeGlobalPositions();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pMesh->setUserData(this, 1);

    m_pSpecial = new CHAIObject(this, m_pMesh, world);
}

OscMeshCHAI::~OscMeshCHAI()
{
    if (m_pMesh)
        m_pMesh->getParent()->deleteChild(m_pMesh);
}

void OscMeshCHAI::on_size()
{
    m_pMesh->computeBoundaryBox(true);
    cVector3d vmin(m_pMesh->getBoundaryMin());
    cVector3d vmax(m_pMesh->getBoundaryMax());
    cVector3d scale(vmax - vmin);
    scale.x = m_size.x / scale.x;
    scale.y = m_size.y / scale.y;
    scale.z = m_size.z / scale.z;
    m_pMesh->scale(scale);
}

/****** OscCursorCHAI ******/

OscCursorCHAI::OscCursorCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent)
{
    // create the cursor object
    m_pCursor = new cMeta3dofPointer(world);
    world->addChild(m_pCursor);

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pCursor->setUserData(this, 1);

    // replace the potential proxy algorithm with our own
    cGenericPointForceAlgo *old_proxy, *new_proxy;
    old_proxy = m_pCursor->m_pointForceAlgos[1];
    new_proxy = new cODEPotentialProxy(
        dynamic_cast<cPotentialFieldForceAlgo*>(old_proxy));
    m_pCursor->m_pointForceAlgos[1] = new_proxy;
    delete old_proxy;

    if (m_pCursor->initialize()) {
        m_bInitialized = false;
        printf("Could not initialize haptics.\n");
    } else {
        m_bInitialized = true;
        m_pCursor->start();
    }

    // rotate the cursor to match visual rotation
    m_pCursor->rotate(cVector3d(0,0,1),-90.0*M_PI/180.0);

    // make it a cursor tuned for a dynamic environment
    ((cProxyPointForceAlgo*)m_pCursor->m_pointForceAlgos[0])
        ->enableDynamicProxy(true);

    // this is necessary for the above rotation to take effect
    m_pCursor->computeGlobalPositions();

    // set up mass as zero to begin (transparent proxy)
    m_mass.set(0);

    // no extra force to begin with
    m_nExtraForceSteps = 0;

    m_pSpecial = new CHAIObject(this, m_pCursor, world);
}

OscCursorCHAI::~OscCursorCHAI()
{
    if (m_pCursor)
        m_pCursor->getParent()->deleteChild(m_pCursor);
}

void OscCursorCHAI::on_force()
{
    /* apply the given force for no more than a few timesteps this
     * setting should allow one or two physics timesteps to occur
     * before "giving up", so to speak, allowing plenty of time for a
     * slow servo-loop to run over the network, but still dropping the
     * motors to zero if nothing happens for a while.*/

    /* TODO: Make this timeout a configurable setting. */

    m_extraForce = m_force;
    m_nExtraForceSteps = physics_timestep_ms*2/haptics_timestep_ms;
}

void OscCursorCHAI::on_radius()
{
    printf("OscCursorCHAI::on_radius(). radius = %f\n", m_radius.m_value);

    if (!m_pCursor)
        return;

    m_pCursor->setRadius(m_radius.m_value);
}

/*! Compute a force based on a mass attached to the cursor
 *  and add this force to the global cursor force. */
void OscCursorCHAI::addCursorMassForce()
{
    double timestep = simulation()->timestep();

    // if no mass, just update the mass position
    if (m_mass.m_value <= 0) {
        m_massVel = (m_pCursor->m_deviceGlobalPos - m_massPos) / timestep;
        m_massPos = m_pCursor->m_deviceGlobalPos;
        return;
    }

    double k=10;                         // stiffness of mass-spring
    double b=0.001;//2*sqrt(k*m_mass.m_value);  // critical damping

    cVector3d posdiff(m_pCursor->m_deviceGlobalPos - m_massPos);
    cVector3d springVel((posdiff - m_lastPosDiff)/timestep);
    m_lastPosDiff = posdiff;
    cVector3d veldiff(m_pCursor->m_deviceGlobalVel - m_massVel);
    cVector3d force(-k*posdiff - b*springVel);

    m_massPos += m_massVel*timestep;
    m_massVel -= force/m_mass.m_value*timestep;

    m_pCursor->m_lastComputedGlobalForce += force*10;
}

/*! Compute a force attracting the cursor toward the grabbed
 *  object. */
void OscCursorCHAI::addCursorGrabbedForce(OscObject *pGrabbed)
{
    cVector3d f(m_position - pGrabbed->m_position);
    f.mul(-10);
    f.add(m_velocity * (-0.001));
    m_pCursor->m_lastComputedGlobalForce += f;
}

/*! Add any extra force provided externally by the user. */
void OscCursorCHAI::addCursorExtraForce()
{
    if (m_nExtraForceSteps > 0) {
        m_pCursor->m_lastComputedGlobalForce += m_extraForce;
        m_nExtraForceSteps--;
    }
}
