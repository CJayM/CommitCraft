#ifndef TESTGIT_H
#define TESTGIT_H

#include <QtTest>

class TestGit : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testGitCreation();
    void testGitSettersAndGetters();
};

#endif // TESTGIT_H
