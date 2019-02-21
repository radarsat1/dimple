// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; -*-
//======================================================================================
/*
    This file is part of DIMPLE, the Dynamic Interactive Musically PhysicaL Environment,

    This code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License("GPL") version 2
    as published by the Free Software Foundation.  See the file LICENSE
    for more information.

    sinclair@music.mcgill.ca
    http://www.music.mcgill.ca/~sinclair/content/dimple
*/
//======================================================================================

#include "dimple.h"
#include "OscValue.h"
#include "ValueTimer.h"
#include "Simulation.h"
#include <lo/lo.h>

OscValue::OscValue(const char *name, OscBase *parent)
    : OscBase(name, parent)
{
    m_set_callback = NULL;
    m_set_callback_data = NULL;
    m_get_callback = NULL;
    m_get_callback_data = NULL;

    addHandler("get",           "i"  , OscValue::get_handler);
    addHandler("get",           ""   , OscValue::get_handler);
}

OscValue::~OscValue()
{
    simulation()->valuetimer().removeValue(this);
}

int OscValue::get_handler(const char *path, const char *types, lo_arg **argv,
                          int argc, void *data, void *user_data)
{
    OscValue *me = (OscValue*)user_data;
    
    if (me->m_get_callback) {
        me->m_get_callback(me->m_get_callback_data, *me, (argc==1)?argv[0]->i:-1);
        return 0;
    }

    if (argc==0) {
        me->send();
    }

    else if (argc==1) {
        if (argv[0]->i == 0)
            me->simulation()->valuetimer().removeValue(me);
        else if (argv[0]->i > 0)
            me->simulation()->valuetimer().addValue(me, argv[0]->i);
    }

    return 0;
}

// ----------------------------------------------------------------------------------

OscScalar::OscScalar(const char *name, OscBase *owner)
	 : OscValue(name, owner)
{
    m_value = 0;
    addHandler("",              "f", OscScalar::_handler);
}

void OscScalar::setValue(double value, bool effect)
{
	 m_value = value;
     if (m_set_callback && effect)
         m_set_callback(m_set_callback_data, *this);
}

void OscScalar::send()
{
    lo_send(address_send, c_path(), "f", m_value);
}

int OscScalar::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscScalar *me = static_cast<OscScalar*>(user_data);

	 if (argc == 1)
		  me->m_value = argv[0]->f;

     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);

	 return 0;
}
// ----------------------------------------------------------------------------------

OscBoolean::OscBoolean(const char *name, OscBase *owner)
	 : OscValue(name, owner)
{
    m_value = false;
    addHandler("", "i", OscBoolean::_handler);
}

void OscBoolean::setValue(bool value, bool effect)
{
	 m_value = value;
     if (m_set_callback && effect)
         m_set_callback(m_set_callback_data, *this);
}

void OscBoolean::send()
{
    lo_send(address_send, c_path(), "i", (int)m_value);
}

int OscBoolean::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscBoolean *me = static_cast<OscBoolean*>(user_data);

	 if (argc == 1)
		  me->m_value = argv[0]->i!=0;

     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);

	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscVector3 is a 3-vector which can report its magnitude.
OscVector3::OscVector3(const char *name, OscBase *owner)
    : OscValue(name, owner),
	  m_magnitude("magnitude", this),
      cVector3d()
{
    m_magnitude.setSetCallback((OscScalar::SetCallback*)set_magnitude_callback, this);

    addHandler("",              "fff", OscVector3::_handler);
}

//! Named setValue to avoid shadowing non-virtual cVector3d::set().
void OscVector3::setValue(double _x, double _y, double _z, bool effect)
{
    cVector3d::set(_x, _y, _z);
    m_magnitude.setValue(sqrt(_x*_x + _y*_y + _z*_z), false);
    if (m_set_callback && effect)
        m_set_callback(m_set_callback_data, *this);
}

void OscVector3::send()
{
    lo_send(address_send, c_path(), "fff", x(), y(), z());
}

void OscVector3::set_magnitude_callback(OscVector3 *me, OscScalar& s)
{
    double ratio;
    if (me->x()==0 && me->y()==0 && me->z()==0)
        ratio = 0;
    else
        ratio = s.m_value / me->length();
    
    *me *= ratio;

    if (me->m_set_callback)
        me->m_set_callback(me->m_set_callback_data, *me);
}

int OscVector3::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscVector3 *me = static_cast<OscVector3*>(user_data);

	 if (argc != 3)
         return 0;

     me->setValue(argv[0]->f, argv[1]->f, argv[2]->f);
     return 0;
}

// ----------------------------------------------------------------------------------

//! OscMatrix3 is a 3-matrix which can report its magnitude.
OscMatrix3::OscMatrix3(const char *name, OscBase *owner)
    : OscValue(name, owner),
      cMatrix3d()
{
    addHandler("",              "fffffffff", OscMatrix3::_handler);
}

//! Named setd to avoid shadowing non-virtual cMatrix3d::set().
void OscMatrix3::setd(double m00, double m01, double m02,
                      double m10, double m11, double m12,
                      double m20, double m21, double m22, bool effect)
{
    cMatrix3d::set(m00, m01, m02, m10, m11, m12, m20, m21, m22);
    if (m_set_callback && effect)
        m_set_callback(m_set_callback_data, *this);
}

void OscMatrix3::send()
{
    lo_send(address_send, c_path(), "fffffffff",
            (*this)(0,0), (*this)(0,1), (*this)(0,2),
            (*this)(1,0), (*this)(1,1), (*this)(1,2),
            (*this)(2,0), (*this)(2,1), (*this)(2,2)
        );
}

int OscMatrix3::_handler(const char *path, const char *types, lo_arg **argv,
                         int argc, void *data, void *user_data)
{
    OscMatrix3 *me = static_cast<OscMatrix3*>(user_data);

	 if (argc != 9)
         return 0;

     me->setd(argv[0]->f, argv[1]->f, argv[2]->f,
              argv[3]->f, argv[4]->f, argv[5]->f,
              argv[6]->f, argv[7]->f, argv[8]->f);
	 return 0;
}

// ----------------------------------------------------------------------------------

//! OscString is an OSC-accessible and -settable string value.
OscString::OscString(const char *name, OscBase *owner)
    : OscValue(name, owner)
{
    addHandler("",              "s",   OscString::_handler);
//    addHandler("get",           "i"  , OscString::get_handler);
//    addHandler("get",           ""   , OscString::get_handler);
}

void OscString::send()
{
    lo_send(address_send, c_path(), "s", c_str());
}

void OscString::setValue(const std::string& s, bool effect)
{
    assign(s);
    if (m_set_callback && effect)
        m_set_callback(m_set_callback_data, *this);
}

void OscString::setValue(const char* s, bool effect)
{
    assign(s);
    if (m_set_callback && effect)
        m_set_callback(m_set_callback_data, *this);
}

int OscString::_handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data)
{
    OscString *me = static_cast<OscString*>(user_data);

	 if (argc == 1) {
         me->assign(&argv[0]->s);
	 }
     
     if (me->m_set_callback)
         me->m_set_callback(me->m_set_callback_data, *me);

	 return 0;
}

