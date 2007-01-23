
//===========================================================================
/*
    This file is part of a proof-of-concept implementation for using
    Open SoundControl to interact with a haptic virtual environment.

    stephen.sinclair@mail.mcgill.ca
*/
//===========================================================================

//---------------------------------------------------------------------------
#ifndef CODEMeshH
#define CODEMeshH

#pragma warning(disable:4244 4305)  // for VC++, no precision loss complaints

//---------------------------------------------------------------------------
#include "CGenericObject.h"
#include "CMaterial.h"
#include "CTexture2D.h"
#include "CMesh.h"
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
class cTriangle;
class cVertex;
class cODEWorld;
//---------------------------------------------------------------------------


//===========================================================================
/*!
      \class      cODEMesh
      \brief      cODEMesh extends cMesh, connecting the CHAI mesh to an ODE
	              object.
*/
//===========================================================================
class cODEMesh : public cMesh, public cODEPrimitive
{
  public:
    // CONSTRUCTOR & DESTRUCTOR:
    //! Constructor of cODEMesh.
    cODEMesh(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace);
    //! Destructor of cODEMesh.
    ~cODEMesh();

    // METHODS:
	  //! Initialize the dynamic object.
	  virtual void initDynamic(geomType a_type = TRIMESH,objectType a_objType = DYNAMIC_OBJECT,
								float a_x = 0.0, float a_y = 0.0, float a_z = 0.0,
								float a_density = 1.0);
	  //! Inertia tensor.
	  float			cgx, cgy, cgz;
	  float			I11, I22, I33;
	  float			I12, I13, I23;
  private:
	  // METHODS:
	  //! Convert CHAI mesh to ODE mesh.	  
	  void cODEMeshToODE(unsigned int &a_vertexcount,unsigned int &a_indexcount);
	  //! Calculate the object's inertia tensor.
	  void calculateInertiaTensor(dReal density, float &mass,
								float &cgx, float &cgy, float &cgz,
								float &I11, float &I22, float &I33,
								float &I12, float &I13, float &I23);
	  //! Translate the whole mesh.
	  void translateMesh(float x, float y, float z);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------



