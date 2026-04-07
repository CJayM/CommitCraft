#include "tst_intralinediff.h"

void TestIntraLineDiff::initTestCase() {}
void TestIntraLineDiff::cleanupTestCase() {}

void TestIntraLineDiff::testIdenticalLines()
{
    QString oldLine = "int x = 42;";
    QString newLine = "int x = 42;";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    QVERIFY(oldRanges.isEmpty());
    QVERIFY(newRanges.isEmpty());
}

void TestIntraLineDiff::testSimpleChange()
{
    QString oldLine = "int x = 42;";
    QString newLine = "int x = 100;";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    // "42" изменено на "100" — должны быть диапазоны изменений
    QVERIFY(!oldRanges.isEmpty());
    QVERIFY(!newRanges.isEmpty());
}

void TestIntraLineDiff::testCompleteReplacement()
{
    QString oldLine = "hello world";
    QString newLine = "goodbye world";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    // "hello" → "goodbye"
    QVERIFY(!oldRanges.isEmpty());
    QVERIFY(!newRanges.isEmpty());
}

void TestIntraLineDiff::testEmptyStrings()
{
    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges("", "");

    QVERIFY(oldRanges.isEmpty());
    QVERIFY(newRanges.isEmpty());

    // Одна пустая, одна не пустая
    auto [oldRanges2, newRanges2] =
        IntraLineDiffCalculator::computeChangedRanges("", "some text");

    QVERIFY(oldRanges2.isEmpty());
    QCOMPARE(newRanges2.size(), 1);
    QCOMPARE(newRanges2[0].first, 0);
    QCOMPARE(newRanges2[0].second, 9);
}

void TestIntraLineDiff::testLongString()
{
    // Строка > 200 символов — должен использоваться word diff
    QString oldLine = "this is a very long line that contains many words and should trigger the word-based diff algorithm instead of character-based because it exceeds the threshold of two hundred characters total length";
    QString newLine = "this is a very long line that contains MANY DIFFERENT words and should trigger the word-based diff algorithm instead of character-based because it exceeds the threshold of two hundred characters total length";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    // Должны быть обнаружены изменения
    QVERIFY(!oldRanges.isEmpty() || !newRanges.isEmpty());
}
