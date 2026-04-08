#include "diffpanel.h"

#include <QTextBlock>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QPainter>

DiffPanel::DiffPanel(QWidget *parent)
    : CodeEditor(parent)
{
    // DiffPanel всегда только для чтения (diff viewer)
    setReadOnly(true);

    // Разрываем все соединения cursorPositionChanged от CodeEditor
    // и переподключаем к нашему highlightCurrentLine
    disconnect(this, &QPlainTextEdit::cursorPositionChanged, nullptr, nullptr);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &DiffPanel::highlightCurrentLine);

    // Очищаем любые существующие выделения
    setExtraSelections({});
}

void DiffPanel::setContent(const QString &text)
{
    setPlainText(text);
    QTextCursor cursor(document());
    cursor.setPosition(0);
    setTextCursor(cursor);
}

void DiffPanel::setDiffData(const QMap<int, LineDiffInfo> &lineDiffMap)
{
    m_lineDiffMap = lineDiffMap;
    applyDiffHighlighting();
}

void DiffPanel::clearDiffData()
{
    m_lineDiffMap.clear();
    m_placeholderLines.clear();
    setExtraSelections({});
}

void DiffPanel::setPlaceholderLines(const QSet<int> &lines)
{
    m_placeholderLines = lines;
    highlightCurrentLineNoEmit();
}

void DiffPanel::setLineNumbers(const QVector<int> &numbers)
{
    m_lineNumbers = numbers;
    update();
}

void DiffPanel::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    QColor textColor(140, 140, 140);
    painter.setPen(textColor);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString numberStr;
            if (blockNumber >= 0 && blockNumber < m_lineNumbers.size()) {
                int realNum = m_lineNumbers[blockNumber];
                if (realNum >= 0) {
                    numberStr = QString::number(realNum + 1); // 1-based
                } else {
                    numberStr = ""; // Placeholder line
                }
            } else {
                numberStr = QString::number(blockNumber + 1); // Fallback
            }
            painter.drawText(0,
                             top,
                             lineNumberArea->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             numberStr);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void DiffPanel::setCursorToLine(int line)
{
    if (m_updatingCursor)
        return;
    m_updatingCursor = true;

    if (line >= 0 && line < document()->blockCount()) {
        QTextBlock block = document()->findBlockByNumber(line);
        if (block.isValid()) {
            // Блокируем сигнал cursorPositionChanged, чтобы не было рекурсии
            const QSignalBlocker blocker(this);
            QTextCursor cursor(block);
            setTextCursor(cursor);
            ensureCursorVisible();
            // Подсвечиваем вручную, без испускания сигнала
            highlightCurrentLineNoEmit();
        }
    }

    m_updatingCursor = false;
}

void DiffPanel::highlightCurrentLine()
{
    if (m_updatingCursor)
        return;
    m_updatingCursor = true;

    highlightCurrentLineNoEmit();

    // Испускаем сигнал для синхронизации с другой панелью
    emit panelCursorMoved(textCursor().blockNumber());

    m_updatingCursor = false;
}

void DiffPanel::highlightCurrentLineNoEmit()
{
    QList<QTextEdit::ExtraSelection> selections;

    // Подсветка текущей строки (жёлтая)
    QTextEdit::ExtraSelection selection;
    QColor lineColor = QColor(Qt::yellow).lighter(160);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    selections.append(selection);

    // Цвета
    const QColor removedColor(255, 200, 200);
    const QColor addedColor(200, 255, 200);
    const QColor modifiedColor(180, 200, 255);
    const QColor placeholderColor(230, 230, 230); // серый для пустых строк

    // Добавляем diff-подсветку и placeholder-подсветку
    QTextCursor cursor(document());
    QTextBlock block = document()->begin();
    int blockNumber = 0;

    while (block.isValid()) {
        auto it = m_lineDiffMap.find(blockNumber);
        QColor bgColor;
        QColor fgColor; // Цвет текста
        bool setFg = false;

        // Проверяем placeholder (серый фон)
        if (m_placeholderLines.contains(blockNumber)) {
            bgColor = placeholderColor;
        }
        // Проверяем разделитель чанков
        else if (it != m_lineDiffMap.end() && it->type == DiffType::Separator) {
            bgColor = QColor(240, 240, 240); // Светло-серый фон
            fgColor = QColor(150, 150, 150); // Темно-серый текст для "..."
            setFg = true;
        }
        // Проверяем diff (красный/зелёный/синий)
        else {
            if (it != m_lineDiffMap.end() && it->type != DiffType::Unchanged) {
                switch (it->type) {
                    case DiffType::Removed:  bgColor = removedColor; break;
                    case DiffType::Added:    bgColor = addedColor; break;
                    case DiffType::Modified: bgColor = modifiedColor; break;
                    default: break;
                }
            }
        }

        if (bgColor.isValid()) {
            cursor.setPosition(block.position());
            cursor.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);

            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format.setBackground(bgColor);
            if (setFg) {
                selection.format.setForeground(fgColor);
            }
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);

            selections.append(selection);
        }

        block = block.next();
        blockNumber++;
    }

    setExtraSelections(selections);
}

void DiffPanel::applyDiffHighlighting()
{
    QList<QTextEdit::ExtraSelection> selections;
    QTextCursor cursor(document());

    // Цвета фонов
    const QColor removedColor(255, 200, 200);
    const QColor addedColor(200, 255, 200);
    const QColor modifiedColor(180, 200, 255);

    // Проходим по всем строкам документа
    QTextBlock block = document()->begin();
    int blockNumber = 0;

    while (block.isValid()) {
        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end() && it->type != DiffType::Unchanged) {
            QColor bgColor;
            switch (it->type) {
                case DiffType::Removed:  bgColor = removedColor; break;
                case DiffType::Added:    bgColor = addedColor; break;
                case DiffType::Modified: bgColor = modifiedColor; break;
                default: break;
            }

            if (bgColor.isValid()) {
                // Выделяем всю строку
                cursor.setPosition(block.position());
                cursor.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);

                QTextEdit::ExtraSelection selection;
                selection.cursor = cursor;
                selection.format.setBackground(bgColor);
                // FullWidthSelection растягивает фон на всю ширину виджета
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);

                selections.append(selection);
            }
        }

        block = block.next();
        blockNumber++;
    }

    // Применяем выделения. Qt сам нарисует их ПОД текстом.
    setExtraSelections(selections);
}

void DiffPanel::paintEvent(QPaintEvent *event)
{
    // Сначала рисуем базовый контент
    CodeEditor::paintEvent(event);

    // Затем рисуем горизонтальные линии-разделители чанков
    QPainter painter(viewport());
    
    QTextBlock block = document()->begin();
    int blockNumber = 0;

    while (block.isValid()) {
        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end() && it->type == DiffType::Separator) {
            // Получаем геометрию блока строки-разделителя
            QRectF blockRect = blockBoundingRect(block).translated(contentOffset());
            
            // Рисуем горизонтальную линию по центру строки-разделителя
            int lineY = qRound(blockRect.top() + blockRect.height() / 2.0);
            
            QPen pen(QColor(180, 180, 180), 1, Qt::DashLine);
            painter.setPen(pen);
            painter.drawLine(0, lineY, viewport()->width(), lineY);
        }

        block = block.next();
        blockNumber++;
    }
}
