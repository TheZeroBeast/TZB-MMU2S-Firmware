#! /bin/bash

# format C++ code with artistic style V3.1
# please download latest version from 
# https://launchpad.net/ubuntu/cosmic/+package/astyle

astyle --suffix=none --project=.astylerc *.cpp,*.h,*.c

