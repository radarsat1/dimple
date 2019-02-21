// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include "dimple.h"
#include "HapticsSim.h"
#include "devices/CGenericHapticDevice.h"
#include "devices/CHapticDeviceHandler.h"
#include "tools/CToolCursor.h"
#include <memory>

// TODO: right way to pass this around
cHapticDeviceInfo g_hapticDeviceInfo;
float g_maxStiffnessRatio = 0.1;

bool HapticsPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("HapticsPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrismCHAI *obj = new OscPrismCHAI(simulation()->world(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

    return true;
}

bool HapticsSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

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

    obj->m_position.setValue(x, y, z);

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
    // Stop the simulation before deleting objects, otherwise thread
    // is still running and may dereference them.
    stop();

    if (m_chaiWorld) delete m_chaiWorld;
}

void HapticsSim::initialize()
{
    // create the world object
    m_chaiWorld = new cWorld();
    m_chaiWorld->setBackgroundColor(0.0f,0.0f,0.0f);

    // create an OscObject to point to the cursor
    m_cursor = new OscCursorCHAI(m_chaiWorld, "cursor", this);

    // special case:
    // we know that the libnifalcon driver times itself, so don't
    // allow the Simulation to time itself before each step().
#ifdef DEVICE_LIBNIFALCON
    if (m_cursor->object()->getPhysicalDevice() == DEVICE_LIBNIFALCON)
        m_bSelfTimed = false;
#endif

    // ensure workspace is recalibrated
    m_resetWorkspace = true;
    m_learnWorkspace = true;

    // quit the haptics simulation if the cursor couldn't be initialized.
    if (!m_cursor->is_initialized())
    {
        // turn off workspace scaling
        m_learnWorkspace = false;

        // create the corresponding visual cursor
        simulation()->sendtotype(Simulation::ST_VISUAL, false,
                                 "/world/sphere/create","sfff",
                                 "cursor", 0.0, 0.0, 0.0);
        simulation()->sendtotype(Simulation::ST_VISUAL, false,
                                 "/world/cursor/color","fff",
                                 1.0, 1.0, 0.0);

        // Create a virtual device named "device"
        simulation()->sendtotype(Simulation::ST_VISUAL, false,
                                 "/world/virtdev/create","sfff",
                                 "device", 0.0, 0.0, 0.0);
        simulation()->sendtotype(Simulation::ST_VISUAL, false,
                                 "/world/virtdev/color","fff",
                                 1.0, 0.0, 0.0);

        // Create a local object to accept messages from the virtual device
        m_pVirtdev = new OscHapticsVirtdevCHAI(m_chaiWorld, "device", this);

        // Hook it up to the cursor
        m_cursor->initializeWithDevice(m_chaiWorld, m_pVirtdev->object());

        if (!m_cursor->is_initialized())
            m_bDone = true;
    }

    // initialize step count
    m_counter = 0;

    m_pGrabbedObject = NULL;

    Simulation::initialize();
}

void HapticsSim::updateWorkspace(cVector3d &pos, cVector3d &vel)
{
    int i;
    if (m_resetWorkspace) {
        m_workspace[0] = pos;
        m_workspace[1] = pos;
        m_resetWorkspace = false;
    }

    for (i=0; i<3; i++) {
        // Update workspace boundaries
        if (m_learnWorkspace)
        {
            if (pos(i) < m_workspace[0](i))
                m_workspace[0](i) = pos(i);
            if (pos(i) > m_workspace[1](i))
                m_workspace[1](i) = pos(i);
        }

        float dif = (m_workspace[1](i) - m_workspace[0](i));
        if (dif != 0.0)
            m_workspaceScale(i) = 2.0/(m_workspace[1](i) - m_workspace[0](i));
        else
            m_workspaceScale(i) = 1;
        m_workspaceOffset(i) = -(m_workspace[1](i) + m_workspace[0](i)) / 2.0;

        // Normalize position to [-1, 1] within workspace.
        // Further scale by user-specified workspace scaling. (default=1)
        pos(i) = (pos(i) + m_workspaceOffset(i)) * m_workspaceScale(i) * m_scale(i);
        vel(i) = vel(i) * m_workspaceScale(i) * m_scale(i);
    }
}

