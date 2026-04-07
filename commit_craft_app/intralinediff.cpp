#include "intralinediff.h"

#include <QVarLengthArray>
#include <QRegularExpression>
#include <algorithm>

static const int WORD_DIFF_THRESHOLD = 200;

QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>>
IntraLineDiffCalculator::computeChangedRanges(const QString &oldLine, const QString &newLine)
{
    QVector<QPair<int, int>> oldRanges;
    QVector<QPair<int, int>> newRanges;

    if (oldLine == newLine) {
        // Строки идентичны — никаких изменений
        return qMakePair(oldRanges, newRanges);
    }

    if (oldLine.length() > WORD_DIFF_THRESHOLD || newLine.length() > WORD_DIFF_THRESHOLD) {
        // Длинные строки — diff на уровне слов
        computeWordDiff(oldLine, newLine, oldRanges, newRanges);
    } else {
        // Короткие строки — посимвольный LCS
        computeLCS(oldLine, newLine, oldRanges, newRanges);
    }

    return qMakePair(oldRanges, newRanges);
}

void IntraLineDiffCalculator::computeLCS(const QString &a, const QString &b,
                                         QVector<QPair<int, int>> &aRanges,
                                         QVector<QPair<int, int>> &bRanges)
{
    const int m = a.length();
    const int n = b.length();

    if (m == 0) {
        // Вся строка b — добавлена
        if (n > 0)
            bRanges.append(qMakePair(0, n));
        return;
    }
    if (n == 0) {
        // Вся строка a — удалена
        if (m > 0)
            aRanges.append(qMakePair(0, m));
        return;
    }

    // LCS через динамическое программирование
    // Используем QVarLengthArray для stack allocation при малых размерах
    QVarLengthArray<QVarLengthArray<int>> dp(m + 1, QVarLengthArray<int>(n + 1, 0));

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = qMax(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Восстанавливаем alignment, проходя с конца
    int i = m, j = n;
    int aDelStart = -1, aDelLen = 0;
    int bAddStart = -1, bAddLen = 0;

    auto flushDel = [&]() {
        if (aDelStart >= 0 && aDelLen > 0) {
            aRanges.append(qMakePair(aDelStart, aDelLen));
            aDelStart = -1;
            aDelLen = 0;
        }
    };
    auto flushAdd = [&]() {
        if (bAddStart >= 0 && bAddLen > 0) {
            bRanges.append(qMakePair(bAddStart, bAddLen));
            bAddStart = -1;
            bAddLen = 0;
        }
    };

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
            // Символ совпал — сбросить накопленные диапазоны
            flushDel();
            flushAdd();
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            // Символ добавлен в b
            flushDel();
            if (bAddStart < 0) bAddStart = j - 1;
            ++bAddLen;
            --j;
        } else {
            // Символ удалён из a
            flushAdd();
            if (aDelStart < 0) aDelStart = i - 1;
            ++aDelLen;
            --i;
        }
    }
    flushDel();
    flushAdd();

    // Обратный порядок — восстанавливаем при обходе с конца
    std::reverse(aRanges.begin(), aRanges.end());
    std::reverse(bRanges.begin(), bRanges.end());
}

void IntraLineDiffCalculator::computeWordDiff(const QString &a, const QString &b,
                                              QVector<QPair<int, int>> &aRanges,
                                              QVector<QPair<int, int>> &bRanges)
{
    // Простая эвристика для длинных строк:
    // разбить на слова и сравнить
    const auto aWords = a.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    const auto bWords = b.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    // Если мало слов — fallback на посимвольный
    if (aWords.size() < 2 && bWords.size() < 2) {
        computeLCS(a, b, aRanges, bRanges);
        return;
    }

    // LCS на уровне слов
    const int m = aWords.size();
    const int n = bWords.size();
    QVarLengthArray<QVarLengthArray<int>> dp(m + 1, QVarLengthArray<int>(n + 1, 0));

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (aWords[i - 1] == bWords[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = qMax(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Найти изменённые слова и преобразовать в символьные диапазоны
    int i = m, j = n;
    int aDelStart = -1, aDelEnd = -1;
    int bAddStart = -1, bAddEnd = -1;

    auto flushDel = [&]() {
        if (aDelStart >= 0 && aDelEnd >= aDelStart) {
            aRanges.append(qMakePair(aDelStart, aDelEnd - aDelStart));
            aDelStart = -1;
            aDelEnd = -1;
        }
    };
    auto flushAdd = [&]() {
        if (bAddStart >= 0 && bAddEnd >= bAddStart) {
            bRanges.append(qMakePair(bAddStart, bAddEnd - bAddStart));
            bAddStart = -1;
            bAddEnd = -1;
        }
    };

    // Найти позиции слов в исходных строках
    auto wordPositions = [](const QString &text, const QStringList &words) {
        QVector<QPair<int, int>> positions;
        int pos = 0;
        for (const auto &word : words) {
            pos = text.indexOf(word, pos);
            if (pos >= 0) {
                positions.append(qMakePair(pos, word.length()));
                pos += word.length();
            }
        }
        return positions;
    };

    const auto aPos = wordPositions(a, aWords);
    const auto bPos = wordPositions(b, bWords);

    i = m; j = n;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && aWords[i - 1] == bWords[j - 1]) {
            flushDel();
            flushAdd();
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            flushDel();
            if (bAddStart < 0 && j - 1 < bPos.size()) {
                bAddStart = bPos[j - 1].first;
                bAddEnd = bPos[j - 1].first + bPos[j - 1].second;
            } else if (bAddStart >= 0 && j - 1 < bPos.size()) {
                bAddEnd = qMax(bAddEnd, bPos[j - 1].first + bPos[j - 1].second);
                bAddStart = qMin(bAddStart, bPos[j - 1].first);
            }
            --j;
        } else {
            flushAdd();
            if (aDelStart < 0 && i - 1 < aPos.size()) {
                aDelStart = aPos[i - 1].first;
                aDelEnd = aPos[i - 1].first + aPos[i - 1].second;
            } else if (aDelStart >= 0 && i - 1 < aPos.size()) {
                aDelEnd = qMax(aDelEnd, aPos[i - 1].first + aPos[i - 1].second);
                aDelStart = qMin(aDelStart, aPos[i - 1].first);
            }
            --i;
        }
    }
    flushDel();
    flushAdd();
}
