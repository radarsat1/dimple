
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple

    This file based on example code from CHAI 3D
*/
//======================================================================================

//---------------------------------------------------------------------------

#include "dimple.h"

#ifdef _MSVC
#pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints
#include <windows.h>
#include <conio.h>
#pragma warning (disable : 4786)
//#pragma comment(lib,"oded.lib")
//#pragma comment(lib,"opcode_d.lib")
#endif

#include "CODEPrimitive.h"
#include "CVertex.h"
#include "CTriangle.h"
#include "CMeshLoader.h"
#include "CCollisionBrute.h"
#include "CCollisionAABB.h"
#include "CCollisionSpheres.h"
#include <algorithm>

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

#define SQR(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))
#define X 0
#define Y 1
#define Z 2
//---------------------------------------------------------------------------

//===========================================================================
/*!
    Constructor of cODEPrimitive

    \fn       cODEPrimitive::cODEPrimitive(cWorld* a_parent)
    \param    a_parent  Pointer to parent world.
*/
//===========================================================================
cODEPrimitive::cODEPrimitive(cWorld* a_parent, dWorldID a_odeWorld,
                             dSpaceID a_odeSpace)
{
    m_odeVertex = NULL;
    m_odeIndices = NULL;
    m_odeGeom = NULL;
    m_odeTriMeshData = NULL;
    m_odeBody   = NULL;
    m_odeWorld = a_odeWorld;
    m_odeSpace = a_odeSpace;
    cGenericObject *o = dynamic_cast<cGenericObject*>(this);
    if (o) o->m_lastRot.identity();
}


//===========================================================================
/*!
    Destructor of cODEPrimitive.

    \fn     cODEPrimitive::~cODEPrimitive()
*/
//===========================================================================
cODEPrimitive::~cODEPrimitive()
{
//    wait_ode_request(destroyCallback, this);
    destroyCallback(this);
}

void cODEPrimitive::destroyCallback(cODEPrimitive *self)
{
    if (self->m_odeVertex  != NULL)      delete []self->m_odeVertex;
    if (self->m_odeIndices != NULL)      delete []self->m_odeIndices;
    if (self->m_odeGeom != NULL)         dGeomDestroy(self->m_odeGeom);
    if (self->m_odeTriMeshData != NULL)  dGeomTriMeshDataDestroy(self->m_odeTriMeshData);
    if (self->m_odeBody != NULL)         dBodyDestroy(self->m_odeBody);
    
    self->m_Joint.clear();
}

//===========================================================================
/*!
    Update the position and rotation of the cODEPrimitive to the body value.

    \fn     cODEPrimitive::updateDynamicPosition()
*/
//===========================================================================
void cODEPrimitive::updateDynamicPosition()
{    
    const float *odePosition;
    const float *odeRotation;
    cMatrix3d   chaiRotation;

    if (m_objType == DYNAMIC_OBJECT)
    {
        odePosition =  dBodyGetPosition(m_odeBody);
        odeRotation =  dBodyGetRotation(m_odeBody);
    }
    else {
        if (!m_odeGeom) return;
        odePosition =  dGeomGetPosition(m_odeGeom);
        odeRotation =  dGeomGetRotation(m_odeGeom);
    }
    
    chaiRotation.set(odeRotation[0],odeRotation[1],odeRotation[2],
        odeRotation[4],odeRotation[5],odeRotation[6],
        odeRotation[8],odeRotation[9],odeRotation[10]);
    
    cGenericObject *o = dynamic_cast<cGenericObject*>(this);
    if (o) {
        o->setRot(chaiRotation);
        o->setPos(odePosition[0],odePosition[1],odePosition[2]);    
        o->computeGlobalPositions(1);
    }
}


