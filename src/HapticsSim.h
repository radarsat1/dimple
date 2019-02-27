// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#ifndef _HAPTICS_SIM_H_
#define _HAPTICS_SIM_H_

#include "Simulation.h"
#include "OscObject.h"

#include <world/CWorld.h>
#include <display/CCamera.h>
#include <lighting/CPositionalLight.h>
#include <world/CShapeSphere.h>
#include <world/CShapeBox.h>
#include <world/CMultiMesh.h>
#include <tools/CToolCursor.h>
#include <devices/CGenericHapticDevice.h>

class OscCursorCHAI;
class OscHapticsVirtdevCHAI;

class HapticsSim : public Simulation
{
  public:
    HapticsSim(const char *port);
    virtual ~HapticsSim();

    cWorld *world() { return m_chaiWorld; }

    OscObject *contact_object() { return m_pContactObject; }

    //! Set the grabbed object or ungrab by setting to NULL.
    virtual void set_grabbed(OscObject *pGrabbed);

    virtual void on_reset_workspace() { m_resetWorkspace = true; }

    bool m_forceEnabled;

    const cHapticDeviceInfo& getSpecs();

  protected:
    virtual void initialize();
    virtual void step();

    void findContactObject();
    void updateWorkspace(cVector3d &pos, cVector3d &vel);

    OscObject *m_pContactObject;
    cVector3d m_lastContactPoint;
    cVector3d m_lastForce;

    cVector3d m_workspace[2];
    cVector3d m_workspaceScale;
    cVector3d m_workspaceOffset;
    bool m_resetWorkspace;
    bool m_learnWorkspace;

    //! A step counter
    int m_counter;

    cWorld* m_chaiWorld;            //! the world in which we will create our environment
    OscCursorCHAI* m_cursor;    //! An OscObject representing the 3D cursor.
    OscHapticsVirtdevCHAI* m_pVirtdev;

    friend OscHapticsVirtdevCHAI;
};

class HapticsPrismFactory : public PrismFactory
{
public:
    HapticsPrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~HapticsPrismFactory() {}

