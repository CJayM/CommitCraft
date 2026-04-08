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

    /// Переместить курсор на указанную строку (для синхронизации с другой панелью)
    void setCursorToLine(int line);

signals:
    /// Сигнал перемещения курсора с номером строки (для синхронизации)
    void panelCursorMoved(int blockNumber);

private:
    /// Применить diff-подсветку через ExtraSelections (фон ПОД текстом)
    void applyDiffHighlighting();

    /// Переопределение для сохранения diff-подсветки при выделении текущей строки
    void highlightCurrentLine();

    /// Подсветка без испускания сигнала (для программной синхронизации)
    void highlightCurrentLineNoEmit();

    QMap<int, LineDiffInfo> m_lineDiffMap;
    bool m_updatingCursor = false;
};

#endif // DIFFPANEL_H
