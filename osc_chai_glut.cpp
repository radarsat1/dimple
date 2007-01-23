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
extern "C" {
#include "lo/lo.h"
}
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

// world objects
std::map<std::string,cODEPrimitive*> objects;
typedef std::map<std::string,cODEPrimitive*>::iterator objects_iter;

// width and height of the current viewport display
int width   = 0;
int height  = 0;

// menu options
const int OPTION_FULLSCREEN     = 1;
const int OPTION_WINDOWDISPLAY  = 2;

// OSC handlers
lo_server_thread st;

int glutStarted = 0;
int hapticsStarted = 0;
int requestHapticsStart = 0;
int requestHapticsStop = 0;
float globalForceMagnitude = 0;
int quit = 0;

void poll_requests();

#define MAX_CONTACTS 30
#define FPS 30
#define GLUT_TIMESTEP_MS   (int)((1.0/FPS)*1000.0)
#define HAPTIC_TIMESTEP_MS 1
#define ODE_TIMESTEP_MS    GLUT_TIMESTEP_MS
//#define ODE_TIMESTEP_MS    HAPTIC_TIMESTEP_MS

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

	// update ODE
	ode_simStep();
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
        objects_iter it;
        for (it=objects.begin(); it!=objects.end(); it++)
        {
            cGenericObject *o = dynamic_cast<cGenericObject*>(objects[(*it).first]);
            if (o) world->deleteChild(o);
        }

        objects.clear();
        clearFlag = false;
    }

    // update ODE
	//ode_simStep();

    // Synchronize CHAI & ODE
    /*
    objects_iter it;
    for (it=objects.begin(); it!=objects.end(); it++)
    {
        cODEPrimitive *o = objects[(*it).first];
        o->syncPose();
    }
    */
    
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
    
	dSpaceCollide (ode_space,0,&ode_nearCallback);
	dWorldStepFast1 (ode_world, ode_step, 5);
	for (int j = 0; j < dSpaceGetNumGeoms(ode_space); j++){
		dSpaceGetGeom(ode_space, j);
	}
	dJointGroupEmpty (ode_contact_group);
    
    // Synchronize CHAI & ODE
    objects_iter it;
    for (it=objects.begin(); it!=objects.end(); it++)
    {
        cODEPrimitive *o = objects[(*it).first];
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
    //dWorldSetGravity (ode_world,0,0,-5);
    dWorldSetGravity (ode_world,0,0,0);
    ode_step = ODE_TIMESTEP_MS/1000.0;
    ode_space = dSimpleSpaceCreate(0);
    ode_contact_group = dJointGroupCreate(0);
}

void startHaptics()
{
    // set up the device
    cursor->initialize();

    // open communication to the device
    cursor->start();

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
    clearFlag = true;
    return 0;
}

int worldGravity1_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    dWorldSetGravity(ode_world, 0, 0, argv[0]->f);
    return 0;
}

int worldGravity3_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    dWorldSetGravity(ode_world, argv[0]->f, argv[1]->f, argv[2]->f);
    return 0;
}

int world_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    printf("here.\n");
    return 0;
}

int objectCreate_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
	cVector3d pos;
	if (argc>2)
		 pos.x = argv[2]->f;
	if (argc>3)
		 pos.y = argv[3]->f;
	if (argc>4)
		 pos.z = argv[4]->f;

    cODEPrimitive *ob=NULL;
    if (std::string(&argv[1]->s)=="sphere") {
        cODESphere *sp = new cODESphere(world,ode_world,ode_space,0.01);
        ob = (cODEPrimitive*)sp;
        world->addChild(sp);
        printf("Sphere added at (%f, %f, %f).\n", pos.x, pos.y, pos.z);
    }

    else if (std::string(&argv[1]->s)=="prism") {
        cVector3d size(0.01,0.01,0.01);
        cODEPrism *pr = new cODEPrism(world,ode_world,ode_space,size);
        ob = (cODEPrimitive*)pr;
        world->addChild(pr);
        printf("Prism added at (%f, %f, %f).\n", pos.x, pos.y, pos.z);
    }

    else if (std::string(&argv[1]->s)=="cube") {
        cVector3d size(0.1,0.1,0.1);
        cODEPrism *pr = new cODEPrism(world,ode_world,ode_space,size);
        ob = (cODEPrimitive*)pr;
        world->addChild(pr);
        printf("Cube added at (%f, %f, %f).\n", pos.x, pos.y, pos.z);
    }

    if (ob) {
        ob->setDynamicPosition(pos);
        ob->setMass(0.5);
        objects[&argv[0]->s] = ob;
    }
    
	return 0;
}

