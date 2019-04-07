#!/bin/bash

set -e
set -x

# HUGO=/snap/hugo/current/bin/hugo
HUGO=hugo

git submodule init
git submodule update

# Prepare hugo site with docs
git clone `git remote get-url origin` -b master dimple
cp -v dimple/doc/messages.md content/

# Empty pages, run hugo, replace contents with static site
$HUGO -d pages
