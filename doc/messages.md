# DIMPLE: Documentation

This documentation describes the messages received and sent by DIMPLE.
The following syntax is used:

  * Square brackets ''[]'' for optional parameters
  * Triangle brackets ''<>'' for required parameters
  * Parameters are prefixed with a type indicator:
    * i: 32-bit integer
    * f: 32-bit floating point
    * s: string
  * DIMPLE coerces types, so integers can be specified for float parameters

## Values ##

Any value specified by an OSC address can be **set** by providing a
parameter directly, or **retrieved** by appending the suffix ''/get''.
The ''/get'' suffix with no parameters will return the value exactly
once.  An optional integer parameter to ''/get'' specifies that the
value should be returned at regular intervals the given number of
milliseconds apart.  (Currently 10 ms is the lowest interval that can
be specified.)  These timed messages can be cancelled by specifying
a parameter of 0 to ''/get''.

With only a few exceptions, values can be either //scalars// or
//vectors//.  Vectors can be identified by exactly three
floating-point parameters.  Vectors can also be referenced by
appending the ''/magnitude'' suffix which will set the magnitude of
the vector, leaving its current direction the same.  ''/get'' can also
be appended to ''/magnitude''.

There are currently some exceptions to these rules, which are detailed
below.

## Connecting to DIMPLE ##

DIMPLE communicates exclusively using
[Open Sound Control](http://opensoundcontrol.org/) over UDP, using
port 7774 for receiving and port 7775 for sending.  In other words,
using 7774 as the destination port in your application, and 7775 as
the receiving port.  DIMPLE is hard-coded to send to localhost, so
your application must be running on the same computer.  (This will
be configurable in the future.)

## Messages ##

### Creating objects ###

    /world/prism/create <s:name> [f:x] [f:y] [f:z]
    /world/sphere/create <s:name> [f:x] [f:y] [f:z]
    /world/mesh/create <s:name> <s:filename.3ds> [f:x] [f:y] [f:z]

These messages create a named object as either a prism, sphere, or
arbitrary mesh, which must be specified in a format supported by CHAI
3D, currently this includes .3ds, .obj, and .stl file formats.
Optionally, an initial position can be specified.  The initial size
will be quite small, so these messages are usually followed up by a
''/size'' or ''/radius'' message.

### Creating constraints ###

    /world/fixed/create <s:name> <s:object1> <s:object2>
    /world/free/create <s:name> <s:object1> <s:object2>
    /world/ball/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz>
    /world/hinge/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz> <f:axisx> <f:axisy> <f:axisz>
    /world/hinge2/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz> <f:axis1x> <f:axis1y> <f:axis1z> <f:axis2x> <f:axis2y> <f:axis2z>
    /world/universal/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz> <f:axis1x> <f:axis1y> <f:axis1z> <f:axis2x> <f:axis2y> <f:axis2z>
    /world/slide/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz>
    /world/piston/create <s:name> <s:object1> <s:object2> <f:anchorx> <f:anchory> <f:anchorz>

Constraints can be created between two objects, or between an object
and the **world** coordinate system.  For the latter, specify
//object2// as ''world''.  Different constraints take different
numbers of arguments depending on how many points and vectors are
needed to define them.  Please see the
[ODE documentation section on Joints](http://ode-wiki.org/wiki/index.php?title=Manual:_Joint_Types_and_Functions)
for more information on each constraint type.

The "free" constraint is not really a constraint --- all the axes are
free.  However, it is useful for establishing a response between the
two specified objects.  It is not possible to specify a free
constraint between an object and the world, as this already exists by
default.

### Object values ###

    /world/<name>/position <f:x> <f:y> <f:z>
    /world/<name>/velocity <f:x> <f:y> <f:z>
    /world/<name>/acceleration <f:x> <f:y> <f:z>
    /world/<name>/rotation <f:r11> <f:r12> <f:r13> <f:r21> <f:r22> <f:r23> <f:r31> <f:r32> <f:r33>
    /world/<name>/force <f:x> <f:y> <f:z>
    /world/<name>/mass <f:mass>
    /world/<name>/density <f:density>
    /world/<name>/color <f:r> <f:g> <f:b>
    /world/<name>/friction/static <f:coefficient>
    /world/<name>/friction/dynamic <f:coefficient>

Note that ''/rotation'' takes a 3x3 rotation matrix as argument, in
the form of 9 floating-point values.  See, for example,
[Wikipedia](http://en.wikipedia.org/wiki/Rotation_matrix), on how to
calculate this.

An object's initial density is 100, but be aware that resizing objects
preserves the density, and therefore changes their mass accordingly.
The object's mass has an important effect on haptic interaction as
well the magnitude of force and spring stiffness values to use.  (The
value 100 has been selected to feel "good" for haptic interaction, but
this should be tuned according to the world you are designing.)

#### Values for prisms and meshes ####

    /world/<name>/size <f:width> <f:depth> <f:height>

#### Values for spheres ####

    /world/<name>/radius <f:radius>

### Other object messages ###

    /world/<name>/collide <i:0,1>

A parameter of 1 indicates that collisions for this object are
requested.  0 indicates not to report collisions for this object.

    /world/<name>/collide <s:object> <f:velocity>

This is the form of the response generated by DIMPLE when a collision
occurs.

    /world/<name>/grab

This message with no parameter indicates that this object should be
"grabbed", i.e., should be attached by virtual coupling to the
location of the haptic proxy.  Grabbing another object will cause
the current grabbed object to be dropped.

    /world/<name>/visible <i:0,1>

Controls the visibility of this object in the visual display.  1 means
the object is visible, and 0 makes the object not visible.

    /world/<name>/destroy

Destroys this object.


### Constraint responses ###

    /world/<name>/response/spring <f:stiffness> <f:damping>

Springs are allowed for the following constraints:

  * Hinge
  * Hinge2
  * Universal
  * Slide
  * Piston

Stiffness and damping coefficients can be specified for this spring,
making it return to its original orientation or position.

For rotational constraints, the torque can be accessed by the
following:

    /world/<name>/response/torque <f:magnitude>

Some constraints have two free axes (e.g., universal and hinge2),
and these must be refered to with a numerical suffix:

    /world/<name>/response/torque1 <f:magnitude>
    /world/<name>/response/torque2 <f:magnitude>

For linear constraints (e.g., slide), the force can be accessed by:

    /world/<name>/response/force <f:magnitude>

### Global messages ###

    /world/collide <i:0,1>

This message with a parameter of 1 specifies that collisions between
//any// two objects should be reported.  A parameter of 0 disables
reporting of collisions between any two objects, but specific objects
can still be enabled for collision reporting.

    /world/collide <s:object1> <s:object2> <f:velocity>

This is the form of the response generated by DIMPLE when a collision
occurs.

    /world/gravity <f:x> <f:y> <f:z>

Sets the world's gravity vector to a given direction and magnitude.

    /world/drop

Drops a grabbed object.

    /world/clear

Clears all objects in the world.

### Special objects ###

There are a couple of predefined special objects in the DIMPLE world.
These are used to control the camera and get information about the
haptic cursor.

#### Camera ####

The camera can be addressed by the prefix,

    /world/camera

It has the following parameters:

    /world/camera/position <f:x> <f:y> <f:z>
    /world/camera/lookat <f:x> <f:y> <f:z>
    /world/camera/up <f:x> <f:y> <f:z>

#### Cursor ####

The cursor can be addressed by the prefix,

    /world/cursor

Note that currently it is necessary to address this particular object
on UDP port 7772 (the haptic thread's port) to get meaningful
information.

The cursor has most of the same parameters as other objects in DIMPLE.