int objectRadius_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    if (argc!=2)
        return 0;

	cODESphere *sphere = dynamic_cast<cODESphere*>( objects[&argv[0]->s] );
    if (sphere)
        sphere->setRadius(argv[1]->f);

	return 0;
}

int objectSize_handler(const char *path, const char *types, lo_arg **argv,
                       int argc, void *data, void *user_data)
{
    if (argc!=4)
        return 0;

	cODEPrism *prism = dynamic_cast<cODEPrism*>( objects[&argv[0]->s] );
    if (prism)
    {
        cVector3d size;
        size.x = argv[1]->f;
        size.y = argv[2]->f;
        size.z = argv[3]->f;
        prism->setSize(size);
    }

	return 0;
}

// Simple reg-exp function for OSC pattern matching
// Note: supports only * and ? for now
int osc_matcher(const char *matchpath, const char *path)
{
    const char *m = matchpath;
    const char *p = path;

    while (*p) {

        if (*m!=*p)
            return 0;

        m++;
        p++;

        enum {
            NORMAL,
            ANY,
            WILDCARD
        } state = NORMAL;

        while (*m && *p && *m!='/' && *p!='/') {
            switch (state) {
            case NORMAL:
                if (*m == '?') {
                    state = ANY;
                    continue;
                }
                if (*m == '*') {
                    state = WILDCARD;
                    continue;
                }
                if (*m!=*p)
                    return 0;
                m++;
                p++;
                break;
            case ANY:
                m++;
                p++;
                state = NORMAL;
                break;
            case WILDCARD:
                p++;
                break;
            }
        }

        if (state==WILDCARD) {
            m++;
            state = NORMAL;
        }

    }
    
    return 1;
}

int osc_handler(const char *path, const char *types, lo_arg **argv,
                       int argc, void *data, void *user_data)
{
    printf("OSC message: %s, %d args\n", path, argc);

    char str[1024];
    strcpy(str, path);
    char *s = strtok(str, "/");
    int n=0;
    while (s) {
        if (osc_matcher("/wor?d/*", path))
            printf("World message.\n");
        else if (osc_matcher("/object/*/test", path))
            printf("Object message.\n");
        s = strtok(NULL, "/");
        n++;
    }

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
	 st = lo_server_thread_new("7770", liblo_error);

	 /* add methods for each message */
	 lo_server_thread_add_method(st, "/haptics/enable", "i", hapticsEnable_handler, NULL);
	 lo_server_thread_add_method(st, "/graphics/enable", "i", graphicsEnable_handler, NULL);
	 lo_server_thread_add_method(st, "/object/create", "ssfff", objectCreate_handler, NULL);
     lo_server_thread_add_method(st, "/object/radius", "sf", objectRadius_handler, NULL);
     lo_server_thread_add_method(st, "/object/size", "sfff", objectSize_handler, NULL);
     lo_server_thread_add_method(st, "/world/clear", "", worldClear_handler, NULL);
     lo_server_thread_add_method(st, "/world/gravity", "f", worldGravity1_handler, NULL);
     lo_server_thread_add_method(st, "/world/gravity", "fff", worldGravity3_handler, NULL);
     lo_server_thread_add_method(st, NULL, NULL, osc_handler, NULL);

	 lo_server_thread_start(st);

	 printf("OSC server initialized on port 7770.\n");
}

void sighandler_quit(int sig)
{
	requestHapticsStop = 1;
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
		hapticsStarted = 1;
		requestHapticsStart = 0;
	}

	if (requestHapticsStop) {
		if (hapticsStarted)
			stopHaptics();
		hapticsStarted = 0;
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
