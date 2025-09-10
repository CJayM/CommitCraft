#include "tst_commithistorymodel.h"

void TestCommitHistoryModel::initTestCase()
{
    // This function is called before any test function is executed
}

void TestCommitHistoryModel::cleanupTestCase()
{
    // This function is called after all test functions have been executed
}

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

