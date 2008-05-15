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
oscsend localhost 7774 /world/prism/create sfff cube 0 0 0
oscsend localhost 7774 /world/cube/size fff 0.15 0.15 0.15
oscsend localhost 7774 /world/cube/color fff 1 0.2 0.3

# Push it slowly to the left
oscsend localhost 7774 /world/cube/force fff -0.000001 0 0
