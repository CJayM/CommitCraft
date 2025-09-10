#!/bin/bash
# Test runner script for Commit Craft

echo "Building and running Commit Craft tests..."

# Create build directory
mkdir -p build
cd build

# Run qmake
qmake ../commit_craft_tests.pro

# Build the tests
make

# Run the tests
./commit_craft_tests

cd ..