#include "tst_commithistorymodel.h"
#include "tst_filemodel.h"

#include <QtTest/QtTest>

int main(int argc, char* argv[])
{
    int result = 0;

    auto RUN_TEST = [&result, argc, argv](QObject* tst) {
        result |= QTest::qExec(tst, argc, argv);
        delete tst;
    };

    RUN_TEST(new TestCommitHistoryModel());
    RUN_TEST(new TestFileModel());

    return result;
}
