
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
#ifndef CODEPrimitiveH
#define CODEPrimitiveH

//---------------------------------------------------------------------------
#include "CGenericObject.h"
#include "CMaterial.h"
#include "CTexture2D.h"
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


//Geometry type to use for the mesh.
enum geomType{
	TRIMESH,
	BOX,
	SPHERE
};

enum objectType {
	STATIC_OBJECT,
	DYNAMIC_OBJECT
};

typedef int odeVector3[3];
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
class cWorld;
class cTriangle;
class cVertex;
class cODEWorld;
//---------------------------------------------------------------------------


//===========================================================================
/*!
      \class      cODEPrimitive
      \brief      cODEPrimitive extends cMesh, connecting the CHAI mesh to an ODE
	              object.
*/
//===========================================================================
class cODEPrimitive
{
  public:
    // CONSTRUCTOR & DESTRUCTOR:
    //! Constructor of cODEPrimitive.
    cODEPrimitive(cWorld* a_parent, dWorldID a_odeWorld, dSpaceID a_odeSpace, cGenericObject &chaiObj);
    //! Destructor of cODEPrimitive.
    ~cODEPrimitive();

    // METHODS:
	  //! Initialize the dynamic object.
	  virtual void initDynamic(geomType a_type = TRIMESH,objectType a_objType = DYNAMIC_OBJECT,
								float a_x = 0.0, float a_y = 0.0, float a_z = 0.0,
								float a_density = 1.0) {};
	  //! Update the position of the dynamic object.
	  void updateDynamicPosition();
	  //! Set the position of the dynamic object.
	  void setDynamicPosition(cVector3d &a_pos);
	  //! Set the mass of the dynamic object.
	  void setMass(float a_mass);
	  //! Sync the pose of the ODE object with the pose of the CHAI object
	  void syncPose();
  
 	  //! List of names of the joints.
	  std::map<std::string ,dJointID> m_Joint;
	  //! Create a ball linkage.
	  void ballLink     (string id,cODEPrimitive *meshLinked, cVector3d &anchor);
	  //! Created a hinged linkage.
	  void hingeLink    (string id,cODEPrimitive *meshLinked, cVector3d &anchor, cVector3d &axis);
	  //! Create a double hinged linkage.
	  void hinge2Link   (string id,cODEPrimitive *meshLinked, cVector3d &anchor, cVector3d &axis1, cVector3d &axis2);
	  //! Create a slider linkage.
	  void sliderLink   (string id,cODEPrimitive *meshLinked, cVector3d &anchor, cVector3d &axis);
	  //! Create a universal linkage.
	  void universalLink(string id,cODEPrimitive *meshLinked, cVector3d &anchor, cVector3d &axis1, cVector3d &axis2);
	  //! Create a fixed linkage.
	  void fixedLink    (string id,cODEPrimitive *meshLinked);
    //! Destroy a joint.
	  bool destroyJoint(string id);
	  //! Get one of the body's joints.
	  bool getJoint(string id,dJointID* &pJoint);

    // MEMBERS:
	  //! List of vertices for ODE.
	  float	*m_odeVertex;
	  //! List of vertex indices for ODE.
	  odeVector3 *m_odeIndices;
	
	  //! Geometry for ODE.
	  dGeomID m_odeGeom;
	  //! TriMesh data for ODE.
	  dTriMeshDataID  m_odeTriMeshData;
	  //! Body id for ODE.
	  dBodyID	m_odeBody;
	  //! Mass for ODE.
	  dMass	m_odeMass;
    //! Pointer to ODE world in which this mesh lives.
	  dWorldID m_odeWorld;
	  //! ODE space.
	  dSpaceID m_odeSpace;
	  //! Mass.
	  float			m_Mass;
	  //! Object type.
	  objectType		m_objType;

      //!This is specific to the OSC-Haptics application
      enum objectClass {
          CLASS_SPHERE,
          CLASS_PRISM,
          CLASS_MESH
      } m_objClass;
  private:
	  //! CHAI object
	  cGenericObject &m_chaiObj;
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------



