
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
*/
//===========================================================================

#define _WINSOCKAPI_
//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <GL/glut.h>
#ifdef USE_FREEGLUT
#include <GL/freeglut_ext.h>
#endif

//---------------------------------------------------------------------------
#include "CCamera.h"
#include "CLight.h"
#include "CWorld.h"
#include "CMesh.h"
#include "CTriangle.h"
#include "CVertex.h"
#include "CMaterial.h"
#include "CMatrix3d.h"
#include "CVector3d.h"
#include "CPrecisionClock.h"
#include "CPrecisionTimer.h"
#include "CMeta3dofPointer.h"
#include "CShapeSphere.h"
//---------------------------------------------------------------------------
#include "osc_chai_glut.h"
#include "CODEMesh.h"
#include "CODEProxy.h"
#include "CODEPrism.h"
#include "CODESphere.h"
#include "CODEPotentialProxy.h"
//---------------------------------------------------------------------------

lo_address address_send = lo_address_new("localhost", "7771");

// the world in which we will create our environment
cWorld* world;

// the camera which is used view the environment in a window
cCamera* camera;

// a light source
cLight *light;

// a 3D cursor which represents the haptic device
cMeta3dofPointer* cursor;

// precision clock to sync dynamic simulation
cPrecisionClock g_clock;

// haptic timer callback
cPrecisionTimer timer;

// world objects & constraints
std::map<std::string,OscObject*> world_objects;
std::map<std::string,OscConstraint*> world_constraints;

// width and height of the current viewport display
int width   = 0;
int height  = 0;

// menu options
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

// OSC handlers
lo_server_thread loserver;

int glutStarted = 0;
int hapticsStarted = 0;
int requestHapticsStart = 0;
int requestHapticsStop = 0;
float globalForceMagnitude = 0;
int quit = 0;
int lock_world = 0;

void poll_requests();

#define ODE_IN_HAPTICS_LOOP

#define MAX_CONTACTS 30
#define FPS 30
#define GLUT_TIMESTEP_MS   (int)((1.0/FPS)*1000.0)
#define HAPTIC_TIMESTEP_MS 1

#ifdef ODE_IN_HAPTICS_LOOP
#define ODE_TIMESTEP_MS    HAPTIC_TIMESTEP_MS
#else
#define ODE_TIMESTEP_MS    GLUT_TIMESTEP_MS
#endif

// ODE objects
dWorldID ode_world;
double ode_step = 0;
dSpaceID ode_space;
dJointGroupID ode_contact_group;

cODEPrimitive *contactObject=NULL;
cVector3d lastForce;
cVector3d lastContactPoint;
double force_scale = 0.1;

bool clearFlag = false;

int mousepos[2];

#ifdef _POSIX
#define Sleep usleep
#endif

//---------------------------------------------------------------------------

void ode_simStep();

//---------------------------------------------------------------------------

void draw(void)
{
    // set the background color of the world
    cColorf color = camera->getParentWorld()->getBackgroundColor();
    glClearColor(color.getR(), color.getG(), color.getB(), color.getA());

    // clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render world
    camera->renderView(width, height);

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));

    // Swap buffers
    glutSwapBuffers();

    poll_requests();
}

//---------------------------------------------------------------------------

void key(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // stop the simulation timer
        timer.stop();

        // stop the tool
        cursor->stop();

        // wait for the simulation timer to close
        Sleep(100);

        // exit application
        exit(0);
    }
}

//---------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
    mousepos[0] = x;
    mousepos[1] = y;
}

//---------------------------------------------------------------------------

void motion(int x, int y)
{
    cVector3d tran;
    tran.x = 0;
    tran.y = ((float)x-mousepos[0])/width;
    tran.z = -((float)y-mousepos[1])/height;

    camera->translate(tran);

    mousepos[0] = x;
    mousepos[1] = y;
}

//---------------------------------------------------------------------------

