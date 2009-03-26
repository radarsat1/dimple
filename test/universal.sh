#!/bin/sh

# This test file relies on the programs 'oscdump' and 'oscsend' which
# are available as part of the LibLo distribution.  Currently they are
# present in the LibLo svn repository, but not yet part of a stable
# release.

# This script assumes Dimple is already running.

# Listen on port 7775.  We'll assume this is the only oscdump instance
# running, and we don't want to run it if it's already running in
# another terminal.
if ! ( ps -A | grep oscdump >/dev/null 2>&1 ); then (oscdump 7775 &); fi

oscsend localhost 7774 /world/clear
oscsend localhost 7774 /world/prism/create sfff s -0.1 0 0
oscsend localhost 7774 /world/s/size fff 0.07 0.06 0.05

oscsend localhost 7774 /world/universal/create sssfffffffff \
    c1 s world 0 0 0 0 0 1 0 1 0

# Torque it.
oscsend localhost 7774 /world/c1/torque1 f 0.001
sleep 1
oscsend localhost 7774 /world/c1/torque2 f 0.001
sleep 1
oscsend localhost 7774 /world/c1/torque1 f -0.001
sleep 1
oscsend localhost 7774 /world/c1/torque2 f -0.001

