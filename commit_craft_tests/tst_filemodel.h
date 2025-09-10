#ifndef TESTFILEMODEL_H
#define TESTFILEMODEL_H

#include "../commit_craft/filemodel.h"

#include <QtTest>

class TestFileModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testFileModelCreation();
    void testFileModelWithFiles();
};

#endif // TESTFILEMODEL_H
