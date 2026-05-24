#ifndef TESTCOMMITITEMDELEGATE_H
#define TESTCOMMITITEMDELEGATE_H

#include <QtTest>

class TestCommitItemDelegate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testDelegateCreation();
    void testSizeHint();
    void testPaintWithValidCommitData();
    void testPaintWithInvalidCommitData();
    void testPaintWithEmptyCommitData();
    void testPaintWithPartialCommitData();
};

#endif // TESTCOMMITITEMDELEGATE_H
