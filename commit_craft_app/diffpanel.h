#ifndef DIFFPANEL_H
#define DIFFPANEL_H

#include "codeeditor.h"
#include <QMap>
#include <QColor>
#include <QVector>
#include <QPair>

enum class DiffType {
    Unchanged,
    Added,
    Removed,
    Modified
};

struct LineDiffInfo {
    DiffType type = DiffType::Unchanged;
    int lineNumber = -1;
    QVector<QPair<int, int>> changedRanges;
};

/// Панель для отображения одной стороны diff (staged или current).
/// Наследуется от CodeEditor для получения номеров строк, зума и подсветки синтаксиса.
class DiffPanel : public CodeEditor
{
    Q_OBJECT

public:
    explicit DiffPanel(QWidget *parent = nullptr);

    /// Установка текстового содержимого
    void setContent(const QString &text);

    /// Установка diff-данных (вызывается после setContent)
    void setDiffData(const QMap<int, LineDiffInfo> &lineDiffMap);

    /// Очистить diff-данные
    void clearDiffData();

private:
    /// Применить diff-подсветку через ExtraSelections (фон ПОД текстом)
    void applyDiffHighlighting();

    QMap<int, LineDiffInfo> m_lineDiffMap;
};

#endif // DIFFPANEL_H