void rezizeWindow(int w, int h)
{
    // update the size of the viewport
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

//---------------------------------------------------------------------------

void updateDisplay(int val)
{
    // draw scene
    draw();

    // update the GLUT timer for the next rendering call
    glutTimerFunc(FPS, updateDisplay, 0);

#ifndef ODE_IN_HAPTICS_LOOP
	// update ODE
    if (!WORLD_LOCKED())
    	ode_simStep();
#endif
}

//---------------------------------------------------------------------------

void setOther(int value)
{
    switch (value)
    {
        case OPTION_FULLSCREEN:
            glutFullScreen();
            break;

        case OPTION_WINDOWDISPLAY:
            glutReshapeWindow(512, 512);
            glutInitWindowPosition(0, 0);
            break;
    }
    
    glutPostRedisplay();
}

//---------------------------------------------------------------------------

// TODO: remove this
void hapticsLoop(void* a_pUserData)
{
    // read position from haptic device
    cursor->updatePose();

    // compute forces
    cursor->computeForces();

    // send forces to haptic device
    cursor->applyForces();

    // stop the simulation clock
    g_clock.stop();

    // read the time increment in seconds
    double increment = g_clock.getCurrentTime() / 1000000.0;

    // restart the simulation clock
    g_clock.initialize();
    g_clock.start();

    // get position of cursor in global coordinates
    cVector3d cursorPos = cursor->m_deviceGlobalPos;

	globalForceMagnitude = cursor->m_lastComputedGlobalForce.length();
}

//---------------------------------------------------------------------------
// callback function for ODE
void ode_nearCallback (void *data, dGeomID o1, dGeomID o2)
{
    int i;
    // if (o1->body && o2->body) return;

    // exit without doing anything if the two bodies are connected by a joint
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
    if (b1 && b2 && dAreConnectedExcluding (b1,b2,dJointTypeContact)) return;

    dContact contact[MAX_CONTACTS];   // up to MAX_CONTACTS contacts per box-box
    for (i=0; i<MAX_CONTACTS; i++) {
        contact[i].surface.mode = dContactBounce | dContactSoftCFM;
        contact[i].surface.mu = dInfinity;
        contact[i].surface.mu2 = 0;
        contact[i].surface.bounce = 0.1;
        contact[i].surface.bounce_vel = 0.1;
        contact[i].surface.soft_cfm = 0.01;
    }

	if (int numc = dCollide (o1,o2,MAX_CONTACTS,&contact[0].geom,sizeof(dContact)))
	{
		dMatrix3 RI;
		dRSetIdentity (RI);
		const dReal ss[3] = {0.02,0.02,0.02};
		for (i=0; i<numc; i++) {
			dJointID c = dJointCreateContact (ode_world,ode_contact_group,contact+i);
			dJointAttach (c,b1,b2);
		}
	}
}

void ode_hapticsLoop(void* a_pUserData)
{
    bool cursor_ready = true;

    if (clearFlag) {
        LOCK_WORLD();
        objects_iter it;
        for (it=world_objects.begin(); it!=world_objects.end(); it++)
        {
            OscObject *o = world_objects[(*it).first];
            if (o) delete o;
        }

        world_objects.clear();
        UNLOCK_WORLD();
        clearFlag = false;
    }
    
    // Skip this timestep if world is being modified
    // TODO: improve this to avoid haptic glitches
    if (WORLD_LOCKED())
        return;

#ifdef ODE_IN_HAPTICS_LOOP
    // update ODE
	ode_simStep();
#endif
    
    cursor->computeGlobalPositions(1);
    
    // update the tool's pose and compute and apply forces
    cursor->updatePose();	 
    cursor->computeForces();
    cursor->applyForces();
    
    contactObject = NULL;
    for (unsigned int i=0; i<cursor->m_pointForceAlgos.size(); i++)
    {
        cProxyPointForceAlgo* pointforce_proxy = dynamic_cast<cProxyPointForceAlgo*>(cursor->m_pointForceAlgos[i]);
        if ((pointforce_proxy != NULL) && (pointforce_proxy->getContactObject() != NULL)) 
        {
            lastContactPoint = pointforce_proxy->getContactPoint();
            lastForce = cursor->m_lastComputedGlobalForce;
            contactObject = dynamic_cast<cODEPrimitive*> (pointforce_proxy->getContactObject());
            break;
        }

        cODEPotentialProxy* potential_proxy = dynamic_cast<cODEPotentialProxy*>(cursor->m_pointForceAlgos[i]);
        if ((potential_proxy != NULL) && (potential_proxy->getContactObject() != NULL)) 
        {
            lastContactPoint = potential_proxy->getContactPoint();
            lastForce = cursor->m_lastComputedGlobalForce;
            contactObject = dynamic_cast<cODEPrimitive*> (potential_proxy->getContactObject());
            break;
        }
    }
}

//---------------------------------------------------------------------------

void ode_simStep()
{
    // Add forces to an object in contact with the proxy
	if (contactObject) 
	{
		float x =  lastContactPoint.x;
		float y =  lastContactPoint.y;
		float z =  lastContactPoint.z;
        
		float fx = -force_scale*lastForce.x ;
		float fy = -force_scale*lastForce.y ;
		float fz = -force_scale*lastForce.z ;
        
		dBodyAddForceAtPos(contactObject->m_odeBody,fx,fy,fz,x,y,z);
	}

    // Allow constraints to update forces by calling each constraint's callback
    constraints_iter cit;
    for (cit=world_constraints.begin(); cit!=world_constraints.end(); cit++)
    {
        OscConstraint *oc = (*cit).second;
        oc->simulationCallback();
    }

    // Perform simulation step
	dSpaceCollide (ode_space,0,&ode_nearCallback);
	dWorldStepFast1 (ode_world, ode_step, 5);
	for (int j = 0; j < dSpaceGetNumGeoms(ode_space); j++){
		dSpaceGetGeom(ode_space, j);
	}
	dJointGroupEmpty (ode_contact_group);
    
    // Synchronize CHAI & ODE
    objects_iter oit;
    for (oit=world_objects.begin(); oit!=world_objects.end(); oit++)
    {
        cODEPrimitive *o = world_objects[(*oit).first]->odePrimitive();
        o->syncPose();
    }
}

//---------------------------------------------------------------------------

void initWorld()
{
    // create a new world
    world = new cWorld();
    
    // set background color
    world->setBackgroundColor(0.0f,0.0f,0.0f);
    
    // create a camera
    camera = new cCamera(world);
    world->addChild(camera);

    // position a camera
    camera->set( cVector3d (1.0, 0.0, 0.0),
                 cVector3d (0.0, 0.0, 0.0),
                 cVector3d (0.0, 0.0, 1.0));

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // Create a light source and attach it to the camera
    light = new cLight(world);
    light->setEnabled(true);
    light->setPos(cVector3d(2,0.5,1));
    light->setDir(cVector3d(-2,0.5,1));
    camera->addChild(light);

    // create a cursor and add it to the world.
    cursor = new cMeta3dofPointer(world, 0);

	// replace the cursor's proxy objects with an ODE equivalent
	cGenericPointForceAlgo* old_proxy = cursor->m_pointForceAlgos[0];
	cODEProxy *new_proxy = new cODEProxy(dynamic_cast<cProxyPointForceAlgo*>(old_proxy));
	new_proxy->enableDynamicProxy(true);
	cursor->m_pointForceAlgos[0] = new_proxy;
	delete old_proxy;

	old_proxy = (cPotentialFieldForceAlgo*)(cursor->m_pointForceAlgos[1]);
	cursor->m_pointForceAlgos[1] = new cODEPotentialProxy(dynamic_cast<cPotentialFieldForceAlgo*>(old_proxy));
	delete old_proxy;

    world->addChild(cursor);
    cursor->setPos(0.0, 0.0, 0.0);

    // set up a nice-looking workspace for the cursor so it fits nicely with our
    // cube models we will be building
    cursor->setWorkspace(1.0,1.0,1.0);

    // set the diameter of the ball representing the cursor
    cursor->setRadius(0.01);
}

void initGlutWindow()
{
    // initialize the GLUT windows
    glutInitWindowSize(512, 512);
    glutInitWindowPosition(0, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow("DEFAULT WINDOW");
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(rezizeWindow);
    glutSetWindowTitle("OSC for Haptics");

    // create a mouse menu
    glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("Window Display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // update display
    glutTimerFunc(GLUT_TIMESTEP_MS, updateDisplay, 0);
}

void initODE()
{
    ode_world = dWorldCreate();
    dWorldSetGravity (ode_world,0,0,0);
    ode_step = ODE_TIMESTEP_MS/1000.0;
    ode_space = dSimpleSpaceCreate(0);
    ode_contact_group = dJointGroupCreate(0);
}

void startHaptics()
{
    // set up the device
    if (cursor->initialize()) {
        printf("Could not initialize haptics.\n");
        return;
    }

    // open communication to the device
    if (cursor->start()) {
        printf("Could not start haptics.\n");
        return;
    }

    // start haptic timer callback
    timer.set(HAPTIC_TIMESTEP_MS, ode_hapticsLoop, NULL);

	hapticsStarted = 1;
	printf("Haptics started.\n");
}

void stopHaptics()
{
	if (hapticsStarted) {
        cursor->stop();
        timer.stop();
        
        hapticsStarted = 0;
        printf("Haptics stopped.\n");
	}
}

int hapticsEnable_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
	if (argv[0]->c==0)
		requestHapticsStop = 1;
	else
		requestHapticsStart = 1;
	return 0;
}

int graphicsEnable_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data)
{
	 if (argv[0]->c) {
		  if (!glutStarted) {
			   glutStarted = 1;
		  }
		  else {
			   glutShowWindow(); 
		  }
	 }
	 else if (glutStarted) {
		  glutHideWindow();
	 }
	 return 0;
}

int worldClear_handler(const char *path, const char *types, lo_arg **argv,
                       int argc, void *data, void *user_data)
{
    if (hapticsStarted)
        clearFlag = true;
    else {
        LOCK_WORLD();
        objects_iter it;
        for (it=world_objects.begin(); it!=world_objects.end(); it++)
        {
            OscObject *o = world_objects[(*it).first];
            if (o) delete o;
        }

        world_objects.clear();
        clearFlag = false;
        UNLOCK_WORLD();
    }
    return 0;
}

int worldGravity1_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    LOCK_WORLD();
    dWorldSetGravity(ode_world, 0, 0, argv[0]->f);
    UNLOCK_WORLD();
    return 0;
}

