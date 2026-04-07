#ifndef TESTINTRALINEDIFF_H
#define TESTINTRALINEDIFF_H

#include <QtTest>
#include "intralinediff.h"

class TestIntraLineDiff : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIdenticalLines();
    void testSimpleChange();
    void testCompleteReplacement();
    void testEmptyStrings();
    void testLongString();
    void cleanupTestCase();
};

#endif // TESTINTRALINEDIFF_H
