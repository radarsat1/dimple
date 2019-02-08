#!/bin/sh

# This test file relies on the programs 'oscdump' and 'oscsend' which
# are available as part of the LibLo distribution.  Currently they are
# present in the LibLo svn repository, but not yet part of a stable
# release.

# This script assumes Dimple is already running.

# Disable path mangling in MSYS2
export MSYS2_ARG_CONV_EXCL="/world"

# Listen on port 7778.  We'll assume this is the only oscdump instance
# running, and we don't want to run it if it's already running in
# another terminal.
if ! ((ps -A 2>/dev/null || ps -W 2>/dev/null || ps aux 2>/dev/null) | grep oscdump >/dev/null 2>&1 ); then (oscdump 7778 &); fi

oscsend localhost 7774 /world/clear
oscsend localhost 7774 /world/sphere/create sfff s1 -0.1 0 0
oscsend localhost 7774 /world/s1/radius f 0.03
oscsend localhost 7774 /world/sphere/create sfff s2 -0.3 0 0
oscsend localhost 7774 /world/s2/radius f 0.03
oscsend localhost 7774 /world/s2/color fff 0.2 0.7 0.3
oscsend localhost 7774 /world/hinge/create sssffffff c1 s1 s2 -0.2 0 0 0 1 0

# Destroy the constraint.
#oscsend localhost 7774 /world/c1/destroy

# Push the first one.
oscsend localhost 7774 /world/s2/force fff 0 0 0.01

# Destroy the object, which should destroy the constraint.
oscsend localhost 7774 /world/s1/destroy
oscsend localhost 7774 /world/s2/destroy

# Destroy the constraint
#oscsend localhost 7774 /world/c1/destroy

