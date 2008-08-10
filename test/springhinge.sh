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
oscsend localhost 7774 /world/prism/create sfff box 0 0 0.1
oscsend localhost 7774 /world/box/size fff 0.02 0.1 0.20
oscsend localhost 7774 /world/box/color fff 1 0.2 0.3
oscsend localhost 7774 /world/hinge/create sssffffff boxhinge box world \
    0 0 0 0 1 0
oscsend localhost 7774 /world/boxhinge/response/spring ff 0.0000001 0.00000001

# Give it some torque
oscsend localhost 7774 /world/boxhinge/torque f 0.00001