void HapticsSim::step()
{
    m_chaiWorld->computeGlobalPositions(true);

    cToolCursor *cursor = m_cursor->object();
    cursor->updateFromDevice();

    cVector3d pos = cursor->getDeviceGlobalPos();
    cVector3d vel = cursor->getDeviceGlobalLinVel();

    updateWorkspace(pos, vel);
    cursor->setDeviceGlobalPos(pos);
    cursor->setDeviceGlobalLinVel(vel);

    // Set values without feeding back changes to CHAI
    m_cursor->m_position.setValue(pos, false);
    m_cursor->m_velocity.setValue(vel, false);

    if (m_pGrabbedObject) {
        cursor->setDeviceGlobalForce(0,0,0);
        m_cursor->addCursorGrabbedForce(m_pGrabbedObject);
    } else {
        cursor->computeInteractionForces();

        // Compensate for workspace scaling
        cVector3d force = cursor->getDeviceGlobalForce();
        force = force * (m_workspaceScale * m_scale);
        cursor->setDeviceGlobalForce(force);

        m_cursor->addCursorMassForce();
    }

    m_cursor->addCursorExtraForce();

    // Set force value without feeding back changes to CHAI
    m_cursor->m_force.setValue(cursor->getDeviceGlobalForce(), false);

    cursor->applyToDevice();

    m_counter++;

    int update_sim = Simulation::ST_VISUAL;
    if (m_pGrabbedObject)
        update_sim |= Simulation::ST_PHYSICS;

    if (update_sim)
    {
        /* If in contact with an object, display the cursor at the
         * proxy location instead of the device location, so that it
         * does not show it penetrating the object. */
        pos = cursor->m_image->getGlobalPos();

        /* It appears that non-penetrating tool position is not
         * correctly displayed for potential force algo used with
         * shape primitives, since there is no "proxy".  Instead, we
         * wills how the interaction point. Note that it also does not
         * seem to take into account the haptic point radius. */
        if (cursor->m_hapticPoint->getNumInteractionEvents() > 0)
        {
            const auto& inter = *cursor->m_hapticPoint->getInteractionEvent(0);
            pos = inter.m_object->getGlobalPos() + cMul(inter.m_object->getGlobalRot(),
                                                        inter.m_localSurfacePos);
        }

        sendtotype(update_sim, true,
                   "/world/cursor/position","fff",
                   pos.x(), pos.y(), pos.z());
    }

    findContactObject();

    if (m_pContactObject) {
        sendtotype(Simulation::ST_PHYSICS, true,
                   (m_pContactObject->path()+"/push").c_str(), "ffffff",
                   -m_lastForce.x(),
                   -m_lastForce.y(),
                   -m_lastForce.z(),
                   m_lastContactPoint.x(),
                   m_lastContactPoint.y(),
                   m_lastContactPoint.z());

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

    cToolCursor *cursor = m_cursor->object();
    if (cursor->m_hapticPoint->getNumCollisionEvents() > 0)
    {
        cCollisionEvent* collisionEvent =
            cursor->m_hapticPoint->getCollisionEvent(0);
        m_lastContactPoint = cursor->m_hapticPoint->getGlobalPosProxy();
        m_lastForce = cursor->getDeviceGlobalForce();
        obj = collisionEvent->m_object;
    }

    if (!obj && cursor->m_hapticPoint->getNumInteractionEvents() > 0)
    {
        cInteractionEvent* interactionEvent =
            cursor->m_hapticPoint->getInteractionEvent(0);
        m_lastContactPoint = cursor->m_hapticPoint->getGlobalPosProxy();
        m_lastForce = cursor->getDeviceGlobalForce();
        obj = interactionEvent->m_object;
    }

    // User data is set in the Osc*CHAI constructors
    if (obj)
        m_pContactObject = (OscObject*)obj->m_userData;
}

void HapticsSim::set_grabbed(OscObject *pGrabbed)
{
    CHAIObject *ob = NULL;

    // return previous object to normal state
    if (m_pGrabbedObject) {
        ob = dynamic_cast<CHAIObject*>(m_pGrabbedObject->special());
        if (ob) ob->chai_object()->setHapticEnabled(true, true);
    }

    Simulation::set_grabbed(pGrabbed);

    // remove new object from haptic contact
    ob = NULL;
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
    m_pSphere->m_userData = this;

    // TODO: used to be that setUserData would have an "affect children" flag:
    // m_pSphere->setUserData(this, 1);
    // How to replace in Chai3d 3.2?

    m_pSphere->createEffectSurface();
    m_pSphere->m_material->setStiffness(g_maxStiffnessRatio
                                        * g_hapticDeviceInfo.m_maxLinearStiffness);

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
    m_pPrism = new cMesh();
    createPrism();
    m_pPrism->computeBoundaryBox();
    m_pPrism->m_material->setBlueLight();

    world->addChild(m_pPrism);
    m_pPrism->computeGlobalPositions();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pPrism->m_userData = this;

    m_pPrism->createBruteForceCollisionDetector();
    m_pPrism->createEffectSurface();
    m_pPrism->m_material->setStiffness(g_maxStiffnessRatio
                                       * g_hapticDeviceInfo.m_maxLinearStiffness);

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
    m_pPrism->newVertex( m_size.x()/2,  m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex( m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex( m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex( m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex( m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex( m_size.x()/2, -m_size.y()/2,  m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        m_pPrism->m_vertices->setTexCoord(n,
            (m_pPrism->m_vertices->getLocalPos(n).y() + m_size.x()/2) / (2.0 * m_size.z()/2),
            (m_pPrism->m_vertices->getLocalPos(n).z() + m_size.x()/2) / (2.0 * m_size.y()/2)
            );
        m_pPrism->m_vertices->setNormal(n,1,0,0);
    }

    start_index += 6;

    // -x face
    m_pPrism->newVertex(-m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2,  m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(-m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2,  m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        m_pPrism->m_vertices->setTexCoord(n,
            (m_pPrism->m_vertices->getLocalPos(n).y() + m_size.x()/2) / (2.0 * m_size.z()/2),
            (m_pPrism->m_vertices->getLocalPos(n).z() + m_size.x()/2) / (2.0 * m_size.y()/2)
            );
        m_pPrism->m_vertices->setNormal(n,-1,0,0);
    }

    start_index += 6;

    // +y face
    m_pPrism->newVertex(m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(m_size.x()/2,  m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, m_size.y()/2,  m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        m_pPrism->m_vertices->setTexCoord(n,
            (m_pPrism->m_vertices->getLocalPos(n).x() + m_size.y()/2) / (2.0 * m_size.z()/2),
            (m_pPrism->m_vertices->getLocalPos(n).z() + m_size.x()/2) / (2.0 * m_size.y()/2)
            );
        m_pPrism->m_vertices->setNormal(n,0,1,0);
    }

    start_index += 6;

    // -y face
    m_pPrism->newVertex(m_size.x()/2,  -m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(m_size.x()/2,  -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(m_size.x()/2,  -m_size.y()/2,  m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2,  m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        m_pPrism->m_vertices->setTexCoord(n,
            (m_pPrism->m_vertices->getLocalPos(n).x() + m_size.y()/2) / (2.0 * m_size.z()/2),
            (m_pPrism->m_vertices->getLocalPos(n).z() + m_size.x()/2) / (2.0 * m_size.y()/2)
            );
        m_pPrism->m_vertices->setNormal(n,0,-1,0);
    }

    start_index += 6;

    // -z face
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(m_size.x()/2,   m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(m_size.x()/2,  -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    m_pPrism->newVertex( m_size.x()/2,  m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, -m_size.z()/2);
    m_pPrism->newVertex(-m_size.x()/2,  m_size.y()/2, -m_size.z()/2);
    m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
    cur_index+=3;

    for(n=start_index; n<cur_index; n++) {
        m_pPrism->m_vertices->setTexCoord(n,
            (m_pPrism->m_vertices->getLocalPos(n).x() + m_size.y()/2) / (2.0 * m_size.z()/2),
            (m_pPrism->m_vertices->getLocalPos(n).y() + m_size.x()/2) / (2.0 * m_size.y()/2)
            );
        m_pPrism->m_vertices->setNormal(n,0,0,-1);
    }

    start_index += 6;

    if (!openbox) {

        // +z face
        m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, m_size.z()/2);
        m_pPrism->newVertex(m_size.x()/2,  -m_size.y()/2, m_size.z()/2);
        m_pPrism->newVertex(m_size.x()/2,  m_size.y()/2,  m_size.z()/2);
        m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
        cur_index+=3;

        m_pPrism->newVertex(-m_size.x()/2, -m_size.y()/2, m_size.z()/2);
        m_pPrism->newVertex( m_size.x()/2,  m_size.y()/2, m_size.z()/2);
        m_pPrism->newVertex(-m_size.x()/2,  m_size.y()/2, m_size.z()/2);
        m_pPrism->newTriangle(cur_index,cur_index+1,cur_index+2);
        cur_index+=3;

        for(n=start_index; n<cur_index; n++) {
            m_pPrism->m_vertices->setTexCoord(n,
                (m_pPrism->m_vertices->getLocalPos(n).x() + m_size.y()/2) / (2.0 * m_size.z()/2),
                (m_pPrism->m_vertices->getLocalPos(n).y() + m_size.x()/2) / (2.0 * m_size.y()/2)
                );
            m_pPrism->m_vertices->setNormal(n,0,0,1);
        }

        start_index += 6;
    }
}

void OscPrismCHAI::on_size()
{
    cVector3d curSize = m_pPrism->getBoundaryMax() - m_pPrism->getBoundaryMin();
    cVector3d scale(1.0/curSize.x(), 1.0/curSize.y(), 1.0/curSize.z());
    m_pPrism->scaleXYZ(scale.x()*m_size.x(),
                       scale.y()*m_size.y(),
                       scale.z()*m_size.z());
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
    m_pMesh = new cMultiMesh();

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
    m_size.setValue(0.1/size, 0.1/size, 0.1/size);
    on_size();

    /* setup collision detector */
    m_pMesh->createAABBCollisionDetector(0.01 /* TODO make variable OSC-accessible? */);

    world->addChild(m_pMesh);
    m_pMesh->computeGlobalPositions();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pMesh->m_userData = this;

    // We do not call createEffectSurface() for cMesh since
    // finger-proxy algorithm is engaged.
    m_pMesh->m_material->setStiffness(g_maxStiffnessRatio
                                      * g_hapticDeviceInfo.m_maxLinearStiffness);

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
    m_pMesh->scaleXYZ(m_size.x() / scale.x(),
                      m_size.y() / scale.y(),
                      m_size.z() / scale.z());
}

/****** OscCursorCHAI ******/

OscCursorCHAI::OscCursorCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphere(NULL, name, parent)
{
    // create haptic device handler
    auto handler = std::unique_ptr<cHapticDeviceHandler>(new cHapticDeviceHandler());
    printf("[%s] Haptic devices found: %d\n", simulation()->type_str(),
           handler->getNumDevices());

    // get handle to first available haptic device on the list
    cGenericHapticDevicePtr device;

    m_bInitialized = false;
    m_pCursor = nullptr;
    if (handler->getDevice(device, 0))
        initializeWithDevice(world, device);

    if (!m_bInitialized)
        printf("[%s] Could not initialize.\n", simulation()->type_str());
}

void OscCursorCHAI::initializeWithDevice(cWorld *world, cGenericHapticDevicePtr device)
{
    if (!device->open())
    {
        m_bInitialized = false;
        return;
    }

    m_bInitialized = true;

    device->calibrate();

    // create the cursor object
    if (m_pCursor) {
        world->removeChild(m_pCursor);
        delete m_pCursor;
    }
    m_pCursor = new cToolCursor(world);

    m_pCursor->setHapticDevice(device);
    world->addChild(m_pCursor);

    printf("[%s] Using %s device.\n",
           simulation()->type_str(), device_str());

    // TODO: right way to pass this around
    g_hapticDeviceInfo = device->getSpecifications();

    // User data points to the OscObject, used for identification
    // during object contact.
    m_pCursor->m_userData = this;

    m_pCursor->setWorkspaceRadius(1.0);
    m_pCursor->setRadius(0.01);

    m_pCursor->setWaitForSmallForce(true);

    m_pCursor->start();

    // rotate the cursor to match visual rotation
    m_pCursor->rotateAboutLocalAxisDeg(cVector3d(0,0,1), -90.0);

    // make it a cursor tuned for a dynamic environment
    m_pCursor->enableDynamicObjects(true);

    // this is necessary for the above rotation to take effect
    m_pCursor->computeGlobalPositions();

    // set up mass as zero to begin (transparent proxy)
    m_mass.setValue(0);

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

const char *OscCursorCHAI::device_str()
{
    /* Using m_specifications instead of getSpecifications() to avoid
     * returning a pointer to a temporary. */
    cGenericHapticDevicePtr dev = m_pCursor->getHapticDevice();
    if (dev)
        return dev->m_specifications.m_modelName.c_str();
    return "no";
}

void OscCursorCHAI::on_radius()
{
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
        m_massVel = (m_pCursor->getDeviceGlobalPos() - m_massPos) / timestep;
        m_massPos = m_pCursor->getDeviceGlobalPos();
        return;
    }

    double k=10;                         // stiffness of mass-spring
    double b=0.001;//2*sqrt(k*m_mass.m_value);  // critical damping

    cVector3d posdiff(m_pCursor->getDeviceGlobalPos() - m_massPos);
    cVector3d springVel((posdiff - m_lastPosDiff)/timestep);
    m_lastPosDiff = posdiff;
    cVector3d veldiff(m_pCursor->getDeviceGlobalLinVel() - m_massVel);
    cVector3d force(-k*posdiff - b*springVel);

    m_massPos += m_massVel*timestep;
    m_massVel -= force/m_mass.m_value*timestep;

    m_pCursor->addDeviceGlobalForce(force*10);
}

/*! Compute a force attracting the cursor toward the grabbed
 *  object. */
void OscCursorCHAI::addCursorGrabbedForce(OscObject *pGrabbed)
{
    cVector3d f(m_position - pGrabbed->m_position);
    f.mul(-10);
    f.add(m_velocity * (-0.001));
    m_pCursor->addDeviceGlobalForce(f);
}

/*! Add any extra force provided externally by the user. */
void OscCursorCHAI::addCursorExtraForce()
{
    if (m_nExtraForceSteps > 0) {
        m_pCursor->addDeviceGlobalForce(m_extraForce);
        m_nExtraForceSteps--;
    }
}

OscHapticsVirtdevCHAI::OscHapticsVirtdevCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphereCHAI(world, name, parent)
{
    m_pVirtdev = std::make_shared<cVirtualDevice>();
    m_position.setSetCallback(OscHapticsVirtdevCHAI::on_set_position, this);
}

OscHapticsVirtdevCHAI::~OscHapticsVirtdevCHAI()
{
}

void OscHapticsVirtdevCHAI::on_set_position(void* _me, OscVector3 &p)
{
    OscHapticsVirtdevCHAI *me = static_cast<OscHapticsVirtdevCHAI*>(_me);

    // Rotate virtual device position to match cursor.
    HapticsSim *sim = static_cast<HapticsSim*>(me->simulation());
    cMatrix3d rot(sim->m_cursor->object()->getLocalRot());
    me->m_pVirtdev->setPosition(cMul(cInverse(rot), p));

    // Update visual representation of virtual device
    // TODO: should be able to throttle this but it leads to stuttering visual updates
    me->simulation()->sendtotype(Simulation::ST_VISUAL, false, "/world/device/position", "fff",
                                 me->m_position.x(), me->m_position.y(), me->m_position.z());
}
