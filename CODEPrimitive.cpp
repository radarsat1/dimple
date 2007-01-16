
//===========================================================================
/*
    This file is part of the CHAI 3D visualization and haptics libraries.
    Copyright (C) 2003-2004 by CHAI 3D. All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.

    For using the CHAI 3D libraries with software that can not be combined
    with the GNU GPL, and for taking advantage of the additional benefits
    of our support services, please contact CHAI 3D about acquiring a
    Professional Edition License.

    \author:    <http://www.chai3d.org>
    \author:    Chris Sewell
    \version    1.0
    \date       03/2005
*/
//===========================================================================

//---------------------------------------------------------------------------

#ifdef _MSVC
#pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints
#include <windows.h>
#include <conio.h>
#pragma warning (disable : 4786)
#pragma comment(lib,"oded.lib")
#pragma comment(lib,"opcode_d.lib")
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
cODEPrimitive::cODEPrimitive(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace,
							 cGenericObject &chaiObj)
{
    m_odeVertex = NULL;
    m_odeIndices = NULL;
    m_odeGeom = NULL;
    m_odeTriMeshData = NULL;
    m_odeBody   = NULL;
    m_odeWorld = a_odeWorld;
    m_odeSpace = a_odeSpace;
	m_chaiObj = chaiObj;
    m_chaiObj.m_lastRot.identity();
}


//===========================================================================
/*!
    Destructor of cODEPrimitive.

    \fn     cODEPrimitive::~cODEPrimitive()
*/
//===========================================================================
cODEPrimitive::~cODEPrimitive()
{
    if (m_odeVertex  != NULL)           delete []m_odeVertex;
    if (m_odeIndices != NULL)           delete []m_odeIndices;
    if (m_odeGeom != NULL)          dGeomDestroy(m_odeGeom);
    if (m_odeTriMeshData != NULL)       dGeomTriMeshDataDestroy(m_odeTriMeshData);
    if (m_odeBody != NULL)          dBodyDestroy(m_odeBody);
    
    m_Joint.clear();
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
        
        odePosition =  dGeomGetPosition(m_odeGeom);
        odeRotation =  dGeomGetRotation(m_odeGeom);
    }
    
    chaiRotation.set(odeRotation[0],odeRotation[1],odeRotation[2],
        odeRotation[4],odeRotation[5],odeRotation[6],
        odeRotation[8],odeRotation[9],odeRotation[10]);
    
    m_chaiObj.setRot(chaiRotation);
    m_chaiObj.setPos(odePosition[0],odePosition[1],odePosition[2]);
    
    m_chaiObj.computeGlobalPositions(1);    
}


//===========================================================================
/*!
Set the dynamic position of the cODEPrimitive to the given value.

  \fn     cODEPrimitive::setDynamicPosition(cVector3d &pos)
*/
//===========================================================================
void cODEPrimitive::setDynamicPosition(cVector3d &a_pos)
{
    dGeomSetPosition(m_odeGeom, a_pos.x, a_pos.y, a_pos.z);
    m_chaiObj.setPos(a_pos.x,a_pos.y,a_pos.z);
    m_chaiObj.computeGlobalPositions(1);    
    
    if (m_objType == DYNAMIC_OBJECT)
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
void cODEPrimitive::setMass(float a_mass)
{
  dMassAdjust(&m_odeMass, a_mass);
  dBodySetMass(m_odeBody,&m_odeMass);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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
    dJointAttach(m_Joint[id],m_odeBody,meshLinked->m_odeBody);
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

