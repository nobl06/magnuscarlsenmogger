#!/bin/bash
###WRITE IN TERMINAL IF DOESN'T WORK
#chmod +x run.sh


# Automatically builds then runs your program with test inputs
echo "🔨 Building..."
cd build || exit
make -s
cd ..

echo "🚀 Running MagnusCarlsenMogger..."
./build/MagnusCarlsenMogger -H test/moves_test.txt -m build/out.txt

echo "📄 Output:"
cat build/out.txt
