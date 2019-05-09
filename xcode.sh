#!/bin/bash

if [ -d xcode ]; then
    rm -rf xcode 
fi

mkdir xcode && 
cd xcode && 
cmake -G Xcode -DCMAKE_INSTALL_PREFIX=~/Library/Frameworks .. &&
xcodebuild -alltargets
