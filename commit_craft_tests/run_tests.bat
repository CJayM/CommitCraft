@echo off
REM Test runner script for Commit Craft on Windows

echo Building and running Commit Craft tests...

REM Create build directory
if not exist build mkdir build
cd build

REM Run qmake
qmake ../commit_craft_tests.pro

REM Build the tests
mingw32-make

REM Run the tests
commit_craft_tests.exe

cd ..