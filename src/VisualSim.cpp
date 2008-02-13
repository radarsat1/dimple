// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include "dimple.h"
#include "VisualSim.h"
#include "HapticsSim.h"

#include <GL/glut.h>
#ifdef USE_FREEGLUT
#include <GL/freeglut_ext.h>
#endif

bool VisualPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("VisualPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);
    return true;
}

bool VisualSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);
    if (obj)
        return simulation()->add_object(*obj);

    return false;
}

/****** VisualSim ******/

VisualSim *VisualSim::m_pGlobalContext = 0;

VisualSim::VisualSim(const char *port)
    : Simulation(port)
{
    m_pPrismFactory = new VisualPrismFactory(this);
    m_pSphereFactory = new VisualSphereFactory(this);

    m_fTimestep = VISUAL_TIMESTEP_MS/1000.0;
    printf("CHAI/GLUT timestep: %f\n", m_fTimestep);

    // create the world object
    m_chaiWorld = new cWorld();
    m_chaiWorld->setBackgroundColor(0.0f,0.0f,0.0f);
    
    // create a camera
    m_chaiCamera = new cCamera(m_chaiWorld);
    m_chaiWorld->addChild(m_chaiCamera);

    // position a camera
    m_chaiCamera->set( cVector3d (1.0, 0.0, 0.0),
                       cVector3d (0.0, 0.0, 0.0),
                       cVector3d (0.0, 0.0, 1.0));

    // set the near and far clipping planes of the m_chaiCamera
    m_chaiCamera->setClippingPlanes(0.01, 10.0);

    // Create a light source and attach it to the camera
    m_chaiLight = new cLight(m_chaiWorld);
    m_chaiLight->setEnabled(true);
    m_chaiLight->setPos(cVector3d(2,0.5,1));
    m_chaiLight->setDir(cVector3d(-2,0.5,1));
    m_chaiCamera->addChild(m_chaiLight);
}

VisualSim::~VisualSim()
{
}

void VisualSim::initGlutWindow()
{
    // Default size
    m_nWidth = 512;
    m_nHeight = 512;

    // initialize global context pointer for GLUT callbacks
    // which don't have a data argument
    VisualSim::m_pGlobalContext = this;

    // initialize the GLUT windows
    glutInitWindowSize(m_nWidth, m_nHeight);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow("Dimple");
    glutDisplayFunc(draw);
    /*
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(rezizeWindow);
    */

    // create a mouse menu
    /*
    glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("Window Display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    */

    glutTimerFunc(VISUAL_TIMESTEP_MS, updateDisplay, (int)this);
}

void VisualSim::updateDisplay(int data)
{
    VisualSim *me = (VisualSim*)data;
    VisualSim::m_pGlobalContext = me;

    while (lo_server_recv_noblock(me->m_server, 0)) {}
    if (me->m_bDone) {}  // TODO

    glutPostRedisplay();

    // update again in a few ms
    glutTimerFunc(VISUAL_TIMESTEP_MS, updateDisplay, (int)me);
}

void VisualSim::step()
{
    // Start GLUT
	int argc=0;
    glutInit(&argc, NULL);
    initGlutWindow();
    glutMainLoop();

    // Don't return call step() again, since glutMainLoop() does not
    // exit, so if it has exited it means we are done.  The simulation
    // step is instead executed in updateDisplay(), which is called at
    // regular intervals.  Incoming OSC messages are also parsed
    // there.
    m_bDone = true;
}

void VisualSim::draw()
{
    VisualSim* me = VisualSim::m_pGlobalContext;

    // set the background color of the world
    cColorf color = me->m_chaiCamera->getParentWorld()->getBackgroundColor();
    glClearColor(color.getR(), color.getG(), color.getB(), color.getA());

    // clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // sync poses if haptics thread isn't doing it
    /* TODO
    if (!hapticsEnabled)
        syncPoses();
    */

    // render world
    me->m_chaiCamera->renderView(me->m_nWidth, me->m_nHeight);

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}
