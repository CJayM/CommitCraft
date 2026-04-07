#ifndef DIFFHIGHLIGHTER_H
#define DIFFHIGHLIGHTER_H

#include "diffpanel.h"
#include "intralinediff.h"

#include <gitparser.h>

#include <QMap>
#include <QVector>
#include <QString>
#include <QPair>
#include <cmath>

/// Преобразует данные GitParser (QList<Hunk>) в LineDiffInfo для каждой панели.
/// Также вычисляет intra-line diff для Modified строк.
class DiffHighlighter
{
public:
    DiffHighlighter() = default;

    /// Построить LineDiffMap для левой панели (staged / HEAD версия).
    /// Строки типа Removed → отображаются, Added → пропускаются.
    static QMap<int, LineDiffInfo> buildLineDiffMapLeft(const QList<Hunk> &hunks);

    /// Построить LineDiffMap для правой панели (current / рабочая версия).
    /// Строки типа Added → отображаются, Removed → пропускаются.
    static QMap<int, LineDiffInfo> buildLineDiffMapRight(const QList<Hunk> &hunks);

    /// Вычислить intra-line diff для Modified строк.
    /// Модифицирует lineDiffMap, заполняя changedRanges.
    static void computeIntraLineDiffs(QMap<int, LineDiffInfo> &leftMap,
                                      QMap<int, LineDiffInfo> &rightMap,
                                      const QList<Hunk> &hunks);

private:
    /// Определить, являются ли две строки «модификацией» друг друга.
    /// Используем similarity ratio > 0.3.
    static bool isModifiedPair(const QString &removed, const QString &added);

    /// Вычислить similarity ratio (0.0..1.0) между двумя строками.
    static double similarityRatio(const QString &a, const QString &b);
};

#endif // DIFFHIGHLIGHTER_H
