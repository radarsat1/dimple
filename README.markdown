
DIMPLE: The Dynamic Interactive Musically PhysicaL Environment
==============================================================

![DIMPLE logo](./icon/dimple_sphere.png)

DIMPLE is a program which allows you to construct physical
environments using Open Sound Control messages.  This means that you
create 3D objects and allow them to interact with each other,
colliding or otherwise connected to each other, and then retrieve
information about them, such as position, velocity, and acceleration.
This data can then be used as the basis for sound synthesis, for
example, or anything else you can think of!

The physical environment can be touched and manipulated using a
force-feedback haptic device, meaning that you can use this to set up
experiments for haptics research, or to create touchable interfaces
for music.

It can be used with any software that supports [Open Sound
Control](http://opensoundcontrol.org).  This includes [_Pure
Data_](http://puredata.info), as well as **Cycling 74**'s
[_Max/MSP_](http://cycling74.com), or even the
[_ChucK_](http://chuck.cs.princeton.edu/) or
[_SuperCollider_](http://supercollider.sourceforge.net/) audio
languages.

This beta release is very young, so if you find repeatable bugs please
report them to me.  This software is cross-platform (Linux, OS X,
Windows), and currently supports the following haptic devices:

- SensAble Phantom (Windows)
- MPB Technologies Freedom 6S (Windows, Linux)
- Novint Falcon (Windows, OS X, Linux)


Using
-----

This will be a brief overview of messages that DIMPLE can receive
and send.  Better documentation will be included in a later version.

To run DIMPLE after it has been compiled, simply execute it, like so:

    ./dimple

On Windows and OS X, double-click dimple.exe or dimple.app,
respectively.

You will be greated by a short message, and see a blank OpenGL window
appear with a small yellow sphere.  This sphere represents the haptic
device cursor location.  The program is simply waiting for OSC
messages on port 7774.  If at any time it is instructed to send
messages, it will also send them to localhost, on port 7775.  (This
will not be hardcoded in future versions.)

If you have installed LibLo, you will have two command-line utilities
named "oscsend" and "oscdump".  These are used from the scripts in the
"test" folder, so try running some of these to see different features
of DIMPLE in action.  Additionally, you can try the small examples
that are included in the Pure Data patch, "test/test.pd".

Run Pure Data and load the test.pd file.  You'll need at least version
0.40-1, as well as the following externals from the extended build:
`sendOSC`, `dumpOSC`, `OSCroute`, and `makesymbol`.

Clicking on the bang in front of 'Initialize' will open the OSC
connections to DIMPLE.  Then, try some of the other bangs for some
examples of scenes you can create.

Included demos:

* 'Force Stick', a single prism which is constrained by a hinge.
  Pushing on it will modulate the frequency of a sinusoid in Pd.

* 'Marble Box', a box which you can fill with marbles.  First click
  the bang to create the box, then give the world some gravity
  and create some marbles by moving the designated number box.
  You can move the marbles around with the haptic device, and
  when they collide, Pd plays a decaying sinusoid.

* 'Smash', a small test for the collision routines, causes three spheres
  to fly at each other and hit.

* 'Snake', a series of connected prisms.  Their velocity controls a
  set of FM synthesizers.  Pushing on one causes oscillations in the
  connected prisms.  This example demonstrates continuous control of
  audio parameters.


OSC Methods
-----------

We can create sphere and prism objects:

    /world/sphere/create <name> <x> <y> <z>
    /world/prism/create <name> <x> <y> <z>

You can then change the object's parameters, such as size and mass, or
give it some force, for example:

    /world/<name>/mass <mass>
    /world/<name>/radius <radius>                   (for a sphere)
    /world/<name>/size <width> <height> <depth>     (for a prism)

    /world/<name>/force <x> <y> <z>                 (any object)

Any object parameter can be requested by the "/get" suffix:

    /world/<name>/velocity/get <interval>

DIMPLE will respond by posting the following message to OSC port 7775
on "localhost":

    /world/<name>/velocity <x> <y> <z>

Note that the <interval> parameter is optional.  If not specified, the
parameter's value is posted only once.  If an interval is specified
(in milliseconds), that parameter is posted continuously.  An interval
of zero stops the parameter from being posted.

More information can be found in the "messages.txt" documentation
file.


Compiling
---------

First, execute "bootstrap.sh", like so:

    ./bootstrap.sh

On Windows, DIMPLE now requires the *MSYS2* environment.  This can
be installed most easily by visiting the [web site](https://www.msys2.org).

Make sure to install the C++ compiler (g++), and the OpenGL API.
`curl` or `wget` is also needed for the bootstrap to work.  The
bootstrap script downloads & compiles libraries upon which DIMPLE
depends.  It also patches them in certain ways when it is necessary.

If boostrap is successful, continue with compiling DIMPLE.  If you run
into errors, you'll have to manually make sure the dependencies
compile correctly.  Please inform me of any problems, as I'm
interested in making the bootstrap as easy as possible.  Note that the
bootstrap compiles only the static version of these libraries, so that
all required code gets linked right into DIMPLE.

Configure the DIMPLE build with configure:

    ./configure

Compile DIMPLE with make:

    make

If all goes well, you should now have an executable called "dimple",
which you can execute like so:

    ./dimple


Questions
---------

This release will hopefully be somewhat more resiliant than previous
ones, but notice that this software is only just approach "beta"
status so you are still likely to find bugs.  If you have questions or
comments, try me at
[sinclair@music.mcgill.ca](mailto:sinclair@music.mcgill.ca), or visit
the [DIMPLE website](http://idmil.org/software/dimple).
