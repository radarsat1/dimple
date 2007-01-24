
#ifndef _OSC_PRISM_H_
#define _OSC_PRISM_H_

#include "CODEMesh.h"

class OscObject
{
  public:
	OscObject(cGenericObject* p) { m_objChai = p; }

	virtual cODEPrimitive*  odePrimitive() { return dynamic_cast<cODEPrimitive*>(m_objChai); }
	virtual cGenericObject* chaiObject()   { return m_objChai; }

  protected:
	cGenericObject* m_objChai;
};

class OscConstraint
{
  public:
};

class OscPrism : public OscObject
{
  public:
	OscPrism(cGenericObject* p) : OscObject(p) {}

	virtual cODEPrism* odePrimitive() { return dynamic_cast<cODEPrism*>(m_objChai); }
	virtual cMesh*     chaiObject()   { return dynamic_cast<cMesh*>(m_objChai); }
};

class OscSphere : public OscObject
{
  public:
	OscSphere(cGenericObject* p) : OscObject(p) {}

	virtual cODESphere*   odePrimitive() { return dynamic_cast<cODESphere*>(m_objChai); }
	virtual cShapeSphere* chaiObject()   { return dynamic_cast<cShapeSphere*>(m_objChai); }
};

#endif // _OSC_PRISM_H_
