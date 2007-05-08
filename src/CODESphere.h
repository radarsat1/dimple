
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
#ifndef CODESphereH
#define CODESphereH

#pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints

//---------------------------------------------------------------------------
#include "CGenericObject.h"
#include "CMaterial.h"
#include "CTexture2D.h"
#include "CShapeSphere.h"
#include "CColor.h"

//---------------------------------------------------------------------------
#include "GL/glu.h"
#include <vector>
#include <list>
//---------------------------------------------------------------------------
#include "ode/ode.h"
#include <string>
#include <map>
//---------------------------------------------------------------------------
#include "CODEPrimitive.h"

//---------------------------------------------------------------------------
class cWorld;
class cODEWorld;
//---------------------------------------------------------------------------


//===========================================================================
/*!
      \class cODESphere
	  \brief cODESphere extends cShapeSphere, connecting the CHAI mesh to an
	         ODE object.
*/
//===========================================================================
class cODESphere : public cShapeSphere, public cODEPrimitive
{
  public:
    // CONSTRUCTOR & DESTRUCTOR:
    //! Constructor of cODESphere.
    cODESphere(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace,
			   double radius);
    //! Destructor of cODESphere.
    ~cODESphere();

    //! Set radius of sphere
    virtual void setRadius (double a_radius);

    //! Set radius of dynamic sphere
	virtual void setDynamicRadius (double a_radius);

  protected:
    // METHODS:
    //! Initialize the dynamic object.
    virtual void initDynamic(objectType a_objType = DYNAMIC_OBJECT,
                             float a_x = 0.0, float a_y = 0.0, float a_z = 0.0,
                             float a_density = 1.0);


    static void initCallbackDefaults(void *self);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
