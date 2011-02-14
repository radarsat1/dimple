// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include "config.h"
#include "dimple.h"
#include "VisualSim.h"
#include "HapticsSim.h"
#include "config.h"

#include <GL/glut.h>
#ifdef USE_FREEGLUT
#include <GL/freeglut.h>
#endif

bool VisualPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("VisualPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrismCHAI *obj = new OscPrismCHAI(simulation()->world(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool VisualSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.set(x, y, z);

    return true;
}

bool VisualMeshFactory::create(const char *name, const char *filename,
                               float x, float y, float z)
{
    printf("VisualMeshFactory (%s) is creating a mesh "
           "object called '%s' (%s)\n",
           m_parent->c_name(), name, filename);

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

/****** VisualSim ******/

VisualSim *VisualSim::m_pGlobalContext = 0;

VisualSim::VisualSim(const char *port)
    : Simulation(port, ST_VISUAL),
      m_camera(NULL),
      m_bFullScreen(false)
{
    m_pPrismFactory = new VisualPrismFactory(this);
    m_pSphereFactory = new VisualSphereFactory(this);
    m_pMeshFactory = new VisualMeshFactory(this);

    m_fTimestep = visual_timestep_ms/1000.0;
    printf("CHAI/GLUT timestep: %f\n", m_fTimestep);
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
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    /*
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    */

    // create a mouse menu
    /*
    glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("Window Display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    */

    glutTimerFunc(visual_timestep_ms, updateDisplay, 0);
}

void VisualSim::updateDisplay(int data)
{
    VisualSim *me = VisualSim::m_pGlobalContext;

    while (lo_server_recv_noblock(me->m_server, 0)) {}

#ifdef USE_QUEUES
    std::vector<LoQueue*>::iterator qit;
    for (qit=me->m_queueList.begin();
         qit!=me->m_queueList.end(); qit++) {
        while ((*qit)->read_and_dispatch(me->m_server)) {}
    }
#endif

    int step_ms = (int)(me->m_fTimestep*1000);
    me->m_valueTimer.onTimer(step_ms);

    if (me->m_bDone) {}  // TODO

    glutPostRedisplay();

    // update again in a few ms
    glutTimerFunc(visual_timestep_ms, updateDisplay, 0);
}

void VisualSim::initialize()
{
    // create the world object
    m_chaiWorld = new cWorld();
    m_chaiWorld->setBackgroundColor(0.0f,0.0f,0.0f);
    
    // create a camera
    m_camera = new OscCameraCHAI(m_chaiWorld, "camera", this);
    m_chaiWorld->addChild(m_camera->object());

    // Create a light source and attach it to the camera
    m_chaiLight = new cLight(m_chaiWorld);
    m_chaiLight->setEnabled(true);
    m_chaiLight->setPos(cVector3d(2,0.5,1));
    m_chaiLight->setDir(cVector3d(-2,0.5,1));
    m_camera->object()->addChild(m_chaiLight);

    // Create an object to represent the cursor
    OscSphereCHAI *pCursor = new OscSphereCHAI(m_chaiWorld, "cursor", this);
    if (pCursor) {
        m_chaiWorld->addChild(pCursor->object());
        pCursor->m_position.set(0, 0, 0);
        pCursor->m_color.set(1, 1, 0);
    }

    Simulation::initialize();
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
    cColorf color = me->m_chaiWorld->getBackgroundColor();
    glClearColor(color.getR(), color.getG(), color.getB(), color.getA());

    // clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // sync poses if haptics thread isn't doing it
    /* TODO
    if (!hapticsEnabled)
        syncPoses();
    */

    // render world
    me->m_camera->object()->renderView(me->m_nWidth, me->m_nHeight);

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    glutSwapBuffers();
}

void VisualSim::on_clear()
{
    object_iterator it = world_objects.begin();
    while (it != world_objects.end())
    {
        if (it->second->name() == "cursor")
            it++;
        else {
            it->second->on_destroy();
            it = world_objects.begin();
        }
    }
}

void VisualSim::key(unsigned char key, int x, int y)
{
    VisualSim* me = VisualSim::m_pGlobalContext;
    switch (key)
    {
        case (27):
        {
            me->m_bDone = true;
#ifdef USE_FREEGLUT
            glutLeaveMainLoop();
#endif
            break;
        }
        case ('f'):
        {
            if (me->m_bFullScreen) {
                glutReshapeWindow(500, 500);
                glutInitWindowPosition(100, 100);
            }
            else
                glutFullScreen();
            me->m_bFullScreen = !me->m_bFullScreen;
            break;
        }
        case ('w'):
        {
            me->sendtotype(Simulation::ST_HAPTICS, false,
                           "/world/reset_workspace", "");
        }
    }
}

void VisualSim::reshape(int w, int h)
{
    VisualSim* me = VisualSim::m_pGlobalContext;

    // update the size of the viewport
    me->m_nWidth = w;
    me->m_nHeight = h;
    glViewport(0, 0, w, h);
}

OscCameraCHAI::OscCameraCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscCamera(name, parent)
{
    m_pCamera = new cCamera(world);

    // position a camera such that X increases to the right, Y
    // increases into the screen, and Z is up.
    m_position.set(0.0, -1.0, 0.0);
    m_lookat.set(0.0, 0.0, 0.0);
    m_up.set(0.0, 0.0, 1.0);

    m_pCamera->set(m_position, m_lookat, m_up);

    // set the near and far clipping planes of the camera
    m_pCamera->setClippingPlanes(0.01, 10.0);
}

OscCameraCHAI::~OscCameraCHAI()
{
    if (m_pCamera)
        m_pCamera->getParent()->deleteChild(m_pCamera);
}
