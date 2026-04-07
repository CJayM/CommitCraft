#ifndef DIFFPANEL_H
#define DIFFPANEL_H

#include "codeeditor.h"
#include <QMap>
#include <QColor>
#include <QPainter>
#include <QTextBlock>

/// Тип diff-строки для подсветки
enum class DiffType {
    Unchanged,   ///< Не изменялась — без фона
    Added,       ///< Добавлена — зелёный фон
    Removed,     ///< Удалена — красный фон
    Modified     ///< Модифицирована — синий фон (+ intra-line подсветка)
};

/// Информация о diff для одной строки
struct LineDiffInfo {
    DiffType type = DiffType::Unchanged;
    int lineNumber = -1;                       ///< 0-based номер строки в панели
    QVector<QPair<int, int>> changedRanges;   ///< (start, length) для intra-line подсветки
};

/// Панель для отображения одной стороны diff (staged или current).
/// Наследуется от CodeEditor, получая номера строк, зум и подсветку синтаксиса.
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

protected:
    /// Переопределённый paintEvent: сначала текст, потом фоны ПОВЕРХ
    void paintEvent(QPaintEvent *event) override;

private:
    /// Рисует фоны строк (красный/зелёный/синий) поверх текста
    void drawLineBackgrounds(QPainter &painter);

    /// Рисует intra-line подсветку (тёмно-синие фрагменты на Modified строках)
    void drawIntraLineHighlights(QPainter &painter);

    /// Цвета для diff-подсветки
    QColor removedColor() const;
    QColor addedColor() const;
    QColor modifiedColor() const;
    QColor intraLineColor() const;

    QMap<int, LineDiffInfo> m_lineDiffMap;
};

#endif // DIFFPANEL_H
