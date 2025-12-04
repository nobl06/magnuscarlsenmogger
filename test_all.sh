#!/bin/bash

# Build the project first
cd build && make && cd ..

echo "=== Test 1: Starting Position ==="
./build/MagnusCarlsenMogger -H test_start.txt -m output1.txt
echo ""

echo "=== Test 2: After e4 ==="
./build/MagnusCarlsenMogger -H test_e4.txt -m output2.txt
echo ""

echo "=== Test 3: Italian Game ==="
./build/MagnusCarlsenMogger -H test_italian.txt -m output3.txt
echo ""

echo "=== Outputs ==="
echo "Output 1: $(cat output1.txt)"
echo "Output 2: $(cat output2.txt)"
echo "Output 3: $(cat output3.txt)"