//===========================================================================
/*!
Set the dynamic position of the cODEPrimitive to the given value.

  \fn     cODEPrimitive::setDynamicPosition(cVector3d &pos)
*/
//===========================================================================
void cODEPrimitive::setDynamicPosition(cVector3d &a_pos)
{
    if (!m_odeGeom) return;
    dGeomSetPosition(m_odeGeom, a_pos.x, a_pos.y, a_pos.z);

    if (m_objType == DYNAMIC_OBJECT && m_odeBody)
    {
        dBodySetPosition(m_odeBody, a_pos.x, a_pos.y, a_pos.z);
    }
}

//===========================================================================
/*!
    Set a custom mass for the body.

    \fn     cODEPrimitive::setMass(dReal mass)
*/
//===========================================================================
void cODEPrimitive::setDynamicMass(float a_mass)
{
  dMassAdjust(&m_odeMass, a_mass);
  dBodySetMass(m_odeBody,&m_odeMass);
}

//===========================================================================
/*!
    Set a force on the body.

    \fn     cODEPrimitive::setDynamicForce(cVector3d &a_force)
*/
//===========================================================================
void cODEPrimitive::setDynamicForce(cVector3d &a_force)
{
    dBodySetForce(m_odeBody, a_force.x, a_force.y, a_force.z);
}

//===========================================================================
/*!
    Set the dynamic object's rotational position.

    \fn     cODEPrimitive::setDynamicRotation(cMatrix3d &a_rot)
*/
//===========================================================================
void cODEPrimitive::setDynamicRotation(cMatrix3d &a_rot)
{
    // Convert from a chai rotation matrix to an ODE rotation matrix
    dMatrix3 odeRotation;
    odeRotation[ 0] = a_rot.getCol0().x;
    odeRotation[ 1] = a_rot.getCol0().y;
    odeRotation[ 2] = a_rot.getCol0().z;
    odeRotation[ 3] = 0;
    odeRotation[ 4] = a_rot.getCol1().x;
    odeRotation[ 5] = a_rot.getCol1().y;
    odeRotation[ 6] = a_rot.getCol1().z;
    odeRotation[ 7] = 0;
    odeRotation[ 8] = a_rot.getCol2().x;
    odeRotation[ 9] = a_rot.getCol2().y;
    odeRotation[10] = a_rot.getCol2().z;
    odeRotation[11] = 0;
    dGeomSetRotation(m_odeGeom, odeRotation);
    dBodySetRotation(m_odeBody, odeRotation);
}

//===========================================================================
/*!
    Synchronize the pose of the CHAI object with the pose of the ODE body.

    \fn     cODEPrimitive::syncPose()
*/
//===========================================================================
void cODEPrimitive::syncPose()
{
    if (!m_odeGeom) return;

	const dReal* odePosition = dGeomGetPosition(m_odeGeom);
	const dReal* odeRotation = dGeomGetRotation(m_odeGeom);

	cMatrix3d   chaiRotation;
	chaiRotation.set(odeRotation[0],odeRotation[1],odeRotation[2],
		odeRotation[4],odeRotation[5],odeRotation[6],
		odeRotation[8],odeRotation[9],odeRotation[10]);

    cGenericObject *o = dynamic_cast<cGenericObject*>(this);
    if (o) {
        o->setRot(chaiRotation);
        o->setPos(odePosition[0],odePosition[1],odePosition[2]);
        o->computeGlobalPositions(1);
    }
}

