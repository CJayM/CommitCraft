#include "diffhighlighter.h"

#include <QVarLengthArray>
#include <algorithm>

// ---------------------------------------------------------------------------
// Построение LineDiffMap для левой панели (staged / HEAD)
// ---------------------------------------------------------------------------
QMap<int, LineDiffInfo> DiffHighlighter::buildLineDiffMapLeft(const QList<Hunk> &hunks)
{
    QMap<int, LineDiffInfo> result;

    for (const auto &hunk : hunks) {
        int leftLineNum = hunk.leftStart - 1; // 0-based
        int rightLineNum = hunk.rightStart - 1; // 0-based, для поиска пар

        for (int i = 0; i < hunk.lines.size(); ++i) {
            const auto &line = hunk.lines[i];

            switch (line.type) {
            case HunkLine::Removed: {
                LineDiffInfo info;
                info.type = DiffType::Removed; // Временно, может стать Modified
                info.lineNumber = leftLineNum;
                result[leftLineNum] = info;
                ++leftLineNum;
                break;
            }
            case HunkLine::Added:
                // В левой панели добавленные строки не отображаются
                ++rightLineNum;
                break;
            case HunkLine::Unchanged:
                // Неизменные строки — без подсветки (Unchanged)
                ++leftLineNum;
                ++rightLineNum;
                break;
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Построение LineDiffMap для правой панели (current / рабочая)
// ---------------------------------------------------------------------------
QMap<int, LineDiffInfo> DiffHighlighter::buildLineDiffMapRight(const QList<Hunk> &hunks)
{
    QMap<int, LineDiffInfo> result;

    for (const auto &hunk : hunks) {
        int leftLineNum = hunk.leftStart - 1;
        int rightLineNum = hunk.rightStart - 1; // 0-based

        for (int i = 0; i < hunk.lines.size(); ++i) {
            const auto &line = hunk.lines[i];

            switch (line.type) {
            case HunkLine::Removed:
                // В правой панели удалённые строки не отображаются
                ++leftLineNum;
                break;
            case HunkLine::Added: {
                LineDiffInfo info;
                info.type = DiffType::Added; // Временно, может стать Modified
                info.lineNumber = rightLineNum;
                result[rightLineNum] = info;
                ++rightLineNum;
                break;
            }
            case HunkLine::Unchanged:
                ++leftLineNum;
                ++rightLineNum;
                break;
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Вычисление intra-line diff для Modified строк
// ---------------------------------------------------------------------------
void DiffHighlighter::computeIntraLineDiffs(QMap<int, LineDiffInfo> &leftMap,
                                            QMap<int, LineDiffInfo> &rightMap,
                                            const QList<Hunk> &hunks)
{
    // Проходим по всем ханкам и ищем пары Removed → Added
    for (const auto &hunk : hunks) {
        int leftLineNum = hunk.leftStart - 1;
        int rightLineNum = hunk.rightStart - 1;

        // Собираем Removed и Added строки в пределах этого ханка
        struct LineEntry {
            HunkLine::Type type;
            QString content;
            int leftNum;   // номер в левой панели (для Removed)
            int rightNum;  // номер в правой панели (для Added)
        };
        QVector<LineEntry> removedLines;
        QVector<LineEntry> addedLines;

        for (int i = 0; i < hunk.lines.size(); ++i) {
            const auto &line = hunk.lines[i];
            switch (line.type) {
            case HunkLine::Removed:
                removedLines.append({line.type, line.content, leftLineNum, -1});
                ++leftLineNum;
                break;
            case HunkLine::Added:
                addedLines.append({line.type, line.content, -1, rightLineNum});
                ++rightLineNum;
                break;
            case HunkLine::Unchanged:
                ++leftLineNum;
                ++rightLineNum;
                break;
            }
        }

        // Попарно сопоставляем Removed и Added
        int rIdx = 0, aIdx = 0;
        while (rIdx < removedLines.size() && aIdx < addedLines.size()) {
            const auto &rem = removedLines[rIdx];
            const auto &add = addedLines[aIdx];

            if (isModifiedPair(rem.content, add.content)) {
                // Это модификация — обновляем тип и вычисляем intra-line diff
                auto lit = leftMap.find(rem.leftNum);
                auto rit = rightMap.find(add.rightNum);

                if (lit != leftMap.end()) {
                    lit->type = DiffType::Modified;
                }
                if (rit != rightMap.end()) {
                    rit->type = DiffType::Modified;
                }

                // Вычислить изменённые диапазоны
                auto [oldRanges, newRanges] =
                    IntraLineDiffCalculator::computeChangedRanges(rem.content, add.content);

                if (lit != leftMap.end()) {
                    lit->changedRanges = oldRanges;
                }
                if (rit != rightMap.end()) {
                    rit->changedRanges = newRanges;
                }

                ++rIdx;
                ++aIdx;
            } else {
                // Не похожи — значит это отдельное удаление и отдельное добавление
                // Не меняем тип (останется Removed/Added)
                if (similarityRatio(rem.content, add.content) < 0.1) {
                    // Совсем не похожи — пробуем следующий pair
                    ++aIdx;
                } else {
                    ++rIdx;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Проверка: являются ли две строки «модификацией»
// ---------------------------------------------------------------------------
bool DiffHighlighter::isModifiedPair(const QString &removed, const QString &added)
{
    return similarityRatio(removed, added) > 0.3;
}

double DiffHighlighter::similarityRatio(const QString &a, const QString &b)
{
    if (a.isEmpty() && b.isEmpty()) return 1.0;
    if (a.isEmpty() || b.isEmpty()) return 0.0;

    // Используем LCS для вычисления similarity
    const int m = a.length();
    const int n = b.length();

    // Оптимизация: если строки сильно различаются по длине, ratio будет низким
    int maxLen = qMax(m, n);
    if (maxLen > 500) {
        // Приближённая оценка по первым 500 символам
        return similarityRatio(a.left(500), b.left(500));
    }

    // LCS через DP
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

    int lcsLen = dp[m][n];
    return (2.0 * lcsLen) / (m + n); // Dice coefficient
}
