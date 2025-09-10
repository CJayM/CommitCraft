#include <QApplication>
#include <QtTest>

#include "tst_mainwindow.h"

void TestMainWindow::initTestCase()
{
    // This function is called before any test function is executed
}

void TestMainWindow::cleanupTestCase()
{
    // This function is called after all test functions have been executed
}

void TestMainWindow::testWindowCreation()
{
    MainWindow window;
    QVERIFY(window.windowTitle().contains("Commit Craft"));
}
