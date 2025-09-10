#ifndef TESTCOMMITHISTORYMODEL_H
#define TESTCOMMITHISTORYMODEL_H

#include <QtTest>

#include "../commit_craft/commithistorymodel.h"

class TestCommitHistoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCommitHistoryModelCreation();
    void testCommitHistoryModelWithCommits();
};


#endif // TESTCOMMITHISTORYMODEL_H
