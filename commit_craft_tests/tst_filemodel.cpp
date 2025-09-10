#include "tst_filemodel.h"

#include "../commit_craft_lib/filemodel.h"

void TestFileModel::initTestCase() {}

void TestFileModel::cleanupTestCase() {}

void TestFileModel::testFileModelCreation()
{
    FileModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 2);
}

void TestFileModel::testFileModelWithFiles()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    files.append(qMakePair(QString("M"), QString("main.cpp")));
    files.append(qMakePair(QString("A"), QString("newfile.txt")));
    model.setFiles(files);
    
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.columnCount(), 2);
    
    QCOMPARE(model.data(model.index(0, 0)).toString(), QString("M"));
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("main.cpp"));
    QCOMPARE(model.data(model.index(1, 0)).toString(), QString("A"));
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("newfile.txt"));
    
    QCOMPARE(model.getFileName(0), QString("main.cpp"));
    QCOMPARE(model.getFileStatus(0), QString("M"));
    QCOMPARE(model.getFileName(1), QString("newfile.txt"));
    QCOMPARE(model.getFileStatus(1), QString("A"));
}

int this_method_make_TestFileModel_visible_in_QT_tests_panel()
{
    return QTest::qExec(new TestFileModel());
}
