#include "tst_git.h"

#include "../commit_craft_lib/git.h"

void TestGit::initTestCase() {}

void TestGit::cleanupTestCase() {}

void TestGit::testGitCreation()
{
    Git git{this};
    QVERIFY(!git.repositoryPath().isEmpty());
    QCOMPARE(git.gitPath(), QString("git"));
}

void TestGit::testGitSettersAndGetters()
{
    Git git{this};

    git.setRepositoryPath("/test/path");
    QCOMPARE(git.repositoryPath(), QString("/test/path"));
    
    git.setGitPath("/usr/bin/git");
    QCOMPARE(git.gitPath(), QString("/usr/bin/git"));
}
