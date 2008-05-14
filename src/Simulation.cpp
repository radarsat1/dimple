// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"scons debug=1" -*-

#include <lo/lo.h>
#include "dimple.h"
#include "Simulation.h"
#include "OscObject.h"

ShapeFactory::ShapeFactory(char *name, Simulation *parent)
    : OscBase(name, parent)
{
}

ShapeFactory::~ShapeFactory()
{
}

PrismFactory::PrismFactory(Simulation *parent)
    : ShapeFactory("prism", parent)
{
    // Name, Width, Height, Depth
    addHandler("create", "sfff", create_handler);
}

PrismFactory::~PrismFactory()
{
}

int PrismFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                 int argc, void *data, void *user_data)
{
    PrismFactory *me = static_cast<PrismFactory*>(user_data);
    me->create(&argv[0]->s, argv[1]->f, argv[2]->f, argv[3]->f);
    return 0;
}

SphereFactory::SphereFactory(Simulation *parent)
    : ShapeFactory("sphere", parent)
{
    // Name, Radius
    addHandler("create", "sfff", create_handler);
}

SphereFactory::~SphereFactory()
{
}

int SphereFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    SphereFactory *me = static_cast<SphereFactory*>(user_data);

    // Optional position, default (0,0,0)
	cVector3d pos;
	if (argc>0)
		 pos.x = argv[1]->f;
	if (argc>1)
		 pos.y = argv[2]->f;
	if (argc>2)
		 pos.z = argv[3]->f;

    if (!me->create(&argv[0]->s, pos.x, pos.y, pos.z))
        printf("[%s] Error creating sphere '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

HingeFactory::HingeFactory(Simulation *parent)
    : ShapeFactory("hinge", parent)
{
    // Name, object1, object2, x, y, z, axis x, y, z
    addHandler("create", "sssffffff", create_handler);
}

HingeFactory::~HingeFactory()
{
}

int HingeFactory::create_handler(const char *path, const char *types, lo_arg **argv,
                                  int argc, void *data, void *user_data)
{
    HingeFactory *me = static_cast<HingeFactory*>(user_data);
    OscObject *object1=0, *object2=0;

    if (argc != 9) return -1;

    if (strcmp(&argv[1]->s, "world")!=0)
        object1 = me->simulation()->find_object(&argv[1]->s);
    if (strcmp(&argv[2]->s, "world")!=0)
        object2 = me->simulation()->find_object(&argv[2]->s);

    // Swap objects if one is world
    if (object2 && !object1) {
        object1 = object2;
        object2 = 0;
    }

    // At least one object must exist
    if (!object1)
        return -1;

    // The objects cannot be the same.
    if (object1==object2)
        return -1;

    if (!me->create(&argv[0]->s, object1, object2,
                    argv[3]->f, argv[4]->f, argv[5]->f,
                    argv[6]->f, argv[7]->f, argv[8]->f))
        printf("[%s] Error creating hinge '%s'.\n",
               me->simulation()->type_str(), &argv[0]->s);
    return 0;
}

/****** Simulation *******/

Simulation::Simulation(const char *port, int type)
    : OscBase("world", NULL, lo_server_new(port, NULL))
{
    m_addr = lo_address_new("localhost", port);
    m_type = type;

    m_bDone = false;
    m_bStarted = false;
    if (pthread_create(&m_thread, NULL, Simulation::run, this))
        printf("[%s] Error creating simulation thread.", type_str());
    else
        m_bStarted = true;
}

Simulation::~Simulation()
{
    printf("[%s] Ending simulation... ", type_str());
    m_bDone = true;
    if (m_bStarted)
        pthread_join(m_thread, NULL);

    if (m_server)
        lo_server_free(m_server);
    printf("done.\n");

    lo_address_free(m_addr);
}

void* Simulation::run(void* param)
{
    Simulation* me = static_cast<Simulation*>(param);

    me->initialize();

    printf("[%s] Simulation running.\n", me->type_str());

    int step_ms = (int)(me->m_fTimestep*1000);
    int step_us = (int)(me->m_fTimestep*1000000);
    int step_left = step_ms;
    while (!me->m_bDone)
    {
        me->m_clock.initialize();
        me->m_clock.setTimeoutPeriod(step_us);
        me->m_clock.start();
        step_left = step_ms;
        while (lo_server_recv_noblock(me->m_server, step_left) > 0) {
            step_left = step_ms-(me->m_clock.getCurrentTime()/1000);
            if (step_left < 0) step_left = 0;
        }
        me->m_clock.stop();
        me->step();
        me->m_valueTimer.onTimer(step_ms);
    }

    return 0;
}

bool Simulation::add_object(OscObject& obj)
{
    world_objects[obj.name()] = &obj;

    printf("[%s] Added object %s\n", type_str(), obj.c_name());
    return true;
}

bool Simulation::delete_object(OscObject& obj)
{
    printf("[%s] Removing object %s\n", type_str(), obj.c_name());

    world_objects.erase(obj.name());
    delete &obj;

    return true;
}

OscObject* Simulation::find_object(const char* name)
{
    object_iterator it = world_objects.find(name);
    if (it!=world_objects.end()) return it->second;
    else
        return 0;
}

bool Simulation::add_constraint(OscConstraint& obj)
{
    world_constraints[obj.name()] = &obj;

    printf("[%s] Added constraint %s\n", type_str(), obj.c_name());
    return true;
}

// from liblo internals:
// eventually this will be a public function in liblo,
// but for now we'll reproduce it here.

static void add_varargs(/*lo_address t,*/ lo_message msg, va_list ap,
			const char *types)
{
    int count = 0;
    int i;
    int64_t i64;
    float f;
    char *s;
    lo_blob b;
    uint8_t *m;
    lo_timetag tt;
    double d;

    while (types && *types) {
	count++;
	switch (*types++) {

	case LO_INT32:
	    i = va_arg(ap, int32_t);
	    lo_message_add_int32(msg, i);
	    break;

	case LO_FLOAT:
	    f = (float)va_arg(ap, double);
	    lo_message_add_float(msg, f);
	    break;

	case LO_STRING:
	    s = va_arg(ap, char *);
	    if (s == (char *)LO_MARKER_A) { //error
	    } else
	    lo_message_add_string(msg, s);
	    break;

	case LO_BLOB:
	    b = va_arg(ap, lo_blob);
	    lo_message_add_blob(msg, b);
	    break;

	case LO_INT64:
	    i64 = va_arg(ap, int64_t);
	    lo_message_add_int64(msg, i64);
	    break;

	case LO_TIMETAG:
	    tt = va_arg(ap, lo_timetag);
	    lo_message_add_timetag(msg, tt);
	    break;

	case LO_DOUBLE:
	    d = va_arg(ap, double);
	    lo_message_add_double(msg, d);
	    break;

	case LO_SYMBOL:
	    s = va_arg(ap, char *);
	    if (s == (char *)LO_MARKER_A) { //error
	    } else
	    lo_message_add_symbol(msg, s);
	    break;

	case LO_CHAR:
	    i = va_arg(ap, int);
	    lo_message_add_char(msg, i);
	    break;

	case LO_MIDI:
	    m = va_arg(ap, uint8_t *);
	    lo_message_add_midi(msg, m);
	    break;

	case LO_TRUE:
	    lo_message_add_true(msg);
	    break;

	case LO_FALSE:
	    lo_message_add_false(msg);
	    break;

	case LO_NIL:
	    lo_message_add_nil(msg);
	    break;

	case LO_INFINITUM:
	    lo_message_add_infinitum(msg);
	    break;

	default:
        //error
	    break;
	}
    }
}

void Simulation::send(bool throttle, const char *path, const char *types, ...)
{
    va_list ap;
    lo_message msg = lo_message_new();
    va_start(ap, types);
    add_varargs(msg, ap, types);
    va_end(ap);

    std::vector<SimulationInfo>::iterator it;
    for (it=m_simulationList.begin();
         it!=m_simulationList.end();
         it++)
    {
        lo_send_message((*it).addr(), path, msg);
    }

    lo_message_free(msg);
}

void Simulation::sendtotype(int type, bool throttle, const char *path, const char *types, ...)
{
    va_list ap;
    lo_message msg = lo_message_new();
    va_start(ap, types);
    add_varargs(msg, ap, types);
    va_end(ap);

    std::vector<SimulationInfo>::iterator it;
    for (it=m_simulationList.begin();
         it!=m_simulationList.end();
         it++)
    {
        if ((*it).type() & type)
            lo_send_message((*it).addr(), path, msg);
    }

    lo_message_free(msg);
}

const char* Simulation::type_str()
{
    switch (m_type) {
    case ST_PHYSICS:
        return "physics";
    case ST_HAPTICS:
        return "haptics";
    case ST_VISUAL:
        return "visual";
    case ST_INTERFACE:
        return "interface";
    }
}
