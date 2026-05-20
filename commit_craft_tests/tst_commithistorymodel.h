#ifndef TESTCOMMITHISTORYMODEL_H
#define TESTCOMMITHISTORYMODEL_H

#include <QtTest>

#include "../commit_craft_lib/commithistorymodel.h"

class TestCommitHistoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCommitHistoryModelCreation();
    void testCommitHistoryModelWithCommits();
    void testCommitHistoryModelEmpty();
    void testCommitHistoryModelDataDisplayRole();
    void testCommitHistoryModelDataUserRole();
    void testCommitHistoryModelDataToolTipRole();
    void testCommitHistoryModelInvalidIndex();
    void testCommitHistoryModelPartialCommitData();
};

#endif // TESTCOMMITHISTORYMODEL_H
