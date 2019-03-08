// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#ifndef _VISUAL_SIM_H_
#define _VISUAL_SIM_H_

#include "Simulation.h"
#include "HapticsSim.h"

#include <world/CWorld.h>
#include <display/CCamera.h>
#include <lighting/CSpotLight.h>
#include <widgets/CLabel.h>

class OscCameraCHAI;
class VisualVirtdevFactory;

class VisualSim : public Simulation
{
  public:
    VisualSim(const char *port);
    virtual ~VisualSim();

    cWorld *world() { return m_chaiWorld; }
    OscCameraCHAI *camera() { return m_camera; }
    cSpotLight *light(unsigned int i);

    //! Message to append to log (displayed in window)
    OSCSTRING(VisualSim, log);

  protected:
    virtual void initialize();
    virtual void step();

    void initGlutWindow();
    static void updateDisplay(int data);
    static void draw();
    static void key(unsigned char key, int x, int y);
    static void reshape(int w, int h);
    static void mouseClick(int button, int state, int x, int y);
    static void mouseMotion(int x, int y);

    cVector3d projectOnWindowRay(const cVector3d& vec, int x, int y);

    int m_nWidth, m_nHeight;

    cWorld* m_chaiWorld;            //! the world in which we will create our environment
    cSpotLight *m_chaiLight0;       //! a light source
    cSpotLight *m_chaiLight1;       //! a light source

    OscCameraCHAI *m_camera;        //! an OSC-controllable camera

    /** GLUT callback functions require a pointer to the VisualSim
     ** object, but do not have a user-specified data parameter.  On
     ** the assumption that all callback functions are called
     ** subsquently after the timer callback, this static pointer is
     ** used to point to the one and only VisualSim instance to give
     ** the callback functions context. */
    static VisualSim *m_pGlobalContext;

    OscObject *m_selectedObject;
    cVector3d m_selectionOffset;
    int m_selectionPlane;
    struct CameraProjection;
    CameraProjection *m_cameraProj;
    VisualVirtdevFactory *m_pVirtdevFactory;

    cLabel* m_logLabel;

    bool m_bFullScreen;
};

class VisualPrismFactory : public PrismFactory
{
public:
    VisualPrismFactory(Simulation *parent) : PrismFactory(parent) {}
    virtual ~VisualPrismFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class VisualSphereFactory : public SphereFactory
{
public:
    VisualSphereFactory(Simulation *parent) : SphereFactory(parent) {}
    virtual ~VisualSphereFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    bool create(const char *name, float x, float y, float z);
};

class VisualMeshFactory : public MeshFactory
{
public:
    VisualMeshFactory(Simulation *parent) : MeshFactory(parent) {}
    virtual ~VisualMeshFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    bool create(const char *name, const char *filename,
                float x, float y, float z);
};

class VisualVirtdevFactory : public ShapeFactory
{
public:
    VisualVirtdevFactory(Simulation *parent);
    virtual ~VisualVirtdevFactory() {}

    virtual VisualSim* simulation() { return static_cast<VisualSim*>(m_parent); }

protected:
    // message handlers
    static int create_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data);

    bool create(const char *name, float x, float y, float z);
};

class OscCameraCHAI : public OscCamera
{
public:
    OscCameraCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscCameraCHAI();

    virtual cCamera *object() { return m_pCamera; }

    virtual void on_position() { m_pCamera->set(m_position, m_lookat, m_up); }
    virtual void on_lookat()   { m_pCamera->set(m_position, m_lookat, m_up); }
    virtual void on_up()       { m_pCamera->set(m_position, m_lookat, m_up); }

protected:
    cCamera *m_pCamera;
};

class OscVisualVirtdevCHAI : public OscSphereCHAI
{
public:
    OscVisualVirtdevCHAI(cWorld *world, const char *name, OscBase *parent=NULL);
    virtual ~OscVisualVirtdevCHAI();

    cShapeSphere *object() { return m_pSphere; }

    //! Return an integer indicate which plane obj corresponds to, or 0 if none.
    int getSelectionPlane(cGenericObject *obj);

protected:
    // virtual void on_radius();

    virtual void on_color()
        { object()->m_material->m_diffuse.set(m_color.x(), m_color.y(), m_color.z()); }

    cGenericObject *m_pHandleXY;     //! virtual device handle for XY plane
    cGenericObject *m_pHandleXZ;     //! virtual device handle for XZ plane
};

#endif // _VISUAL_SIM_H_
