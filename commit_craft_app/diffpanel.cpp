#include "diffpanel.h"

#include <QTextBlock>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QPainter>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

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
    painter.fillRect(event->rect(), QColor(245, 247, 250));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    QColor textColor(148, 155, 164);
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

    // Подсветка текущей строки (голубоватая)
    QTextEdit::ExtraSelection selection;
    QColor lineColor(210, 230, 255, 80);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    selections.append(selection);

    // Современные цвета diff (GitHub-style)
    const QColor removedColor(255, 235, 233);      // #ffebe9
    const QColor addedColor(218, 251, 225);         // #dafbe1
    const QColor modifiedColor(213, 228, 255);      // Subtle blue
    const QColor placeholderColor(246, 248, 250);   // #f6f8fa - светлый серый

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
            bgColor = QColor(246, 248, 250);      // #f6f8fa - светлый серый
            fgColor = QColor(148, 155, 164);      // #949ba4 - серый текст
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

    // Современные цвета фонов (GitHub-style)
    const QColor removedColor(255, 235, 233);
    const QColor addedColor(218, 251, 225);
    const QColor modifiedColor(213, 228, 255);

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
    painter.setClipRect(event->rect());
    
    // Получаем видимую область
    QRect visibleRect = viewport()->rect();
    
    // Находим первый видимый блок
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    while (block.isValid()) {
        QRectF blockRect = blockBoundingGeometry(block).translated(contentOffset()).toRect();
        
        // Если блок вышел за видимую область, прерываем
        if (blockRect.top() > visibleRect.bottom())
            break;
        
        // Рисуем линию только если блок частично или полностью виден
        if (blockRect.bottom() >= visibleRect.top()) {
            auto it = m_lineDiffMap.find(blockNumber);
            if (it != m_lineDiffMap.end() && it->type == DiffType::Separator) {
                // Рисуем горизонтальную линию по центру строки-разделителя
                int lineY = blockRect.top() + blockRect.height() / 2;
                
                painter.save();
                painter.setPen(QPen(QColor(208, 215, 222), 1, Qt::DashLine));
                painter.drawLine(visibleRect.left(), lineY, visibleRect.right(), lineY);
                painter.restore();
            }
        }

        block = block.next();
        blockNumber++;
    }
}

void DiffPanel::contextMenuEvent(QContextMenuEvent *event)
{
    // Показываем меню только если есть diff-изменения
    bool hasChanges = false;
    for (auto it = m_lineDiffMap.constBegin(); it != m_lineDiffMap.constEnd(); ++it) {
        if (it->type != DiffType::Unchanged) {
            hasChanges = true;
            break;
        }
    }
    if (!hasChanges) {
        QPlainTextEdit::contextMenuEvent(event);
        return;
    }

    QMenu menu(this);

    QAction *stageAction = menu.addAction(tr("Stage Selected Lines"));
    QAction *revertAction = menu.addAction(tr("Revert Selected Lines"));

    connect(stageAction, &QAction::triggered, this, &DiffPanel::stageSelectedRequested);
    connect(revertAction, &QAction::triggered, this, &DiffPanel::revertSelectedRequested);

    menu.exec(event->globalPos());
}
