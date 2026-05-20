#include "tst_gitparser.h"
#include <QList>
#include <QFile>
#include <QDir>

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

    QString diffOutput = "@@ -1,3 +1,4 @@\n"
                         " int main() {\n"
                         "-    printf(\"Hello World\");\n"
                         "+    printf(\"Hello, World!\");\n"
                         "+    return 0;\n"
                         "  }\n"
                         "";

    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    QCOMPARE(hunks.size(), 1);

    const Hunk &hunk = hunks.first();
    QCOMPARE(hunk.leftStart, 1);
    QCOMPARE(hunk.rightStart, 1);
    QCOMPARE(hunk.leftSize, 3);
    QCOMPARE(hunk.rightSize, 4);

    QCOMPARE(hunk.lines.size(), 6);

    // Check first line (unchanged)
    QCOMPARE(hunk.lines[0].type, HunkLine::Unchanged);
    QCOMPARE(hunk.lines[0].content, QString("int main() {"));

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

    QString diffOutput = "diff --git a/test.txt b/test.txt\n"
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
                         " Line 12";

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

void TestGitParser::testParseFromFile()
{
    GitParser parser;
    
    // Read sample diff from file directly
    QString filePath = "sample_data/mainwindow.cpp.diff";
    QFile file(filePath);
    
    QVERIFY2(file.exists(), "Sample diff file does not exist");
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), "Could not open sample diff file");
    
    QString diffOutput = file.readAll();
    file.close();
    
    QVERIFY2(!diffOutput.isEmpty(), "Sample diff file is empty");
    
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    // Based on the sample file, we should have 14 hunks
    QCOMPARE(hunks.size(), 14);

    // Check first hunk (line number change only)
    const Hunk &hunk1 = hunks[0];
    QCOMPARE(hunk1.leftStart, 16);
    QCOMPARE(hunk1.rightStart, 16);
    QCOMPARE(hunk1.leftSize, 6);
    QCOMPARE(hunk1.rightSize, 7);
    QCOMPARE(hunk1.lines.size(), 7);

    // Check second hunk (whitespace change)
    const Hunk &hunk2 = hunks[1];
    QCOMPARE(hunk2.leftStart, 23);
    QCOMPARE(hunk2.rightStart, 24);
    QCOMPARE(hunk2.leftSize, 7);
    QCOMPARE(hunk2.rightSize, 6);
    QCOMPARE(hunk2.lines.size(), 7);

    // Check third hunk (added lines)
    const Hunk &hunk3 = hunks[2];
    QCOMPARE(hunk3.leftStart, 31);
    QCOMPARE(hunk3.rightStart, 31);
    QCOMPARE(hunk3.leftSize, 6);
    QCOMPARE(hunk3.rightSize, 7);
    QCOMPARE(hunk3.lines.size(), 7);

    // Check fourth hunk (removed line)
    const Hunk &hunk4 = hunks[3];
    QCOMPARE(hunk4.leftStart, 46);
    QCOMPARE(hunk4.rightStart, 47);
    QCOMPARE(hunk4.leftSize, 6);
    QCOMPARE(hunk4.rightSize, 9);
    QCOMPARE(hunk4.lines.size(), 9);

    // Check fifth hunk (added function)
    const Hunk &hunk5 = hunks[4];
    QCOMPARE(hunk5.leftStart, 104);
    QCOMPARE(hunk5.rightStart, 108);
    QCOMPARE(hunk5.leftSize, 9);
    QCOMPARE(hunk5.rightSize, 12);
    QCOMPARE(hunk5.lines.size(), 13);

    // Check last hunk (added function)
    const Hunk &hunk6 = hunks.constLast();
    QCOMPARE(hunk6.leftStart, 807);
    QCOMPARE(hunk6.rightStart, 606);
    QCOMPARE(hunk6.leftSize, 6);
    QCOMPARE(hunk6.rightSize, 7);
    QCOMPARE(hunk6.lines.size(), 8);
}

void TestGitParser::testParseEmptyDiff()
{
    GitParser parser;    
    QString diffOutput = "";    
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    QCOMPARE(hunks.size(), 0);
}

void TestGitParser::testHunkLineTypes()
{
    GitParser parser;

    QString diffOutput = "@@ -1,3 +1,3 @@\n"
                         " unchanged line\n"
                         "-removed line\n"
                         "+added line\n";

    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    QCOMPARE(hunks.size(), 1);
    const Hunk &hunk = hunks.first();

    QCOMPARE(hunk.lines.size(), 3);
    QCOMPARE(hunk.lines[0].type, HunkLine::Unchanged);
    QCOMPARE(hunk.lines[1].type, HunkLine::Removed);
    QCOMPARE(hunk.lines[2].type, HunkLine::Added);
}

void TestGitParser::testParseDiffWithCaption()
{
    GitParser parser;

    QString diffOutput = "@@ -1,3 +1,4 @@ function_name()\n"
                         " line1\n"
                         "+new line\n"
                         " line2\n"
                         " line3\n";

    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    QCOMPARE(hunks.size(), 1);
    const Hunk &hunk = hunks.first();

    QCOMPARE(hunk.caption, QString("function_name()"));
}

void TestGitParser::testGetTypeString()
{
    // Test static method getTypeString
    QCOMPARE(GitParser::getTypeString(HunkLine::Unchanged), QString(" "));
    QCOMPARE(GitParser::getTypeString(HunkLine::Removed), QString("-"));
    QCOMPARE(GitParser::getTypeString(HunkLine::Added), QString("+"));
}

void TestGitParser::testParseDiffWithNoNewlineIndicator()
{
    GitParser parser;

    QString diffOutput = "@@ -1,2 +1,2 @@\n"
                         " line1\n"
                         "-old line\n"
                         "\\ No newline at end of file\n"
                         "+new line\n";

    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    QCOMPARE(hunks.size(), 1);
    const Hunk &hunk = hunks.first();

    // The "\ No newline" line should be skipped
    QCOMPARE(hunk.lines.size(), 3);
    QCOMPARE(hunk.lines[0].type, HunkLine::Unchanged);
    QCOMPARE(hunk.lines[1].type, HunkLine::Removed);
    QCOMPARE(hunk.lines[2].type, HunkLine::Added);
}

int this_method_make_TestGitParser_visible_in_QT_tests_panel()
{
    return QTest::qExec(new TestGitParser());
}
