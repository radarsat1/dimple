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

#ifndef _OSC_VALUE_H_
#define _OSC_VALUE_H_

#include "OscBase.h"
#include <math/CVector3d.h>
#include <math/CMatrix3d.h>

using namespace chai3d;

/* === Macros for easily adding member functions with associated callbacks */

#define OSCVALUE(c, t, x, p) t m_##x;           \
    static void set_##x(void *data, t& s) \
    { p; ((c*)data)->on_##x(); }                \
    virtual void on_##x()
#define OSCSCALAR(c, o) OSCVALUE(c, OscScalar, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> %f\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.m_value)))
#define OSCBOOLEAN(c, o) OSCVALUE(c, OscBoolean, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> %s\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.m_value ? "true" : "false")))
#define OSCVECTOR3(c, o) OSCVALUE(c, OscVector3, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> (%f, %f, %f)\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.x(), s.y(), s.z())))
#define OSCMATRIX3(c, o) OSCVALUE(c, OscMatrix3, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> (%f, %f, %f; %f, %f, %f; %f, %f, %f)\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s(0,0), s(0,1), s(0,2), s(1,0), s(1,1), s(1,2), s(2,0), s(2,1), s(2,2))))
#define OSCSTRING(c, o) OSCVALUE(c, OscStrings, o, ptrace(((c*)data)->m_bTrace, ("[%s] %s." _dimple_str(o) " -> '%s'\n", ((c*)data)->simulation()->type_str(), ((c*)data)->c_path(), s.c_str())))
#define OSCMETHOD0(t, x)                                                \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->m_msg=data; \
        ptrace(((t*)user_data)->m_bTrace,                               \
               ("[%s] %s." _dimple_str(x) "()\n",                       \
                ((t*)user_data)->simulation()->type_str(),              \
                ((t*)user_data)->c_path()));                            \
        ((t*)user_data)->on_##x();return 0;}                            \
    virtual void on_##x()
#define OSCMETHOD1(t, x)                                                \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->m_msg=data;\
        ptrace(((t*)user_data)->m_bTrace,                               \
               ("[%s] %s." _dimple_str(x) "(%g)\n",                     \
                ((t*)user_data)->simulation()->type_str(),              \
                ((t*)user_data)->c_path(), argv[0]->f));                \
                           ((t*)user_data)->on_##x(argv[0]->f);return 0;}\
    virtual void on_##x(float arg)
#define OSCMETHOD2(t, x)                                                \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->m_msg=data;\
        ptrace(((t*)user_data)->m_bTrace,                               \
               ("[%s] %s." _dimple_str(x) "(%g, %g)\n",                 \
                ((t*)user_data)->simulation()->type_str(),              \
                ((t*)user_data)->c_path(), argv[0]->f, argv[1]->f));    \
                           ((t*)user_data)->on_##x(argv[0]->f, argv[1]->f);\
                           return 0;}                                   \
    virtual void on_##x(float arg1, float arg2)
#define OSCMETHOD1S(t, x)                                               \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->m_msg=data;\
        ptrace(((t*)user_data)->m_bTrace,                               \
               ("[%s] %s." _dimple_str(x) "(\"%s\")\n",                 \
                ((t*)user_data)->simulation()->type_str(),              \
                ((t*)user_data)->c_path(), &argv[0]->s));               \
                           ((t*)user_data)->on_##x(&argv[0]->s);return 0;}\
    virtual void on_##x(const char *arg1)
#define OSCMETHOD2S(t, x)                                               \
    static int x##_handler(const char *path, const char *types,         \
                           lo_arg **argv, int argc, void *data,         \
                           void *user_data) {((t*)user_data)->m_msg=data;\
        ptrace(((t*)user_data)->m_bTrace,                               \
               ("[%s] %s." _dimple_str(x) "(\"%s\", \"%s\")\n",         \
                ((t*)user_data)->simulation()->type_str(),              \
                ((t*)user_data)->c_path(), &argv[0]->s, &argv[1]->s));  \
                           ((t*)user_data)->on_##x(&argv[0]->s,         \
                           &argv[1]->s);return 0;} \
    virtual void on_##x(const char *arg1, const char *arg2)

/* === End of macro definitions. */

//! The OscValue class is the base class for all OSC-accessible values,
//! including vectors and scalars.
class OscValue : public OscBase
{
  public:
    OscValue(const char *name, OscBase *owner);
    virtual ~OscValue();
    virtual void send() = 0; //! Send the value to user receiver.