int worldGravity3_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    LOCK_WORLD();
    dWorldSetGravity(ode_world, argv[0]->f, argv[1]->f, argv[2]->f);
    UNLOCK_WORLD();
    return 0;
}

OscObject *findObject(const char *name)
{
    if (name==NULL || name[0]==0) return NULL;

    objects_iter it = world_objects.find(name);
    if (it!=world_objects.end())
        return world_objects[(*it).first];

    return NULL;
}

int objectPrismCreate_handler(const char *path, const char *types, lo_arg **argv,
                              int argc, void *data, void *user_data)
{
    if (findObject(&argv[0]->s))
        return 0;

    // Optional position, default (0,0,0)
	cVector3d pos;
	if (argc>0)
		 pos.x = argv[1]->f;
	if (argc>1)
		 pos.y = argv[2]->f;
	if (argc>2)
		 pos.z = argv[3]->f;

    // Default size
    cVector3d size(0.01,0.01,0.01);

    // Create object
    LOCK_WORLD();
    cODEPrism *pr = new cODEPrism(world,ode_world,ode_space,size);
    pr->setDynamicPosition(pos);
    pr->setMass(0.5);

    // Track the OSC object
    OscObject *ob=NULL;
    ob = new OscPrism(static_cast<cGenericObject*>(pr), &argv[0]->s);
    world_objects[&argv[0]->s] = ob;

    // Add to CHAI world
    world->addChild(pr);
    UNLOCK_WORLD();

    printf("Prism added at (%f, %f, %f).\n", pos.x, pos.y, pos.z);
    return 0;
}

