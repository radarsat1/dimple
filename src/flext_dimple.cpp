// -*- mode:c++; indent-tabs-mode:nil; c-basic-offset:4; compile-command:"../build.sh" -*-
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

/* 
Based on flext tutorials by Thomas Grill
*/

#include <flext.h>
#include "dimple.h"
#include "AudioStreamer.h"
#include <queue>
#include <string>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1
#endif


class dimple:
	public flext_dsp
{
	FLEXT_HEADER(dimple,flext_dsp)

public:
    dimple(int argc, t_atom *argv);
	~dimple();

protected:
	void m_any(const t_symbol *s,int argc,t_atom *argv);
	void m_timer(void*);
    virtual void m_signal(int n, float *const *in, float *const *out);

    int m_input_channels;
    int m_output_channels;

	Timer timer;
private:
	FLEXT_CALLBACK_A(m_any);
	FLEXT_CALLBACK_T(m_timer);
};

static std::queue<dimple::AtomList> send_queue;

FLEXT_NEW_DSP_V("dimple~",dimple)

// constructor

dimple::dimple(int argc, t_atom *argv)
{ 
	// Initialize DIMPLE:
	dimple_main(0, NULL);

	AddInAnything("DIMPLE message input");
	AddOutAnything("DIMPLE message output");

    int i;
    m_input_channels = 0;
    if (argc > 0)
        m_input_channels = GetInt(argv[0]);
    if (m_input_channels > 4)
        m_input_channels = 4;
    if (m_input_channels < 0)
        m_input_channels = 0;
    for (i=1; i<m_input_channels; i++) {
        char str[256];
        sprintf(str, "signal input %d", i-1);
        AddInSignal(str);
    }

    m_output_channels = 0;
    if (argc > 1)
        m_output_channels = GetInt(argv[1]);
    if (m_output_channels > 4)
        m_output_channels = 4;
    if (m_output_channels < 0)
        m_output_channels = 0;
    for (i=0; i<m_output_channels; i++) {
        char str[256];
        sprintf(str, "signal output %d", i);
        AddOutSignal(str);
    }

	// register methods
	FLEXT_ADDMETHOD(0,m_any);

    // register timer
    FLEXT_ADDTIMER(timer, m_timer);
    timer.Periodic(0.001);

    // set-up AudioStreamer instances
    audioStreamer = new AudioStreamer*[1];
    audioStreamer[0] = new AudioStreamer(Samplerate(), 1000, 50, 1);
}

dimple::~dimple()
{
    printf("Cleaning up DIMPLE.\n");
    dimple_cleanup();
}


// method

// HACK!
struct _lo_server {
	int	                 socket;
	struct addrinfo         *ai;
	lo_method                first;
	lo_err_handler           err_h;
	int	 	         port;
	char                   	*hostname;
	char                   	*path;
	int            	         protocol;
	void		        *queued;
	struct sockaddr_storage  addr;
	socklen_t 	         addr_len;
    lo_dispatch_method_callback dispatch_callback;
    lo_handler_callback handler_callback;
};

void dimple::m_any(const t_symbol *s,int argc,t_atom *argv)
{
    // Translate atom list into OSC message
    char osc_data[4096];
    size_t len=4096;

    // Do nothing if not an OSC message
    if (!s || GetString(s)[0]!='/') {
        if (s)
            post("dimple: %s not an OSC method.", GetString(s));
        else
            post("dimple: No method.");
        return;
    }

    lo_message msg = lo_message_new();
	 int t;
	 for (t=0; t<argc; t++)
	 {
         if (IsSymbol(argv[t]) || IsString(argv[t]))
             lo_message_add_string(msg, GetString(argv[t]));
         else if (IsFloat(argv[t]))
             lo_message_add_float(msg, GetFloat(argv[t]));
         else if (IsInt(argv[t]))
             lo_message_add_int32(msg, GetInt(argv[t]));
	 }
     lo_message_serialise(msg, GetString(s), osc_data, &len);

     // Pass message to DIMPLE (had to HACK liblo internals here)
     lo_message_free(msg);
     ((struct _lo_server*)loserver)->protocol = 0;
     lo_server_dispatch_method(loserver, osc_data, len);

	 return;
}

void flext_dimple_send(lo_address adr, const char *path, const char *types, ...)
{
    t_atom a;
    dimple::SetString(a, path);
    dimple::AtomList lst(1, &a);

    va_list ap;
    va_start(ap, types);
    int n_args = strlen(types), i;
    for (i=0; i<n_args; i++) {
        switch (types[i]) {
        case 'f': dimple::SetFloat(a, (float)va_arg(ap, double));  break;
        case 'i': dimple::SetFloat(a, (float)va_arg(ap, int));     break;
        case 's': dimple::SetString(a, va_arg(ap, char*));         break;
        default:
            continue;
        }
        lst.Append(a);
    }
    va_end(ap);

    send_queue.push(lst);
}

void dimple::m_timer(void*)
{
    while (send_queue.size() > 0)
    {
        AtomList *plst = &send_queue.front();
        if (plst->Count()==1)
            ToOutSymbol(0, dimple::GetSymbol((*plst)[0]));
        else
            ToOutAnything(0, dimple::GetSymbol((*plst)[0]),
                          plst->Count()-1,
                          &(*plst)[1]);
        send_queue.pop();
    }
}

void dimple::m_signal(int n, float *const *in, float *const *out)
{
    int input_chans = CntInSig();
    audioStreamer[0]->writeSamples(in[0], n);
}
