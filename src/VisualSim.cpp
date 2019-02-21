// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-

#include "config.h"
#include "dimple.h"
#include "VisualSim.h"
#include "HapticsSim.h"
#include "config.h"

#include <graphics/CFont.h>
#include <resources/CFontCalibri20.h>
#include <widgets/CLabel.h>
#include <math/CQuaternion.h>

#include <GL/glut.h>
#ifdef USE_FREEGLUT
#include <GL/freeglut.h>
#endif

using namespace chai3d;

bool VisualPrismFactory::create(const char *name, float x, float y, float z)
{
    printf("VisualPrismFactory (%s) is creating a prism object called '%s'\n",
           m_parent->c_name(), name);

    OscPrismCHAI *obj = new OscPrismCHAI(simulation()->world(),
                                         name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

    return true;
}

bool VisualSphereFactory::create(const char *name, float x, float y, float z)
{
    OscSphereCHAI *obj = new OscSphereCHAI(simulation()->world(),
                                           name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

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

    obj->m_position.setValue(x, y, z);

    return true;
}

int VisualVirtdevFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                         int argc, void *data, void *user_data)
{
    VisualVirtdevFactory *me = static_cast<VisualVirtdevFactory*>(user_data);

    // Optional position, default (0,0,0)
    cVector3d pos;
    if (argc>0)
        pos.x(argv[1]->f);
    if (argc>1)
        pos.y(argv[2]->f);
    if (argc>2)
        pos.z(argv[3]->f);

    OscObject *o = me->simulation()->find_object(&argv[0]->s);
    if (o)
        printf("[%s] Already an object named %s\n",
               me->simulation()->type_str(), &argv[0]->s);
    else
        if (!me->create(&argv[0]->s, pos.x(), pos.y(), pos.z()))
            printf("[%s] Error creating sphere '%s'.\n",
                   me->simulation()->type_str(), &argv[0]->s);
}

bool VisualVirtdevFactory::create(const char *name, float x, float y, float z)
{
    OscVisualVirtdevCHAI *obj = new OscVisualVirtdevCHAI(simulation()->world(),
                                             name, m_parent);

    if (!(obj && simulation()->add_object(*obj)))
            return false;

    obj->m_position.setValue(x, y, z);

    return true;
}

VisualVirtdevFactory::VisualVirtdevFactory(Simulation *parent)
    : ShapeFactory("virtdev", parent)
{
    // Name, Width, Height, Depth
    addHandler("create", "sfff", create_handler);
}

OscVisualVirtdevCHAI::OscVisualVirtdevCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscSphereCHAI(world, name, parent)
{
    // Create handles for manipulating virtual device in specific planes
    m_pHandleXY = new cShapeBox(0.1,0.1,0.001);
    m_pHandleXY->setLocalTransform(cTransform(cVector3d(0.05, 0.05, 0)));
    m_pHandleXY->m_material->setRed();
    m_pHandleXY->setWireMode(true, false);
    m_pSphere->addChild(m_pHandleXY);

    m_pHandleXZ = new cShapeBox(0.1,0.001,0.1);
    m_pHandleXZ->setLocalTransform(cTransform(cVector3d(-0.05, 0, -0.05)));
    m_pHandleXZ->m_material->setRed();
    m_pHandleXZ->setWireMode(true, false);
    m_pSphere->addChild(m_pHandleXZ);

    m_pSphere->deleteEffectSurface();
    m_color.set(1,0,0);
    m_pSphere->m_material->setRed();

    m_pSpecial = new CHAIObject(this, m_pSphere, world);
}

OscVisualVirtdevCHAI::~OscVisualVirtdevCHAI()
{
}

/* This struct is used to freeze camera transform during mouse interaction */
struct VisualSim::CameraProjection {
    cVector3d globalPos;
    cMatrix3d globalRot;
    double fieldViewAngleDeg;
    int windowWidth;
    int windowHeight;
    bool orthographicView;
    double orthographicWidth;
    cStereoMode stereoMode;
    bool mirrorHorizontal;
    bool mirrorVertical;
    void set(cCamera *cam, int width, int height);
    cVector3d projectOnWindowRay(const cVector3d& vec, int x, int y);
};

/****** VisualSim ******/

VisualSim *VisualSim::m_pGlobalContext = 0;

VisualSim::VisualSim(const char *port)
    : Simulation(port, ST_VISUAL),
      m_camera(NULL),
      m_bFullScreen(false),
      m_selectedObject(NULL)
{
    m_pPrismFactory = new VisualPrismFactory(this);
    m_pSphereFactory = new VisualSphereFactory(this);
    m_pMeshFactory = new VisualMeshFactory(this);
    m_pVirtdevFactory = new VisualVirtdevFactory(this);

    m_fTimestep = visual_timestep_ms/1000.0;
    printf("CHAI/GLUT timestep: %f\n", m_fTimestep);

    m_cameraProj = new CameraProjection();
}

VisualSim::~VisualSim()
{
    // Stop the simulation before deleting objects, otherwise thread
    // is still running and may dereference them.
    stop();

    delete m_cameraProj;
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
#ifdef GLEW_VERSION
    // initialize GLEW
    glewInit();
#endif
    glutDisplayFunc(draw);
    glutKeyboardFunc(key);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseMotion);

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

    // Create a light source and attach it to the camera so that it
    // moves with the point of view
    m_chaiLight0 = new cSpotLight(m_chaiWorld);
    m_chaiLight0->setEnabled(true);
    m_chaiLight0->setLocalPos(cVector3d(20,25,10));
    m_chaiLight0->setDir(-m_chaiLight0->getLocalPos());
    m_camera->object()->addChild(m_chaiLight0);

    // And a second one from the other side
    m_chaiLight1 = new cSpotLight(m_chaiWorld);
    m_chaiLight1->setEnabled(true);
    m_chaiLight1->setLocalPos(cVector3d(-30,5,15));
    m_chaiLight1->setDir(-m_chaiLight1->getLocalPos());
    m_camera->object()->addChild(m_chaiLight0);

    // Support shadows
    unsigned int i=0;
    cSpotLight *lt;
    while (lt = light(i++))
    {
        lt->setCutOffAngleDeg(30);
        lt->setShadowMapEnabled(true);
        lt->m_shadowMap->setQualityLow();
    }

    // Cursor object created by haptics sim after device initializes,
    // so nothing to do here for the cursor.

    // create a font
    cFontPtr font = NEW_CFONTCALIBRI20();
    cLabel* label = new cLabel(font);
    m_camera->object()->m_frontLayer->addChild(label);
    label->m_fontColor.setWhite();
    label->setText("my message");
    label->setLocalPos(10, label->getTextHeight());

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
    me->m_chaiWorld->updateShadowMaps(false, false);
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

/* This struct is used to freeze camera transform during mouse interaction */
void VisualSim::CameraProjection::set(cCamera *cam, int width, int height) {

    cVector3d projectOnWindowRay(const cVector3d& vec, int x, int y);
    globalPos = cam->getGlobalPos();
    globalRot = cam->getGlobalRot();
    fieldViewAngleDeg = cam->getFieldViewAngleDeg();

    // not exposed by Chai3d, but also not used by DIMPLE
    orthographicView = false;
    /*
      if (cam->getOrthographicView())
      orthographicWidth = cam->getOrthographicWidth();
    */

    stereoMode = cam->getStereoMode();
    mirrorHorizontal = cam->getMirrorHorizontal();
    mirrorVertical = cam->getMirrorVertical();

    windowWidth = width;
    windowHeight = height;
}

cVector3d VisualSim::CameraProjection::projectOnWindowRay(const cVector3d& vec, int x, int y)
{
    /* This function returns the closest point from some vector onto
     * the ray cast from the camera at the x,y position in window
     * coordinates.  Needed for dragging objects around with the mouse
     * on the plane parallel to the camera plane. */
    /* Adapted from Chai3d's selectWorld. */

    // store values
    int windowPosX = x;
    int windowPosY = y;
    double scaleFactorX = 1.0;
    double scaleFactorY = 1.0;

    cVector3d pos, dir;

    // adjust values when passive stereo is used
    if (stereoMode == C_STEREO_PASSIVE_LEFT_RIGHT)
    {
        double center = 0.5 * windowWidth;
        if (windowPosX > center)
        {
            windowPosX = windowPosX - (int)center;
        }
        windowWidth = (int)center;
        scaleFactorX = 2.0;
        scaleFactorY = 1.0;
    }
    else if (stereoMode == C_STEREO_PASSIVE_TOP_BOTTOM)
    {
        double center = 0.5 * windowHeight;
        if (windowPosY > center)
        {
            windowPosY = windowPosY - (int)center;
        }
        windowHeight = (int)center;
        scaleFactorX = 1.0;
        scaleFactorY = 2.0;
    }

    // adjust values when image is mirrored horizontally
    if (mirrorHorizontal)
    {
        windowPosX = windowWidth - windowPosX;
    }

    // adjust values when image is mirrored vertically
    if (mirrorVertical)
    {
        windowPosY = windowHeight - windowPosY;
    }

    // init variable to store result
    if (!orthographicView)
    {
        // make sure we have a legitimate field of view
        if (fabs(fieldViewAngleDeg) < 0.001f) { return vec; }

        // compute the ray that leaves the eye point at the appropriate angle
        //
        // m_fieldViewAngleDeg / 2.0 would correspond to the _top_ of the window
        double distCam = (scaleFactorY * windowHeight / 2.0f)
            / cTanDeg(fieldViewAngleDeg / 2.0f);

        cVector3d selectRay;
        selectRay.set(-distCam,
                      scaleFactorX * (windowPosX - (windowWidth / 2.0f)),
                      scaleFactorY * (windowPosY - (windowHeight / 2.0f)));
        selectRay.normalize();

        selectRay = cMul(globalRot, selectRay);

        pos = globalPos;
        dir = selectRay;
    }
    else
    {
        double hw = (double)(windowWidth) * 0.5;
        double hh = (double)(windowHeight)* 0.5;
        double aspect = windowWidth / windowHeight;

        // no getOrthographicWidth()
        double offsetX = ((windowPosX - hw) / hw) * 0.5 * orthographicWidth;
        double offsetY =-((windowPosY - hh) / hh) * 0.5 * (orthographicWidth / aspect);

        pos = cAdd(globalPos,
                   cMul(offsetX, globalRot.getCol1()),
                   cMul(offsetY, globalRot.getCol2()));

        dir = cNegate(globalRot.getCol0());
    }

    /* new location is the cloest point on the line perpendicular to
     * the camera plane */
    return cProjectPointOnLine(vec, pos, dir);
}

void VisualSim::mouseClick(int button, int state, int x, int y)
{
    VisualSim* me = VisualSim::m_pGlobalContext;

    // mouse button down
    if (state == GLUT_DOWN)
    {
        cCollisionRecorder recorder;
        cCollisionSettings settings;
        settings.m_checkForNearestCollisionOnly = true;
        settings.m_checkVisibleObjects = true;
        settings.m_checkHapticObjects = true;
        settings.m_ignoreShapes = false;

        // update my m_globalPos and m_globalRot variables
        me->m_chaiWorld->computeGlobalPositions(false);

        // detect for any collision between mouse and scene
        bool hit = me->m_camera->object()->
            selectWorld(x, me->m_nHeight-y, me->m_nWidth, me->m_nHeight, recorder, settings);
        OscObject *obj = nullptr;
        if (hit && recorder.m_nearestCollision.m_object)
        {
            me->m_selectionPlane = 0;
            OscVisualVirtdevCHAI* vdev = dynamic_cast<OscVisualVirtdevCHAI*>(me->world_objects["device"]);
            if (vdev)
                me->m_selectionPlane = vdev->getSelectionPlane(
                    recorder.m_nearestCollision.m_object);
            if (me->m_selectionPlane > 0)
                obj = vdev;
            else
                obj = (OscObject*)recorder.m_nearestCollision.m_object->m_userData;
        }
        else
            obj = (OscObject*)me->m_camera; // dangerous but we'll be careful

        if (obj) {
            // TODO: Problem if selected object is deleted!
            me->m_selectedObject = obj;

            // Freeze camera frame for interactive calculations
            CameraProjection& proj = *me->m_cameraProj;
            proj.set(me->m_camera->object(), me->m_nWidth, me->m_nHeight);

            cVector3d pos;
            if (obj == (OscObject*)me->m_camera)
                pos = me->m_camera->getLookat();
            else
                pos = obj->getPosition();
            me->m_selectionOffset = pos - proj.projectOnWindowRay(pos, x, me->m_nHeight-y);
            // TODO: Not sure why necessary to invert y axis here,
            // possibly related to cursor rotation in the
            // OscCursorCHAI constructor.
        }
    }
    else if (state == GLUT_UP)
        me->m_selectedObject = NULL;
}

int OscVisualVirtdevCHAI::getSelectionPlane(cGenericObject *obj)
{
    if (obj == m_pHandleXY)
        return 1;
    else if (obj == m_pHandleXZ)
        return 2;
    else
        return 0;
}

void VisualSim::mouseMotion(int x, int y)
{
    VisualSim* me = VisualSim::m_pGlobalContext;

    if (!me->m_selectedObject)
        return;

    if (me->m_selectedObject == (OscObject*)me->m_camera)
    {
        cVector3d vec1 = me->m_selectionOffset;
        cVector3d vec2 = me->m_camera->getLookat() - me->m_cameraProj->projectOnWindowRay(
            me->m_camera->getLookat(), x, me->m_nHeight-y);
        if (false)
        {
        //me->m_selectionOffset = vec2;
        vec1.normalize();
        vec2.normalize();

        // calculate rotation from vec1 to vec2 and apply same
        // rotation to camera position centered on lookat
        cQuaternion q1i(0, vec1.x(), vec1.y(), vec1.z());
        cQuaternion q2(0, vec2.x(), vec2.y(), vec2.z());
        q1i.conj();
        cQuaternion q(q1i*q2);
        cVector3d cv1v2( cCross(vec1,vec2) );
        q.w = sqrt(vec1.lengthsq()*vec2.lengthsq()) + vec1.dot(vec2);
        q.x = cv1v2.x(); q.y = cv1v2.y(); q.z = cv1v2.z();
        q.normalize();
        cQuaternion qi(q);
        qi.conj();

        cVector3d& pos = me->m_cameraProj->globalPos;
        cVector3d& lookat = me->m_camera->getLookat();
        cVector3d pl = pos - lookat;
        cQuaternion qpl(0, pl.x(), pl.y(), pl.z());
        qpl = q * qpl * qi;
        cVector3d newpos(cVector3d(qpl.x, qpl.y, qpl.z) + lookat);
        me->m_camera->getPosition().setValue(newpos);
        }
        else {
            cVector3d& pos = me->m_cameraProj->globalPos;
            cVector3d& lookat = me->m_camera->getLookat();
            cVector3d pl = pos - lookat;
            cMatrix3d rot(me->m_cameraProj->globalRot);
            rot.invert();

            double radius1 = pl.length();
            double theta1 = acos(pl.z() / radius1);
            double phi1 = atan(pl.y() / pl.x());

            cVector3d rotvec(cCross(rot.getCol0(), vec2 - vec1));
            theta1 += rotvec.x()*-3;
            phi1 += rotvec.z()*-3;

            cVector3d newvec(sin(theta1)*cos(phi1), sin(theta1)*sin(phi1), cos(theta1));
            newvec *= radius1;

            me->m_camera->getPosition().setValue(lookat + newvec);
        }
    }

    else // object, not camera
    {
        cVector3d pos;
        cVector3d planeNormal(0,1,0);
        switch (me->m_selectionPlane)
        {
        case 0:
            pos = me->m_cameraProj->projectOnWindowRay(
                me->m_selectedObject->getPosition(), x, me->m_nHeight-y) + me->m_selectionOffset;
            break;
        case 1: // XY
            planeNormal.set(0,0,1);
        case 2: { // XZ
            cVector3d planePos = me->m_selectedObject->getPosition() - me->m_selectionOffset;
            cCamera *cam = me->m_camera->object();
            double distCam = (me->m_nHeight / 2.0f)
                / cTanDeg(cam->getFieldViewAngleDeg() / 2.0f);
            cVector3d selectRay(-distCam,
                                (x - (me->m_nWidth / 2.0f)),
                                (me->m_nHeight-y - (me->m_nHeight / 2.0f)));
            selectRay.normalize();
            selectRay = cMul(cam->getGlobalRot(), selectRay);
            cVector3d segA = cam->getGlobalPos();
            cVector3d segB = segA+selectRay*1000;
            cVector3d colNormal;
            cIntersectionSegmentPlane(segA, segB, planePos, planeNormal, pos, colNormal);
            pos += me->m_selectionOffset;
            break;
        }
        }

        char msg[256];
        sprintf(msg, "%s/position", me->m_selectedObject->c_path());
        me->sendtotype(Simulation::ST_HAPTICS, true, msg, "fff", pos.x(), pos.y(), pos.z());
    }
}

cSpotLight *VisualSim::light(unsigned int i)
{
    return i==0?m_chaiLight0:(i==1?m_chaiLight1:nullptr);
}

OscCameraCHAI::OscCameraCHAI(cWorld *world, const char *name, OscBase *parent)
    : OscCamera(name, parent)
{
    m_pCamera = new cCamera(world);

    // position a camera such that X increases to the right, Y
    // increases into the screen, and Z is up.
    m_position.setValue(0.0, -1.0, 0.0);
    m_lookat.setValue(0.0, 0.0, 0.0);
    m_up.setValue(0.0, 0.0, 1.0);

    m_pCamera->set(m_position, m_lookat, m_up);

    // set the near and far clipping planes of the camera
    m_pCamera->setClippingPlanes(0.01, 10.0);
}

OscCameraCHAI::~OscCameraCHAI()
{
    if (m_pCamera)
        m_pCamera->getParent()->deleteChild(m_pCamera);
}
