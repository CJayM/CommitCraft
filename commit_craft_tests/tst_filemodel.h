#ifndef TESTFILEMODEL_H
#define TESTFILEMODEL_H

#include <QtTest>

class TestFileModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testFileModelCreation();
    void testFileModelWithFiles();
    void testFileModelEmpty();
    void testFileModelWithPaths();
    void testFileModelStatusColors();
    void testFileModelHeaderData();
    void testFileModelTextAlignment();
    void testFileModelDirectoryColor();
    void testFileModelInvalidIndex();
};

#endif // TESTFILEMODEL_H
