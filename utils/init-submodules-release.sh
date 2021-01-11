#!/bin/bash

if git submodule status 2>&1 | grep -iq "fatal: Not a git repository (or any of the parent directories): .git"; then
cd "$(dirname "$0")"/../external/ || exit
git clone git@github.com:google/googletest.git
cd googletest/ || exit
git checkout release-1.8.0
cd - || exit
git clone git@github.com:nlohmann/json.git
cd json/ || exit
git checkout v3.4.0
cd - || exit
git clone https://github.com/pdschubert/WALi-OpenNWA.git
fi
