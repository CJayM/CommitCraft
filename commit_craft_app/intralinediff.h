#ifndef INTRALINEDIFF_H
#define INTRALINEDIFF_H

#include <QString>
#include <QVector>
#include <QPair>

/// Вычисляет intra-line diff (изменённые диапазоны символов) для пары строк.
/// Использует LCS (Longest Common Subsequence) на уровне символов.
class IntraLineDiffCalculator
{
public:
    /// Вычислить диапазоны изменённых символов.
    /// \param oldLine Строка из левой панели (удалённая версия)
    /// \param newLine Строка из правой панели (добавленная версия)
    /// \return Пару (removedRanges для oldLine, addedRanges для newLine)
    static QPair<QVector<QPair<int, int>>, QVector<QPair<int, int>>>
        computeChangedRanges(const QString &oldLine, const QString &newLine);

private:
    /// Построить LCS-матрицу и восстановить alignment
    static void computeLCS(const QString &a, const QString &b,
                           QVector<QPair<int, int>> &aRanges,
                           QVector<QPair<int, int>> &bRanges);

    /// Для длинных строк (>200 символов) используем diff на уровне слов
    static void computeWordDiff(const QString &a, const QString &b,
                                QVector<QPair<int, int>> &aRanges,
                                QVector<QPair<int, int>> &bRanges);
};

#endif // INTRALINEDIFF_H
