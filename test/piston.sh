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
oscsend localhost 7774 /world/prism/create sfff pr -0.2 0 0
oscsend localhost 7774 /world/pr/size fff 0.05 0.05 0.05

oscsend localhost 7774 /world/piston/create sssffffff c1 pr world -0.1 0 0 0 0 1

oscsend localhost 7774 /world/c1/response/spring ff 0.001 0.0001

# Push the prism
oscsend localhost 7774 /world/pr/force fff 0.1 0.01 0.001
sleep 2

# Push the piston
oscsend localhost 7774 /world/c1/force f -0.01
