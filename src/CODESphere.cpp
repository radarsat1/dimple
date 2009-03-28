
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

#ifdef _MSVC
#pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints
#endif

#include "dimple.h"
#include "CODESphere.h"
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
    Constructor of cODESphere

    \fn       cODESphere::cODESphere(cWorld* a_parent)
    \param    a_parent  Pointer to parent world.
*/
//===========================================================================
cODESphere::cODESphere(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace,
					   double radius)
	: cShapeSphere(radius),
	  cODEPrimitive(a_parent, a_odeWorld, a_odeSpace)
{
    m_objClass = CLASS_SPHERE;
    UNLOCK_WORLD();
//    wait_ode_request(initCallbackDefaults, this);
    initCallbackDefaults(this);
    LOCK_WORLD();
}

//===========================================================================
/*!
    Destructor of cODESphere.

    \fn     cODESphere::~cODESphere()
*/
//===========================================================================
cODESphere::~cODESphere()
{
    m_Joint.clear();
}

//===========================================================================
/*!
    Initialize the dynamic for the cODESphere.

    \fn     cODESphere::initDynamic()
*/
//===========================================================================
void cODESphere::initCallbackDefaults(void *self)
{
    (static_cast<cODESphere*>(self))->initDynamic();
}

void cODESphere::initDynamic(objectType a_objType, float a_x, 
                             float a_y, float a_z, float a_density) 
{
    // Create a sphere geometry.
    m_odeGeom = dCreateSphere(m_odeSpace, m_radius);

    dGeomSetPosition(m_odeGeom, a_x, a_y, a_z);
    setPos(a_x,a_y,a_z);
    
    m_lastPos = m_localPos;
    // computeGlobalCurrentObjectOnly(true);
    computeGlobalPositions(1);
    
    m_objType = a_objType;
    if (a_objType == DYNAMIC_OBJECT) 
    {
        //Create the body,...the physical entity...in the world.
        //The world is the physic engine.
        //if (odeBody != NULL) dBodyDestroy(odeBody);
        
        m_odeBody          = dBodyCreate(m_odeWorld);
        dBodySetPosition(m_odeBody, a_x, a_y, a_z);
        
        //Create the mass entity, calculate the inertia matrix and link all at the body.
        //dMassSetParameters(&odeMass,m_Mass,cgx,cgy,cgz,I11,I22,I33,I12,I13,I23);
        
        dMassSetSphere(&m_odeMass,a_density,m_radius);
        dBodySetMass(m_odeBody,&m_odeMass);
        dGeomSetBody(m_odeGeom,m_odeBody);
        
        dBodySetPosition(m_odeBody, a_x, a_y, a_z);
    }
}

void cODESphere::setRadius (double a_radius)
{
    cShapeSphere::setRadius(a_radius);
}

void cODESphere::setDynamicRadius (double a_radius)
{
    dGeomSphereSetRadius(m_odeGeom, a_radius);
}