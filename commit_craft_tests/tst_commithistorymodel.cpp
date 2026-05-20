#include "tst_commithistorymodel.h"

void TestCommitHistoryModel::initTestCase() {}

void TestCommitHistoryModel::cleanupTestCase() {}

void TestCommitHistoryModel::testCommitHistoryModelCreation()
{
    CommitHistoryModel model;
    QCOMPARE(model.rowCount(), 0);
}

void TestCommitHistoryModel::testCommitHistoryModelWithCommits()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    
    QList<QString> commit2;
    commit2 << "def456ghi" << "Jane Smith" << "2023-01-02" << "Add feature";
    commits.append(commit2);
    
    model.setCommits(commits);
    
    QCOMPARE(model.rowCount(), 2);
    
    QCOMPARE(model.getCommitHash(0), QString("abc123def"));
    QCOMPARE(model.getCommitHash(1), QString("def456ghi"));
}

void TestCommitHistoryModel::testCommitHistoryModelEmpty()
{
    CommitHistoryModel model;
    
    QCOMPARE(model.rowCount(), 0);
    
    // Test with invalid index
    QVERIFY(model.data(QModelIndex()).isNull());
    QVERIFY(model.getCommitHash(-1).isEmpty());
    QVERIFY(model.getCommitHash(100).isEmpty());
}

void TestCommitHistoryModel::testCommitHistoryModelDataDisplayRole()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    
    model.setCommits(commits);
    
    QModelIndex index = model.index(0, 0);
    QVariant displayData = model.data(index, Qt::DisplayRole);
    
    QVERIFY(displayData.isValid());
    QString displayString = displayData.toString();
    QVERIFY(displayString.contains("2023-01-01"));
    QVERIFY(displayString.contains("Initial commit"));
    QVERIFY(displayString.contains("John Doe"));
}

void TestCommitHistoryModel::testCommitHistoryModelDataUserRole()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    
    model.setCommits(commits);
    
    QModelIndex index = model.index(0, 0);
    QVariant userData = model.data(index, Qt::UserRole);
    
    QVERIFY(userData.isValid());
    QVERIFY(userData.canConvert<QList<QString>>());
    
    QList<QString> commitData = userData.value<QList<QString>>();
    QCOMPARE(commitData.size(), 4);
    QCOMPARE(commitData[0], QString("abc123def"));
    QCOMPARE(commitData[1], QString("John Doe"));
    QCOMPARE(commitData[2], QString("2023-01-01"));
    QCOMPARE(commitData[3], QString("Initial commit"));
}

void TestCommitHistoryModel::testCommitHistoryModelDataToolTipRole()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    
    model.setCommits(commits);
    
    QModelIndex index = model.index(0, 0);
    QVariant toolTipData = model.data(index, Qt::ToolTipRole);
    
    QVERIFY(toolTipData.isValid());
    QString toolTipString = toolTipData.toString();
    QVERIFY(toolTipString.contains("Hash: abc123def"));
    QVERIFY(toolTipString.contains("Author: John Doe"));
    QVERIFY(toolTipString.contains("Date: 2023-01-01"));
    QVERIFY(toolTipString.contains("Message: Initial commit"));
}

void TestCommitHistoryModel::testCommitHistoryModelInvalidIndex()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe" << "2023-01-01" << "Initial commit";
    commits.append(commit1);
    
    model.setCommits(commits);
    
    // Test with out-of-bounds index
    QVERIFY(model.data(model.index(100, 0)).isNull());
    QVERIFY(model.data(model.index(0, 1)).isNull()); // Only 1 column
    
    // Test with parent index (should return 0 rows)
    QModelIndex parent = model.index(0, 0);
    QCOMPARE(model.rowCount(parent), 0);
}

void TestCommitHistoryModel::testCommitHistoryModelPartialCommitData()
{
    CommitHistoryModel model;
    QList<QList<QString>> commits;
    
    // Test with incomplete commit data (less than 4 fields)
    QList<QString> commit1;
    commit1 << "abc123def" << "John Doe"; // Only hash and author
    commits.append(commit1);
    
    // Test with empty commit
    QList<QString> commit2;
    commits.append(commit2);
    
    model.setCommits(commits);
    
    QCOMPARE(model.rowCount(), 2);
    
    // Should still return hash for partial data
    QCOMPARE(model.getCommitHash(0), QString("abc123def"));
    
    // Empty commit should return empty hash
    QVERIFY(model.getCommitHash(1).isEmpty());
}

int this_method_make_TestCommitHistoryModel_visible_in_QT_tests_panel()
{
    return QTest::qExec(new TestCommitHistoryModel());
}