    typedef void SetCallback(void*, OscValue&);
    void setSetCallback(SetCallback*c, void*d)
      { m_set_callback = c; m_set_callback_data = d; }
    typedef void GetCallback(void*, OscValue&, int interval);
    void setGetCallback(GetCallback*c, void*d)
      { m_get_callback = c; m_get_callback_data = d; }

  protected:
    SetCallback *m_set_callback;
    void *m_set_callback_data;
    GetCallback *m_get_callback;
    void *m_get_callback_data;
    static int get_handler(const char *path, const char *types, lo_arg **argv,
                           int argc, void *data, void *user_data);
};

//! The OscScalar class is used to maintain information about scalar values
//! used throughout the OSC interface.
class OscScalar : public OscValue
{
  public:
    OscScalar(const char *name, OscBase *owner);

    //! Set the value with or without affecting the simulation.
	void setValue(double value, bool effect=true);

    void send();

    double m_value;

    typedef void SetCallback(void*, OscScalar&);
    void setSetCallback(SetCallback *c, void *d)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d); }
    typedef void GetCallback(void*, OscScalar&, int interval);
    void setGetCallback(GetCallback *c, void *d)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d); }

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

//! The OscBoolean class is used to maintain information about boolean values
//! used throughout the OSC interface.
class OscBoolean : public OscValue
{
  public:
    OscBoolean(const char *name, OscBase *owner);

    //! Set the value with or without affecting the simulation.
	void setValue(bool value, bool effect=true);

    void send();

    bool m_value;

    typedef void SetCallback(void*, OscBoolean&);
    void setSetCallback(SetCallback *c, void *d)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d); }
    typedef void GetCallback(void*, OscBoolean&, int interval);
    void setGetCallback(GetCallback *c, void *d)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d); }

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

//! The OscVector3 class is used to maintain information about 3-vector values
//! used throughout the OSC interface.
class OscVector3 : public OscValue, public cVector3d
{
  public:
    OscVector3(const char *name, OscBase *owner);

    //! Set the value with or without affecting the simulation.
    void setValue(double x, double y, double z, bool effect=true);

    //! Set the value with or without affecting the simulation.
    void setValue(const cVector3d& vec, bool effect=true)
        { setValue(vec.x(), vec.y(), vec.z(), effect); }

    void send();

	OscScalar m_magnitude;

    typedef void SetCallback(void*, OscVector3&);
    void setSetCallback(SetCallback *c, void *d)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d); }
    typedef void GetCallback(void*, OscVector3&, int interval);
    void setGetCallback(GetCallback *c, void *d)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d); }

  protected:
    static void set_magnitude_callback(OscVector3*, OscScalar&);
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

//! The OscMatrix3 class is used to maintain information about 3-matrix values
//! used throughout the OSC interface.
class OscMatrix3 : public OscValue, public cMatrix3d
{
  public:
    OscMatrix3(const char *name, OscBase *owner);

    //! Set the value with or without affecting the simulation.
    void setd(double m00, double m01, double m02,
              double m10, double m11, double m12,
              double m20, double m21, double m22, bool effect=true);
    void send();

    typedef void SetCallback(void*, OscMatrix3&);
    void setSetCallback(SetCallback *c, void *d)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d); }
    typedef void GetCallback(void*, OscMatrix3&, int interval);
    void setGetCallback(GetCallback *c, void *d)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d); }

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

class OscString : public OscValue, public std::string
{
  public:
    OscString(const char *name, OscBase *owner);
    void send();

    //! Set the string with or without affecting the simulation.
    void setValue(const std::string& s, bool effect=true);

    //! Set the string with or without affecting the simulation.
    void setValue(const char* s, bool effect=true);

    typedef void SetCallback(void*, OscString&);
    void setSetCallback(SetCallback *c, void *d)
        { OscValue::setSetCallback((OscValue::SetCallback*)c, d); }
    typedef void GetCallback(void*, OscString&, int interval);
    void setGetCallback(GetCallback *c, void *d)
        { OscValue::setGetCallback((OscValue::GetCallback*)c, d); }

  protected:
    static int _handler(const char *path, const char *types, lo_arg **argv,
                        int argc, void *data, void *user_data);
};

#endif // _OSC_VALUE_H_
