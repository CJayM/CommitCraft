#include <QtTest/QtTest>
#include "tst_commithistorymodel.h"
#include "tst_filemodel.h"
#include "tst_git.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    int status = 0;

    auto RUN_TEST = [&status, argc, argv](QObject *tst) {
        status |= QTest::qExec(tst, argc, argv);
        delete tst;
    };

    RUN_TEST(new TestCommitHistoryModel());
    RUN_TEST(new TestFileModel());
    RUN_TEST(new TestGit());

    return status;
}
