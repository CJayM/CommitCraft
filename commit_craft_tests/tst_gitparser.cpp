#include "tst_gitparser.h"
#include <QList>

void TestGitParser::initTestCase() {}

void TestGitParser::cleanupTestCase() {}

void TestGitParser::testGitParserCreation()
{
    GitParser parser;
    QVERIFY(true); // Just test that creation works
}

void TestGitParser::testParseSimpleDiff()
{
    GitParser parser;
    
    QString diffOutput = "@@ -1,3 +1,4 @@\n int main() {\n-    printf(\"Hello World\");\n+    printf(\"Hello, World!\");\n+    return 0;\n }\n";
    
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
    
    QCOMPARE(hunks.size(), 1);
    
    const Hunk &hunk = hunks.first();
    QCOMPARE(hunk.leftStart, 1);
    QCOMPARE(hunk.rightStart, 1);
    QCOMPARE(hunk.leftSize, 3);
    QCOMPARE(hunk.rightSize, 4);
    
    QCOMPARE(hunk.lines.size(), 5);
    
    // Check first line (unchanged)
    QCOMPARE(hunk.lines[0].type, HunkLine::Unchanged);
    QCOMPARE(hunk.lines[0].content, QString(" int main() {"));
    
    // Check second line (removed)
    QCOMPARE(hunk.lines[1].type, HunkLine::Removed);
    QCOMPARE(hunk.lines[1].content, QString("    printf(\"Hello World\");"));
    
    // Check third line (added)
    QCOMPARE(hunk.lines[2].type, HunkLine::Added);
    QCOMPARE(hunk.lines[2].content, QString("    printf(\"Hello, World!\");"));
    
    // Check fourth line (added)
    QCOMPARE(hunk.lines[3].type, HunkLine::Added);
    QCOMPARE(hunk.lines[3].content, QString("    return 0;"));
    
    // Check fifth line (unchanged)
    QCOMPARE(hunk.lines[4].type, HunkLine::Unchanged);
    QCOMPARE(hunk.lines[4].content, QString(" }"));
}

void TestGitParser::testParseComplexDiff()
{
    GitParser parser;
    
    QString diffOutput = 
        "diff --git a/test.txt b/test.txt\n"
        "index 8bc97ab..d0c4b8e 100644\n"
        "--- a/test.txt\n"
        "+++ b/test.txt\n"
        "@@ -1,3 +1,5 @@\n"
        " Line 1\n"
        "-Line 2\n"
        " Line 3\n"
        "+New Line 4\n"
        "+New Line 5\n"
        "@@ -10,3 +12,2 @@\n"
        " Line 10\n"
        "-Line 11\n"
        " Line 12\n";
    
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
    
    QCOMPARE(hunks.size(), 2);
    
    // Check first hunk
    const Hunk &hunk1 = hunks[0];
    QCOMPARE(hunk1.leftStart, 1);
    QCOMPARE(hunk1.rightStart, 1);
    QCOMPARE(hunk1.leftSize, 3);
    QCOMPARE(hunk1.rightSize, 5);
    
    QCOMPARE(hunk1.lines.size(), 5);
    
    // Check second hunk
    const Hunk &hunk2 = hunks[1];
    QCOMPARE(hunk2.leftStart, 10);
    QCOMPARE(hunk2.rightStart, 12);
    QCOMPARE(hunk2.leftSize, 3);
    QCOMPARE(hunk2.rightSize, 2);
    
    QCOMPARE(hunk2.lines.size(), 3);
}

void TestGitParser::testParseEmptyDiff()
{
    GitParser parser;
    
    QString diffOutput = "";
    
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
    
    QCOMPARE(hunks.size(), 0);
}