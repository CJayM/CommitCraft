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
    QCOMPARE(model.columnCount(), 3); // Updated to match actual column count
    
    // Check status symbol (not the raw status letter)
    QVERIFY(!model.data(model.index(0, 0)).toString().isEmpty());
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("main.cpp"));
    QVERIFY(!model.data(model.index(1, 0)).toString().isEmpty());
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("newfile.txt"));
    
    QCOMPARE(model.getFileName(0), QString("main.cpp"));
    QCOMPARE(model.getFileStatus(0), QString("M"));
    QCOMPARE(model.getFileName(1), QString("newfile.txt"));
    QCOMPARE(model.getFileStatus(1), QString("A"));
}

void TestFileModel::testFileModelEmpty()
{
    FileModel model;
    
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 3);
    
    // Test with invalid index
    QVERIFY(model.data(QModelIndex()).isNull());
    QVERIFY(model.getFileName(-1).isEmpty());
    QVERIFY(model.getFileStatus(-1).isEmpty());
    QVERIFY(model.getRelativePath(-1).isEmpty());
}

void TestFileModel::testFileModelWithPaths()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    files.append(qMakePair(QString("M"), QString("src/main.cpp")));
    files.append(qMakePair(QString("A"), QString("include/header.h")));
    files.append(qMakePair(QString("D"), QString("README.md"))); // File in root
    model.setFiles(files);
    
    QCOMPARE(model.rowCount(), 3);
    
    // Check file names extracted from paths
    QCOMPARE(model.data(model.index(0, 1)).toString(), QString("main.cpp"));
    QCOMPARE(model.data(model.index(0, 2)).toString(), QString("src"));
    
    QCOMPARE(model.data(model.index(1, 1)).toString(), QString("header.h"));
    QCOMPARE(model.data(model.index(1, 2)).toString(), QString("include"));
    
    // File in root should have empty directory
    QCOMPARE(model.data(model.index(2, 1)).toString(), QString("README.md"));
    QCOMPARE(model.data(model.index(2, 2)).toString(), QString(""));
}

void TestFileModel::testFileModelStatusColors()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    
    // Test all status types
    files.append(qMakePair(QString("M"), QString("modified.cpp")));
    files.append(qMakePair(QString("A"), QString("added.cpp")));
    files.append(qMakePair(QString("D"), QString("deleted.cpp")));
    files.append(qMakePair(QString("R"), QString("renamed.cpp")));
    files.append(qMakePair(QString("C"), QString("copied.cpp")));
    files.append(qMakePair(QString("U"), QString("unmerged.cpp")));
    files.append(qMakePair(QString("T"), QString("typechange.cpp")));
    files.append(qMakePair(QString("?"), QString("untracked.cpp")));
    
    model.setFiles(files);
    
    // Verify all rows have background colors
    for (int i = 0; i < files.size(); ++i) {
        QVariant bgColor = model.data(model.index(i, 0), Qt::BackgroundRole);
        QVERIFY(bgColor.isValid());
        QVERIFY(bgColor.canConvert<QColor>());
    }
}

void TestFileModel::testFileModelHeaderData()
{
    FileModel model;
    
    // Test horizontal header
    QVariant header0 = model.headerData(0, Qt::Horizontal, Qt::DisplayRole);
    QVariant header1 = model.headerData(1, Qt::Horizontal, Qt::DisplayRole);
    QVariant header2 = model.headerData(2, Qt::Horizontal, Qt::DisplayRole);
    
    QCOMPARE(header0.toString(), QString("")); // Empty for status column
    QCOMPARE(header1.toString(), QString("Файл"));
    QCOMPARE(header2.toString(), QString("Директория"));
    
    // Test vertical header (should return invalid)
    QVERIFY(model.headerData(0, Qt::Vertical, Qt::DisplayRole).isNull());
}

void TestFileModel::testFileModelTextAlignment()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    files.append(qMakePair(QString("M"), QString("test.cpp")));
    model.setFiles(files);
    
    // Status column should be centered
    QVariant alignment0 = model.data(model.index(0, 0), Qt::TextAlignmentRole);
    QCOMPARE(alignment0.toInt(), (int)Qt::AlignCenter);
    
    // Filename column should be left-aligned
    QVariant alignment1 = model.data(model.index(0, 1), Qt::TextAlignmentRole);
    QCOMPARE(alignment1.toInt(), (int)Qt::AlignLeft);
    
    // Directory column should be left-aligned
    QVariant alignment2 = model.data(model.index(0, 2), Qt::TextAlignmentRole);
    QCOMPARE(alignment2.toInt(), (int)Qt::AlignLeft);
}

void TestFileModel::testFileModelDirectoryColor()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    files.append(qMakePair(QString("M"), QString("src/test.cpp")));
    model.setFiles(files);
    
    // Directory column should have gray color
    QVariant dirColor = model.data(model.index(0, 2), Qt::ForegroundRole);
    QVERIFY(dirColor.isValid());
    QVERIFY(dirColor.canConvert<QColor>());
    QColor color = dirColor.value<QColor>();
    QCOMPARE(color.red(), 128);
    QCOMPARE(color.green(), 128);
    QCOMPARE(color.blue(), 128);
}

void TestFileModel::testFileModelInvalidIndex()
{
    FileModel model;
    QList<QPair<QString, QString>> files;
    files.append(qMakePair(QString("M"), QString("test.cpp")));
    model.setFiles(files);
    
    // Test with out-of-bounds index
    QVERIFY(model.data(model.index(5, 0)).isNull());
    QVERIFY(model.getFileName(5).isEmpty());
    QVERIFY(model.getFileStatus(5).isEmpty());
    QVERIFY(model.getRelativePath(5).isEmpty());
    
    // Test with parent index (should return 0 rows)
    QModelIndex parent = model.index(0, 0);
    QCOMPARE(model.rowCount(parent), 0);
}

int this_method_make_TestFileModel_visible_in_QT_tests_panel()
{
    return QTest::qExec(new TestFileModel());
}