int objectSphereCreate_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data)
{
    if (findObject(&argv[0]->s))
        return 0;

    // Optional position, default (0,0,0)
	cVector3d pos;
	if (argc>0)
		 pos.x = argv[1]->f;
	if (argc>1)
		 pos.y = argv[2]->f;
	if (argc>2)
		 pos.z = argv[3]->f;

    // Create object
    LOCK_WORLD();
    cODESphere *sp = new cODESphere(world,ode_world,ode_space,0.01);
    sp->setDynamicPosition(pos);
    sp->setMass(0.5);
    
    // Track the OSC object
    OscObject *ob=NULL;
    ob = new OscSphere(static_cast<cGenericObject*>(sp), &argv[0]->s);
    world_objects[&argv[0]->s] = ob;

    // Add to CHAI world
    world->addChild(sp);
    UNLOCK_WORLD();

    printf("Sphere added at (%f, %f, %f).\n", pos.x, pos.y, pos.z);
    return 0;
}

int constraintBallCreate_handler(const char *path, const char *types, lo_arg **argv,
                               int argc, void *data, void *user_data)
{
    if (argc!=6) return 0;

    if (world_constraints.find(&argv[0]->s)!=world_constraints.end())
        return 0;


    // Find first associated object
	OscObject *ob1 = NULL;
	objects_iter it = world_objects.find(&argv[1]->s);
	if (it==world_objects.end() || !(ob1 = world_objects[&argv[1]->s])) {
		 printf("Object %s doesn't exist.\n", &argv[1]->s);
		 return 0;
	}

    // Find second associated object.
    // String "world" indicates a constraint with a fixed point in space,
    // in which case ob2=NULL.
	OscObject *ob2 = NULL;
	if (std::string("world")!=&argv[2]->s) {
		 it = world_objects.find(&argv[2]->s);
		 if (it==world_objects.end() || !(ob2 = world_objects[&argv[2]->s])) {
			  printf("Object %s doesn't exist.\n", &argv[2]->s);
			  return 0;
		 }
    }

    // Track the OSC object
    LOCK_WORLD();
    OscBallJoint *cons=NULL;
    cons = new OscBallJoint(&argv[0]->s, ob1, ob2, argv[3]->f, argv[4]->f, argv[5]->f);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int constraintHingeCreate_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    if (argc!=9) return 0;

    if (world_constraints.find(&argv[0]->s)!=world_constraints.end())
        return 0;


    // Find first associated object
	OscObject *ob1 = NULL;
	objects_iter it = world_objects.find(&argv[1]->s);
	if (it==world_objects.end() || !(ob1 = world_objects[&argv[1]->s])) {
		 printf("Object %s doesn't exist.\n", &argv[1]->s);
		 return 0;
	}

    // Find second associated object.
    // String "world" indicates a constraint with a fixed point in space,
    // in which case ob2=NULL.
	OscObject *ob2 = NULL;
	if (std::string("world")!=&argv[2]->s) {
		 it = world_objects.find(&argv[2]->s);
		 if (it==world_objects.end() || !(ob2 = world_objects[&argv[2]->s])) {
			  printf("Object %s doesn't exist.\n", &argv[2]->s);
			  return 0;
		 }
    }

    // Track the OSC object
    LOCK_WORLD();
    OscHinge *cons=NULL;
    cons = new OscHinge(&argv[0]->s, ob1, ob2, argv[3]->f, argv[4]->f, argv[5]->f, argv[6]->f, argv[7]->f, argv[8]->f);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int constraintHinge2Create_handler(const char *path, const char *types, lo_arg **argv,
                                   int argc, void *data, void *user_data)
{
    if (argc!=12) return 0;

    if (world_constraints.find(&argv[0]->s)!=world_constraints.end())
        return 0;


    // Find first associated object
	OscObject *ob1 = NULL;
	objects_iter it = world_objects.find(&argv[1]->s);
	if (it==world_objects.end() || !(ob1 = world_objects[&argv[1]->s])) {
		 printf("Object %s doesn't exist.\n", &argv[1]->s);
		 return 0;
	}

    // Find second associated object.
    // String "world" indicates a constraint with a fixed point in space,
    // in which case ob2=NULL.
	OscObject *ob2 = NULL;
	if (std::string("world")!=&argv[2]->s) {
		 it = world_objects.find(&argv[2]->s);
		 if (it==world_objects.end() || !(ob2 = world_objects[&argv[2]->s])) {
			  printf("Object %s doesn't exist.\n", &argv[2]->s);
			  return 0;
		 }
    }

    // Track the OSC object
    LOCK_WORLD();
    OscHinge2 *cons=NULL;
    cons = new OscHinge2(&argv[0]->s, ob1, ob2,
                         argv[3]->f, argv[ 4]->f, argv[ 5]->f,
                         argv[6]->f, argv[ 7]->f, argv[ 8]->f,
                         argv[9]->f, argv[10]->f, argv[11]->f);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int constraintUniversalCreate_handler(const char *path, const char *types, lo_arg **argv,
                                      int argc, void *data, void *user_data)
{
    if (argc!=12) return 0;

    if (world_constraints.find(&argv[0]->s)!=world_constraints.end())
        return 0;


    // Find first associated object
	OscObject *ob1 = NULL;
	objects_iter it = world_objects.find(&argv[1]->s);
	if (it==world_objects.end() || !(ob1 = world_objects[&argv[1]->s])) {
		 printf("Object %s doesn't exist.\n", &argv[1]->s);
		 return 0;
	}

    // Find second associated object.
    // String "world" indicates a constraint with a fixed point in space,
    // in which case ob2=NULL.
	OscObject *ob2 = NULL;
	if (std::string("world")!=&argv[2]->s) {
		 it = world_objects.find(&argv[2]->s);
		 if (it==world_objects.end() || !(ob2 = world_objects[&argv[2]->s])) {
			  printf("Object %s doesn't exist.\n", &argv[2]->s);
			  return 0;
		 }
    }

    // Track the OSC object
    LOCK_WORLD();
    OscUniversal *cons=NULL;
    cons = new OscUniversal(&argv[0]->s, ob1, ob2,
                            argv[3]->f, argv[ 4]->f, argv[ 5]->f,
                            argv[6]->f, argv[ 7]->f, argv[ 8]->f,
                            argv[9]->f, argv[10]->f, argv[11]->f);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int constraintFixedCreate_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    if (argc!=3) return 0;

    if (world_constraints.find(&argv[0]->s)!=world_constraints.end())
        return 0;


    // Find first associated object
	OscObject *ob1 = NULL;
	objects_iter it = world_objects.find(&argv[1]->s);
	if (it==world_objects.end() || !(ob1 = world_objects[&argv[1]->s])) {
		 printf("Object %s doesn't exist.\n", &argv[1]->s);
		 return 0;
	}

    // Find second associated object.
    // String "world" indicates a constraint with a fixed point in space,
    // in which case ob2=NULL.
	OscObject *ob2 = NULL;
	if (std::string("world")!=&argv[2]->s) {
		 it = world_objects.find(&argv[2]->s);
		 if (it==world_objects.end() || !(ob2 = world_objects[&argv[2]->s])) {
			  printf("Object %s doesn't exist.\n", &argv[2]->s);
			  return 0;
		 }
    }

    // Track the OSC object
    LOCK_WORLD();
    OscFixed *cons=NULL;
    cons = new OscFixed(&argv[0]->s, ob1, ob2);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int unknown_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data)
{
    printf("Unknown message %s, %d args.\n", path, argc);
    return 0;
}

void liblo_error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

void initOSC()
{
	 /* start a new server on port 7770 */
	 loserver = lo_server_thread_new("7770", liblo_error);
     if (!loserver) {
         printf("Error starting OSC server on port 7770.\n");
         exit(1);
     }

	 /* add methods for each message */
	 lo_server_thread_add_method(loserver, "/haptics/enable", "i", hapticsEnable_handler, NULL);
	 lo_server_thread_add_method(loserver, "/graphics/enable", "i", graphicsEnable_handler, NULL);
	 lo_server_thread_add_method(loserver, "/object/prism/create", "sfff", objectPrismCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/object/prism/create", "s", objectPrismCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/object/sphere/create", "sfff", objectSphereCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/object/sphere/create", "s", objectSphereCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/constraint/ball/create", "sssfff", constraintBallCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/constraint/hinge/create", "sssffffff", constraintHingeCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/constraint/hinge2/create", "sssfffffffff", constraintHinge2Create_handler, NULL);
	 lo_server_thread_add_method(loserver, "/constraint/universal/create", "sssfffffffff", constraintUniversalCreate_handler, NULL);
	 lo_server_thread_add_method(loserver, "/constraint/fixed/create", "sss", constraintFixedCreate_handler, NULL);
     lo_server_thread_add_method(loserver, "/world/clear", "", worldClear_handler, NULL);
     lo_server_thread_add_method(loserver, "/world/gravity", "f", worldGravity1_handler, NULL);
     lo_server_thread_add_method(loserver, "/world/gravity", "fff", worldGravity3_handler, NULL);

     // TODO: this seems to get messages even when it is handled by another function (bug?)
     //lo_server_thread_add_method(loserver, NULL, NULL, unknown_handler, NULL);

	 lo_server_thread_start(loserver);

	 printf("OSC server initialized on port 7770.\n");
}

void sighandler_quit(int sig)
{
	requestHapticsStop = 1;

// TODO: wait for haptics to stop

    // delete all objects
    LOCK_WORLD();
    objects_iter it;
    for (it=world_objects.begin(); it!=world_objects.end(); it++)
    {
        OscObject *o = world_objects[(*it).first];
        if (o) delete o;
    }
    world_objects.clear();
    UNLOCK_WORLD();

    // quit the program
	if (glutStarted) {
#ifdef USE_FREEGLUT
		glutLeaveMainLoop();
#else
		exit(0);
#endif
	}
	 quit = 1;
	 return;
}

// poll waiting requests on the main thread
void poll_requests()
{
	if (requestHapticsStart) {
		if (!hapticsStarted)
			startHaptics();
		requestHapticsStart = 0;
	}

	if (requestHapticsStop) {
		if (hapticsStarted)
			stopHaptics();
		requestHapticsStop = 0;
		printf("Haptics stopped.\n");
	}

	if (globalForceMagnitude!=0) {
		lo_send(address_send, "/force/magnitude", "f", globalForceMagnitude);
		globalForceMagnitude = 0;
	}
}


int main(int argc, char* argv[])
{
	 // display pretty message
	 printf ("\n");
	 printf ("  =============================================\n");
	 printf ("  OSC for Haptics - CHAI 3D/GLUT implementation\n");
	 printf ("  Stephen Sinclair, IDMIL/CIRMMT 2006     \n");
	 printf ("  =============================================\n");
	 printf ("\n");

	 signal(SIGINT, sighandler_quit);

     // initialize all subsystems
	 initOSC();
	 initWorld();
	 initODE();

	 /*
	 cODEMesh *o1 = new cODEPrism(world, ode_world, ode_space, cVector3d(0.1,0.1,0.1));
	 objects["test1"] = o1;
	 world->addChild(o1);
     cVector3d pos(0,0.05,0);
     o1->setDynamicPosition(pos);
	 o1->setMass(2);

	 cODEMesh *o2 = new cODEPrism(world, ode_world, ode_space, cVector3d(0.1,0.1,0.1));
	 objects["test2"] = o2;
	 world->addChild(o2);
     pos.set(0,-0.05,0);
     o2->setDynamicPosition(pos);
	 o2->setMass(2);

	 o1->fixedLink("testlink", NULL);

	 cODESphere *o3 = new cODESphere(world, ode_world, ode_space, 0.04);
	 objects["sphere"] = o3;
	 world->addChild(o3);
     pos.set(0,0,0.1);
     o3->setDynamicPosition(pos);
	 o3->setMass(2);
     dBodySetLinearVel(o3->m_odeBody, 0, 0, -0.05);
	 */

	 // initially loop just waiting for messages
	 glutInit(&argc, argv);
	 while (!glutStarted && !quit) {
		  Sleep(100);
		  poll_requests();
	 }

	 // when graphics start, fall through to initialize GLUT stuff
	 if (glutStarted && !quit) {
		  initGlutWindow();
		  glutMainLoop();
	 }

	 requestHapticsStop = 1;
	 poll_requests();

	 return 0;
}

//---------------------------------------------------------------------------
