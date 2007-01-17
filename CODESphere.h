
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

  protected:
    // METHODS:
    //! Initialize the dynamic object.
    virtual void initDynamic(objectType a_objType = DYNAMIC_OBJECT,
                             float a_x = 0.0, float a_y = 0.0, float a_z = 0.0,
                             float a_density = 1.0);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
