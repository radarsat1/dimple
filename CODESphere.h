
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
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

    //! Set radius of sphere
    virtual void setRadius (double a_radius);

  protected:
    // METHODS:
    //! Initialize the dynamic object.
    virtual void initDynamic(objectType a_objType = DYNAMIC_OBJECT,
                             float a_x = 0.0, float a_y = 0.0, float a_z = 0.0,
                             float a_density = 1.0);


    static void initCallbackDefaults(cODEPrimitive *self);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
