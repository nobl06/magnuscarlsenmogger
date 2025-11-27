#!/bin/bash
###WRITE IN TERMINAL IF DOESN'T WORK
#chmod +x run.sh


#!/bin/bash

mkdir -p build
cd build
cmake ..  # only needed first time
make -s
cd ..

./build/MagnusCarlsenMogger -H test/moves_test.txt -m build/out.txt

