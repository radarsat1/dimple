
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple
*/
//======================================================================================

#define _WINSOCKAPI_
//---------------------------------------------------------------------------
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <queue>

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
#include "dimple.h"
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
float proxyForceMagnitude = 0;
int quit = 0;
int lock_ode = 0;
int lock_chai = 0;
pthread_t ode_pthread;

void poll_requests();

#define ODE_IN_HAPTICS_LOOP

#define MAX_CONTACTS 30
#define FPS 30
#define GLUT_TIMESTEP_MS   (int)((1.0/FPS)*1000.0)
#define HAPTIC_TIMESTEP_MS 1

// ODE objects
dWorldID ode_world;
double ode_step = 0;
dSpaceID ode_space;
dJointGroupID ode_contact_group;

cODEPrimitive *contactObject=NULL;
cVector3d lastForce;
cVector3d lastContactPoint;
double force_scale = 0.1;

int ode_counter = 0;
bool bGetCollide = false;

int mousepos[2];

#ifdef _POSIX
#define Sleep(t) usleep(t*1000)
#endif

// Synchronized class allowing seperate read and write locks & non-block operation
class sync_lock {
public:
    sync_lock() { writelock=0; readlock=0; }
    bool writelocked()   { return writelock!=0 && readlock!=0; }
    bool readlocked()    { return readlock!=0; }
    void lock_write()    { writelock++; }
    void unlock_write()  { writelock--; }
    void lock_read()     { readlock++; }
    void unlock_read()   { readlock--; }
protected:
    int writelock;
    int readlock;
};

template<typename T> class request_queue
    : public std::queue<T>, public sync_lock
{
public:
    void clean();
    void wait(T* req);
};

// This function should be called from a blockable thread
// because it performs memory deallocation.
template<typename T> void request_queue<T>::clean()
{
    // remove any handled requests from queue
    while (writelocked())
        Sleep(1);
    lock_write();
    while (   (std::queue<T>::size() > 0)
           && (std::queue<T>::front().handled))
        std::queue<T>::pop();
    unlock_write();
}

// This function should be called from a blockable thread.
template<typename T> void request_queue<T>::wait(T* req)
{
    // assuming the given req is on this queue...
    while (!req->handled)
        Sleep(1);

    // may as well do some clean-up since we're blocking anyway and
    // the previous request was handled.
    clean();
}

request_queue<ode_request_class> ode_queue;
request_queue<chai_request_class> chai_queue;

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
        if (cursor)
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

#ifdef ODE_IN_HAPTICS_LOOP
	// update ODE
    if (!WORLD_LOCKED() && !hapticsStarted)
        {}//ode_simStep();
#else
	// update ODE
    if (!WORLD_LOCKED())
        {}//ode_simStep();
#endif

        /*
    if (WORLD_LOCKED())
        while (poll_ode_requests());
        */
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
        OscObject *p1 = static_cast<OscObject*>(dGeomGetData(o1));
        OscObject *p2 = static_cast<OscObject*>(dGeomGetData(o2));
        if (p1 && p2) {
            bool co1 = p1->collidedWith(p2);
            bool co2 = p2->collidedWith(p1);
            if ( (co1 || co2) && bGetCollide ) {
                lo_send(address_send, "/object/collide", "ssf", p1->name(), p2->name(), (p1->getVelocity() + p2->getVelocity()).length());
                // TODO: send collision force
            }
            // TODO: this strategy will NOT work for multiple collisions between same objects!!
        }
		for (i=0; i<numc; i++) {
			dJointID c = dJointCreateContact (ode_world,ode_contact_group,contact+i);
			dJointAttach (c,b1,b2);
		}
	}
}

void ode_hapticsLoop(void* a_pUserData)
{
    bool cursor_ready = true;

    // process any waiting Chai messages
    while (poll_chai_requests());

    // Skip this timestep if world is being modified
    // TODO: improve this to avoid haptic glitches
    if (CHAI_LOCKED())
        return;

#ifdef ODE_IN_HAPTICS_LOOP
    // update ODE
	//ode_simStep();
#endif

    LOCK_CHAI();
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
    UNLOCK_CHAI();
}

//---------------------------------------------------------------------------

