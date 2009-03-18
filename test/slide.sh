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
oscsend localhost 7774 /world/prism/create sfff p -0.1 0 0
oscsend localhost 7774 /world/p/size fff 0.03 0.03 0.1

oscsend localhost 7774 /world/slide/create sssfff c1 p world 0 0 1
oscsend localhost 7774 /world/c1/response/spring ff 0.001 0.0001

# Push the prism
oscsend localhost 7774 /world/p/force fff 0.01 0.005 0.002
