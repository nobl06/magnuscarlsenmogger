#!/bin/bash

echo "=== Testing different first moves ===\n"

for move in "e2e4" "e2e3" "d2d4" "g1f3" "b1c3"; do
    echo "\n=== After $move ===" 
    echo "$move" > temp_test.txt
    ./build/MagnusCarlsenMogger -H temp_test.txt -m temp_out.txt | grep -E "Evaluation|Best move"
done

rm temp_test.txt temp_out.txt

