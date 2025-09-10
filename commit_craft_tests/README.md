# Commit Craft Tests

This directory contains unit tests for the Commit Craft application.

## Test Structure

- `tst_filemodel.cpp` - Tests for the FileModel class
- `tst_commithistorymodel.cpp` - Tests for the CommitHistoryModel class
- `tst_mainwindow.cpp` - Tests for the MainWindow class

## Running Tests

To run the tests, build the project and execute the test binary:

```
cd commit_craft_tests
qmake commit_craft_tests.pro
make
./commit_craft_tests
```

Or if using CMake:

```
cd commit_craft_tests
mkdir build
cd build
cmake ..
make
./commit_craft_tests
```

## Adding New Tests

1. Create a new test file (e.g., `tst_feature.cpp`)
2. Add the file to SOURCES in `commit_craft_tests.pro`
3. Write your test cases using the QtTest framework
4. Rebuild and run the tests