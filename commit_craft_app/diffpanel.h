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
    Modified,
    Separator
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

    /// Установить реальные номера строк для отображения (из исходного файла)
    void setLineNumbers(const QVector<int> &numbers);

    /// Переопределение отрисовки номеров строк
    void lineNumberAreaPaintEvent(QPaintEvent *event) override;

    /// Очистить diff-данные
    void clearDiffData();

    /// Установить строки-заполнители (серый фон для пустых строк)
    void setPlaceholderLines(const QSet<int> &lines);

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
    QSet<int> m_placeholderLines; // Номера строк-заполнителей (серый фон)
    QVector<int> m_lineNumbers;   // Реальные номера строк (0-based, -1 для заполнителей)
};

#endif // DIFFPANEL_H
