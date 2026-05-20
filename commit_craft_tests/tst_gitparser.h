#ifndef TESTGITPARSER_H
#define TESTGITPARSER_H

#include "../commit_craft_lib/gitparser.h"

#include <QtTest>

class TestGitParser : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testGitParserCreation();
    void testParseSimpleDiff();
    void testParseComplexDiff();
    void testParseFromFile();
    void testParseEmptyDiff();
    void testHunkLineTypes();
    void testParseDiffWithCaption();
    void testGetTypeString();
    void testParseDiffWithNoNewlineIndicator();
};

#endif // TESTGITPARSER_H