void ode_simStep()
{
    ode_counter ++;

    // Add forces to an object in contact with the proxy
    cODEPrimitive *ob = contactObject;
	if (ob) 
	{
		float x =  lastContactPoint.x;
		float y =  lastContactPoint.y;
		float z =  lastContactPoint.z;
        
		float fx = -force_scale*lastForce.x ;
		float fy = -force_scale*lastForce.y ;
		float fz = -force_scale*lastForce.z ;
        
		dBodyAddForceAtPos(ob->m_odeBody,fx,fy,fz,x,y,z);
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
    /*
	for (int j = 0; j < dSpaceGetNumGeoms(ode_space); j++){
		dSpaceGetGeom(ode_space, j);
	}
    */
	dJointGroupEmpty (ode_contact_group);
    
    // Synchronize CHAI & ODE
    // TODO: this should be done in haptics thread
    //       (check a flag to see if an ODE simstep has run,
    //       then synchronize without blocking)
    objects_iter oit;
    for (oit=world_objects.begin(); oit!=world_objects.end(); oit++)
    {
        OscObject *o = oit->second;
        o->odePrimitive()->syncPose();

        // Track object's position
        o->setDynamicPosition(dBodyGetPosition(o->odePrimitive()->m_odeBody));

        // Track object's velocity
        if (dGeomGetBody(o->odePrimitive()->m_odeGeom)==o->odePrimitive()->m_odeBody) {
            const dReal *vel = dBodyGetLinearVel(o->odePrimitive()->m_odeBody);
            o->setDynamicVelocity(vel);
        }
    }
}

//---------------------------------------------------------------------------

void* ode_threadproc(void*p)
{
    while (!quit) {
    fflush(stdout);
        Sleep((int)(ode_step*1000.0));
        while (poll_ode_requests());
        if (!ODE_LOCKED()) {
            LOCK_ODE();
            ode_simStep();
            UNLOCK_ODE();
        }
    }

    return 0;
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
}

void initCursor()
{
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
    glutCreateWindow("Dimple");
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(rezizeWindow);
    glutSetWindowTitle("Dimple");

    // create a mouse menu
    glutCreateMenu(setOther);
    glutAddMenuEntry("Full Screen", OPTION_FULLSCREEN);
    glutAddMenuEntry("Window Display", OPTION_WINDOWDISPLAY);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // update display
    glutTimerFunc(GLUT_TIMESTEP_MS, updateDisplay, 0);
}

void ode_errorhandler(int errnum, const char *msg, va_list ap)
{
    printf("ODE error %d: %s\n", errnum, msg);
}

void initODE()
{
    dSetDebugHandler(ode_errorhandler);
    dSetErrorHandler(ode_errorhandler);
    dSetMessageHandler(ode_errorhandler);

    ode_world = dWorldCreate();
    dWorldSetGravity (ode_world,0,0,0);
    ode_step = GLUT_TIMESTEP_MS/1000.0; // will be changed when haptics starts
    printf("ode_step = %f\n", ode_step);
    ode_space = dSimpleSpaceCreate(0);
    ode_contact_group = dJointGroupCreate(0);

    if (pthread_create(&ode_pthread, NULL, ode_threadproc, NULL))
        printf("Could not start ODE thread.\n");
}

void startHaptics()
{
    if (!cursor)
        initCursor();

    // set up the device
    if (cursor->initialize()) {
        printf("Could not initialize haptics.\n");
        world->deleteChild(cursor);
        cursor = NULL;
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

#ifdef ODE_IN_HAPTICS_LOOP
    /*
        ode_step = HAPTIC_TIMESTEP_MS / 1000.0;
    printf("ode_step = %f\n", ode_step);
    */
#endif
}

void stopHaptics()
{
	if (hapticsStarted) {
        if (cursor)
            cursor->stop();
        timer.stop();
        
        hapticsStarted = 0;
        printf("Haptics stopped.\n");
	}

#ifdef ODE_IN_HAPTICS_LOOP
        ode_step = GLUT_TIMESTEP_MS / 1000.0;
    printf("ode_step = %f\n", ode_step);
#endif
}

// called from OSC thread, non-blocking
ode_request_class* add_ode_request(ode_callback *callback, cODEPrimitive *ob)
{
    if (ode_queue.writelocked())
        return 0;

    ode_request_class r, *ret;
    r.callback = callback;
    r.ob = ob;

    ode_queue.lock_write();
    ode_queue.push(r);
    ret = (ode_request_class*)&ode_queue.back();
    ode_queue.unlock_write();
    return ret;
}

chai_request_class* add_chai_request(chai_callback *callback, cGenericObject *ob)
{
    if (chai_queue.writelocked())
        return 0;

    chai_request_class r, *ret;
    r.callback = callback;
    r.ob = ob;

    chai_queue.lock_write();
    chai_queue.push(r);
    ret = (chai_request_class*)&chai_queue.back();
    chai_queue.unlock_write();
    return ret;
}

// called from OSC thread
ode_request_class *post_ode_request(ode_callback *callback, cODEPrimitive *ob)
{
    ode_request_class *r=0;
    while (!(r=add_ode_request(callback, ob)))
        Sleep(1);

    // Take care of it right away if graphics isn't running
    // TODO: change this when ODE is on its own thread!
    /*
    if (!glutStarted)
        poll_ode_requests();
    */

    return r;
}

void wait_ode_request(ode_callback *callback, cODEPrimitive *ob)
{
    ode_request_class *r = post_ode_request(callback, ob);
    ode_queue.wait(r);
}

chai_request_class *post_chai_request(chai_callback *callback, cGenericObject *ob)
{
    chai_request_class *r=0;
    while (!(r=add_chai_request(callback, ob)))
        Sleep(1);

    // Take care of it right away if haptics isn't running
    /*
    if (!hapticsStarted)
        poll_chai_requests();
    */
    return r;
}

void wait_chai_request(chai_callback *callback, cGenericObject *ob)
{
    chai_request_class *r = post_chai_request(callback, ob);
    chai_queue.wait(r);
}

// called from respective ODE or CHAI thread
// return non-zero if requests remain in the queue
// handled events are flagged but not removed from the queue
// (to avoid memory de-allocation in real-time threads)

// TODO: only handles the first request, until it is taken
//       off the queue by another thread -- not ideal!
//       find better way to handle the memory de-allocation
//       problem, possibly a vector<> or list<> is a better
//       structure here.
int poll_ode_requests()
{
    bool handled = false;
    if (ode_queue.readlocked())
        return 0;
    ode_queue.lock_read();
    if (ode_queue.size()>0 && !ode_queue.front().handled) {
        ode_request_class *req = (ode_request_class*)&ode_queue.front();
        ode_queue.unlock_read();
        if (req->callback) req->callback(req->ob);
        req->handled = true;
        handled = true;
    }
    else
        ode_queue.unlock_read();

    return (int)handled;
}

int poll_chai_requests()
{
    bool handled = false;
    if (chai_queue.readlocked())
        return 0;
    chai_queue.lock_read();
    if (chai_queue.size()>0 && !chai_queue.front().handled) {
        chai_request_class *req = (chai_request_class*)&chai_queue.front();
        chai_queue.unlock_read();
        if (req->callback) req->callback(req->ob);
        req->handled = true;
        handled = true;
    }
    else
        chai_queue.unlock_read();

    return (int)handled;
}

int hapticsEnable_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
	if (argv[0]->i==0)
		requestHapticsStop = 1;
	else
		requestHapticsStart = 1;
	return 0;
}

int graphicsEnable_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data)
{
	 if (argv[0]->i) {
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

void clear_world(void*)
{
    objects_iter it;
    for (it=world_objects.begin(); it!=world_objects.end(); it++)
    {
        OscObject *o = it->second;
//        world_objects.erase(it);
        if (o) delete o;
    }
    
    world_objects.clear();
}

int worldClear_handler(const char *path, const char *types, lo_arg **argv,
                       int argc, void *data, void *user_data)
{
//    wait_ode_request(clear_world, 0);
    clear_world(0);
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
    WAIT_WORLD_LOCK();
    LOCK_WORLD();
    cODEPrism *pr = new cODEPrism(world,ode_world,ode_space,size);
    pr->setDynamicPosition(pos);
    pr->setMass(0.5);
    pr->m_material.setStaticFriction(1);
    pr->m_material.setDynamicFriction(0.5);
    pr->useMaterial(true);

    // Track the OSC object
    OscObject *ob=NULL;
    ob = new OscPrism(static_cast<cGenericObject*>(pr), &argv[0]->s);
    world_objects[&argv[0]->s] = ob;

    // Add to CHAI world
    world->addChild(pr);
    UNLOCK_WORLD();

    printf("Prism %s added at (%f, %f, %f).\n", ob->name(), pos.x, pos.y, pos.z);
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
    WAIT_WORLD_LOCK();
    LOCK_WORLD();
    cODESphere *sp = new cODESphere(world,ode_world,ode_space,0.01);
    sp->setDynamicPosition(pos);
    sp->setMass(0.5);
    sp->m_material.setStaticFriction(1);
    sp->m_material.setDynamicFriction(0.5);
    
    // Track the OSC object
    OscObject *ob=NULL;
    ob = new OscSphere(static_cast<cGenericObject*>(sp), &argv[0]->s);
    world_objects[&argv[0]->s] = ob;

    // Add to CHAI world
    world->addChild(sp);
    UNLOCK_WORLD();

    printf("Sphere %s added at (%f, %f, %f).\n", ob->name(), pos.x, pos.y, pos.z);
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
    WAIT_WORLD_LOCK();
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
    WAIT_WORLD_LOCK();
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
    WAIT_WORLD_LOCK();
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
    WAIT_WORLD_LOCK();
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
    WAIT_WORLD_LOCK();
    LOCK_WORLD();
    OscFixed *cons=NULL;
    cons = new OscFixed(&argv[0]->s, ob1, ob2);
    world_constraints[&argv[0]->s] = cons;
    UNLOCK_WORLD();

    return 0;
}

int objectCollideGet_handler(const char *path, const char *types, lo_arg **argv,
                             int argc, void *data, void *user_data)
{
    int interval=-1;
    if (argc > 0) {
        interval = argv[0]->i;
    }
    bGetCollide = (interval>0);

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

handler_data::handler_data(lo_method_handler _handler,
                           const char *_path, const char *_types,
                           lo_arg **_argv, int _argc, void *_user_data, int _thread)
    : handler(_handler), path(_path), types(_types), argc(_argc),
      user_data(_user_data), thread(_thread)
{
    argv = new lo_arg*[argc];
    int i;
    for (i=0; i<argc; i++)
        if (types[i]==LO_STRING) {
            argv[i] = (lo_arg*)strdup(&_argv[i]->s);
        }
        else {
            argv[i] = new lo_arg;
            memcpy(argv[i], _argv[i], sizeof(lo_arg));
        }
}

handler_data::~handler_data()
{
    int i;
    for (i=0; i<argc; i++)
        if (types[i]==LO_STRING)
            free(argv[i]);
        else
            delete argv[i];
    delete argv;
}

void thread_handler_callback(void *data)
{
    handler_data *h = (handler_data*)data;

//    printf("handler, thread = %d\n", h->thread);
    if (h->thread == DIMPLE_THREAD_PHYSICS)
        h->handler(h->path.c_str(), h->types.c_str(), h->argv, h->argc, 0, h->user_data);

    delete h;
}

int handler_callback(lo_method_handler handler, const char *path, const char *types,
                     lo_arg **argv, int argc, void *data, void *user_data)
{
    handler_data *h_ode = new handler_data(handler, path, types, argv, argc, user_data, DIMPLE_THREAD_PHYSICS);
    if (!h_ode)
        return 0;

    handler_data *h_chai = new handler_data(handler, path, types, argv, argc, user_data, DIMPLE_THREAD_HAPTICS);
    if (!h_chai) {
        delete h_ode;
        return 0;
    }

    ode_request_class *r1 = post_ode_request(thread_handler_callback, (cODEPrimitive*)h_ode);
    chai_request_class *r2 = post_chai_request(thread_handler_callback, (cGenericObject*)h_chai);

    // TODO: we only have to wait here because the implementation doesn't crawl
    // the message queue properly.  (only handles last one, but it is never cleaned)
    ode_queue.wait(r1);
    chai_queue.wait(r2);

    return 0;
}

void initOSC()
{
	 /* start a new server on port 7770 */
	 loserver = lo_server_thread_new("7770", liblo_error);
     if (!loserver) {
         printf("Error starting OSC server on port 7770.\n");
         exit(1);
     }

     lo_server_thread_set_handler_callback(loserver, handler_callback);

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
     lo_server_thread_add_method(loserver, "/object/collide/get", "", objectCollideGet_handler, NULL);
     lo_server_thread_add_method(loserver, "/object/collide/get", "i", objectCollideGet_handler, NULL);

	 lo_server_thread_start(loserver);

	 printf("OSC server initialized on port 7770.\n");
}

void sighandler_quit(int sig)
{
	requestHapticsStop = 1;

// TODO: wait for haptics to stop

    // delete all objects
    WAIT_WORLD_LOCK();
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
    if (!hapticsStarted)
        while (poll_chai_requests());

	if (requestHapticsStart) {
		if (!hapticsStarted)
			startHaptics();
		requestHapticsStart = 0;
	}

	if (requestHapticsStop) {
		if (hapticsStarted)
			stopHaptics();
		requestHapticsStop = 0;
	}
}

int main(int argc, char* argv[])
{
	 // display pretty message
	 printf ("\n");
	 printf ("  ==========================================================\n");
	 printf ("  DIMPLE: Dynamic Interactive Musically PhysicaL Environment\n");
	 printf ("  Version 0.0.4 (alpha).        Stephen Sinclair, IDMIL 2007\n");
	 printf ("  ==========================================================\n");
	 printf ("\n");
     fflush(stdout);

	 signal(SIGINT, sighandler_quit);

     // initialize all subsystems
	 initOSC();
	 initWorld();
	 initODE();

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

#if defined(WIN32) && defined(PTW32_STATIC_LIB)
class deal_with_pthread_win32
{
public:
	deal_with_pthread_win32() {
		/* initialize win32 pthreads */
		pthread_win32_process_attach_np();
	}
	~deal_with_pthread_win32() {
		/* clean up win32 pthreads */
		pthread_win32_process_detach_np();
	}
} _deal_with_pthread_win32;
#endif

//---------------------------------------------------------------------------