    virtual HapticsSim* simulation() { return static_cast<HapticsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class HapticsSphereFactory : public SphereFactory
{
public:
    HapticsSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~HapticsSphereFactory() {}

    virtual HapticsSim* simulation() { return static_cast<HapticsSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class HapticsMeshFactory : public MeshFactory
{
public:
    HapticsMeshFactory(Simulation *parent) : MeshFactory(parent) {}
    virtual ~HapticsMeshFactory() {}

    virtual HapticsSim* simulation() { return static_cast<HapticsSim*>(m_parent); }

protected:
    bool create(const char *name, const char *filename,
                float x, float y, float z);
};

class CHAIObject : public OscObjectSpecial
{
public:
    CHAIObject(OscObject *obj, cGenericObject *chai_obj, cWorld *world);
    virtual ~CHAIObject();

    virtual OscObject *obj() { return m_object; }
    virtual cGenericObject *chai_object() { return m_chai_object; }

protected:
    OscObject *m_object;
    cGenericObject *m_chai_object;

    static void on_set_position(void* me, OscVector3 &p)
        { ((CHAIObject*)me)->chai_object()->setLocalPos(p); }
    static void on_set_rotation(void* me, OscMatrix3 &r)
        { ((CHAIObject*)me)->chai_object()->setLocalRot(r); }
    static void on_set_visible(void* me, OscBoolean &v)
        { ((CHAIObject*)me)->chai_object()->setShowEnabled(v.m_value, true); }
    static void on_set_stiffness(void* me, OscScalar &s);

    static void on_set_color(void* me, OscVector3 &c)
        { ((CHAIObject*)me)->chai_object()->
                m_material->m_diffuse.set(c.x(), c.y(), c.z()); }
    static void on_set_friction_static(void* me, OscScalar &mus)
        { ((CHAIObject*)me)->chai_object()->
                m_material->setStaticFriction(mus.m_value); }
    static void on_set_friction_dynamic(void* me, OscScalar &mud)
        { ((CHAIObject*)me)->chai_object()->
                m_material->setDynamicFriction(mud.m_value); }
};

class OscSphereCHAI : public OscSphere
{
public:
    OscSphereCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscSphereCHAI();

    cShapeSphere *object() { return m_pSphere; }

protected:
    virtual void on_radius();

    virtual void on_color()
        { object()->m_material->m_diffuse.set(m_color.x(), m_color.y(), m_color.z()); }
    virtual void on_friction_static()
        { object()->m_material->setStaticFriction(m_friction_static.m_value); }
    virtual void on_friction_dynamic()
        { object()->m_material->setDynamicFriction(m_friction_dynamic.m_value); }
    virtual void on_grab();

    cShapeSphere *m_pSphere;
};

class OscPrismCHAI : public OscPrism
{
public:
    OscPrismCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscPrismCHAI();

    virtual cMesh *object() { return m_pPrism; }

protected:
    virtual void on_size();

    virtual void on_color()
        { object()->m_material->m_diffuse.set(m_color.x(), m_color.y(), m_color.z()); }
    virtual void on_friction_static()
        { object()->m_material->setStaticFriction(m_friction_static.m_value); }
    virtual void on_friction_dynamic()
        { object()->m_material->setDynamicFriction(m_friction_dynamic.m_value); }
    virtual void on_grab();

    //! Create a cMesh with a prism structure.
    void createPrism(bool openbox=false);

    cMesh *m_pPrism;
};

class OscMeshCHAI : public OscMesh
{
public:
    OscMeshCHAI(cWorld *world, const char *name, const char *filename,
                OscBase *parent=NULL);
    virtual ~OscMeshCHAI();

    virtual cMultiMesh *object() { return m_pMesh; }

protected:
    virtual void on_color()
        { object()->m_material->m_diffuse.set(m_color.x(), m_color.y(), m_color.z()); }
    virtual void on_friction_static()
        { object()->m_material->setStaticFriction(m_friction_static.m_value); }
    virtual void on_friction_dynamic()
        { object()->m_material->setDynamicFriction(m_friction_dynamic.m_value); }
    virtual void on_size();

    cMultiMesh *m_pMesh;
};

class OscCursorCHAI : public OscSphere
{
public:
    OscCursorCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscCursorCHAI();

    virtual cToolCursor *object() { return m_pCursor; }

    bool is_initialized() { return m_bInitialized; }
    void initializeWithDevice(cWorld *world, cGenericHapticDevicePtr device);

    int start() { return m_pCursor->start(); }
    int stop()  { return m_pCursor->stop(); }

    void addCursorMassForce();
    void addCursorGrabbedForce(OscObject *pGrabbed);
    void addCursorExtraForce();

    const char *device_str();
    const cHapticDeviceInfo& getSpecs() { return m_specs; }

protected:
    virtual void on_force();
    virtual void on_radius();
    virtual void on_color()
      { /*TODO object()->m_colorProxy.set(m_color.x(), m_color.y(), m_color.z());*/ }

    cToolCursor *m_pCursor;
    cVector3d m_massPos;
    cVector3d m_massVel;
    cVector3d m_lastPosDiff;
    int m_nExtraForceSteps;

    cVector3d m_extraForce;
    bool m_bInitialized;
    cHapticDeviceInfo m_specs;  //! Copy of device specifications
};

class cVirtualDevice : public chai3d::cGenericHapticDevice
{
public:
    cVirtualDevice() {
        memset(&m_specifications, 0, sizeof(m_specifications));
        m_specifications.m_modelName = "virtual";
        m_specifications.m_workspaceRadius = 0.4;
        m_specifications.m_maxLinearStiffness = 1000.0;
    }
    void setPosition(const cVector3d& pos) {
        m_pos = pos;
    }
    virtual bool getPosition(cVector3d& pos) {
        estimateLinearVelocity(pos=m_pos);
        return C_SUCCESS;
    }
    virtual bool open()  { m_deviceReady = true; return C_SUCCESS; }
    virtual bool close() { return C_SUCCESS; }
protected:
    cVector3d m_pos;
};
typedef std::shared_ptr<cVirtualDevice> cVirtualDevicePtr;

class OscHapticsVirtdevCHAI : public OscSphereCHAI
{
public:
    OscHapticsVirtdevCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscHapticsVirtdevCHAI();

    cVirtualDevicePtr object() { return m_pVirtdev; }

    static void on_set_position(void* me, OscVector3 &p);

protected:
    cVirtualDevicePtr m_pVirtdev;
};

#endif // _HAPTICS_SIM_H_