//===========================================================================
/*!
    Create a ball linkage for the body.

    \fn     cODEPrimitive::ballLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor)
*/
//===========================================================================
void cODEPrimitive::ballLink     (string id,cODEPrimitive *meshLinked, cVector3d &anchor)
{
    m_Joint[id] = dJointCreateBall(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetBallAnchor(m_Joint[id],anchor.x, anchor.y, anchor.z);
}


//===========================================================================
/*!
    Create a hinged linkage for the body.

    \fn     cODEPrimitive::hingeLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
    cVector3d &axis)
*/
//===========================================================================
void cODEPrimitive::hingeLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
  cVector3d &axis)
{
    m_Joint[id] = dJointCreateHinge(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetHingeAnchor(m_Joint[id],anchor.x, anchor.y, anchor.z);
    dJointSetHingeAxis(m_Joint[id],axis.x, axis.y, axis.z);
}


//===========================================================================
/*!
    Create a hinged linkage for the body.

    \fn     cODEPrimitive::hinge2Link   (string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
    cVector3d &axis1, cVector3d &axis2)
*/
//===========================================================================
void cODEPrimitive::hinge2Link   (string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
  cVector3d &axis1, cVector3d &axis2)
{
    m_Joint[id] = dJointCreateHinge2(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetHinge2Anchor(m_Joint[id],anchor.x, anchor.y, anchor.z);
    dJointSetHinge2Axis1(m_Joint[id],axis1.x, axis1.y, axis1.z);
    dJointSetHinge2Axis2(m_Joint[id],axis2.x, axis2.y, axis2.z);
}


//===========================================================================
/*!
    Create a slider linkage for the body.

    \fn     cODEPrimitive::sliderLink(string id,cODEPrimitive *meshLinked, 
    cVector3d &anchor, cVector3d &axis)
*/
//===========================================================================
void cODEPrimitive::sliderLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
  cVector3d &axis)
{
    m_Joint[id] = dJointCreateSlider(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetSliderAxis(m_Joint[id],axis.x, axis.y, axis.z);
}


//===========================================================================
/*!
    Create a universal linkage for the body.

    \fn     cODEPrimitive::universalLink(string id,cODEPrimitive *meshLinked, 
    cVector3d &anchor, cVector3d &axis1, cVector3d &axis2)
*/
//===========================================================================
void cODEPrimitive::universalLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor, 
  cVector3d &axis1, cVector3d &axis2)
{
    m_Joint[id] = dJointCreateUniversal(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetUniversalAnchor(m_Joint[id],anchor.x, anchor.y, anchor.z);
    dJointSetUniversalAxis1(m_Joint[id],axis1.x, axis1.y, axis1.z);
    dJointSetUniversalAxis2(m_Joint[id],axis2.x, axis2.y, axis2.z);
}


//===========================================================================
/*!
    Create a fixed linkage for the body.

    \fn     ccODEPrimitive::fixedLink    (string id,cODEPrimitive *meshLinked)
*/
//===========================================================================
void cODEPrimitive::fixedLink    (string id,cODEPrimitive *meshLinked)
{
    m_Joint[id] = dJointCreateFixed(m_odeWorld,0);
    dJointAttach(m_Joint[id],m_odeBody,meshLinked?(meshLinked->m_odeBody):0);
    dJointSetFixed(m_Joint[id]);
}


//===========================================================================
/*!
    Destroy a joint in the body.

    \fn     cODEPrimitive::destroyJoint(string id)
*/
//===========================================================================
bool cODEPrimitive::destroyJoint(string id)
{  
    std::map<string,dJointID>::iterator cur = m_Joint.find(id);

    if (cur == m_Joint.end()) 
      return false;
    else {
      dJointDestroy(m_Joint[(*cur).first]);
      m_Joint.erase(cur);
      return true;
    }
}


//===========================================================================
/*!
    Get one of the body's joints.

    \fn     cODEPrimitive::getJoint(string id, dJointID* &pJoint)
*/
//===========================================================================
bool cODEPrimitive::getJoint(string id, dJointID* &pJoint)
{  
    std::map<string,dJointID>::iterator cur = m_Joint.find(id);
    pJoint = NULL;

    if (cur == m_Joint.end())
    {
      return false;
    }
    else
    {
      pJoint = &m_Joint[(*cur).first];
      return true;
    }
}

//===========================================================================
void cODEPrimitive::removeBody()
{
    dGeomSetBody(m_odeGeom, 0);
}

//===========================================================================
void cODEPrimitive::restoreBody()
{
    dGeomSetBody(m_odeGeom, m_odeBody);
}
