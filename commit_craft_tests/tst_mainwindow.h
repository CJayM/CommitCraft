#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include "../commit_craft/mainwindow.h"

#include <QtTest>

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testWindowCreation();
};

#endif // TESTMAINWINDOW_H
