#include "codeeditor.h"
#include "linenumberarea.h"
#include "syntaxhighlighter.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFontDatabase>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QWheelEvent>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), m_zoom(100), m_syntaxHighlighting(true)
{
    // Установить шрифт Consolas по умолчанию (monospace)
    QFont defaultFont;
    if (QFontDatabase::hasFamily("Consolas")) {
        defaultFont.setFamily("Consolas");
    } else {
        defaultFont.setFamily(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    }
    defaultFont.setPointSize(10);
    setFont(defaultFont);

    lineNumberArea = new LineNumberArea(this);
    highlighter = new Highlighter(document());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    
    // Disable word wrap
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn(1);
        } else {
            zoomOut(1);
        }
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        // Современный голубоватый оттенок для текущей строки
        QColor lineColor(210, 230, 255, 80);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
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
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0,
                             top,
                             lineNumberArea->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::zoomIn(int range)
{
    setZoom(m_zoom + range * 10);
}

void CodeEditor::zoomOut(int range)
{
    setZoom(m_zoom - range * 10);
}

void CodeEditor::setZoom(int zoom)
{
    int oldZoom = m_zoom;
    m_zoom = qBound(10, zoom, 150); // Limit zoom between 10% and 300%

    // Only update if zoom actually changed
    if (m_zoom != oldZoom) {
        // Calculate the font size based on zoom level
        QFont f = font();
        f.setPointSizeF(14.0 * m_zoom / 100.0);
        setFont(f);

        // Update line number area
        updateLineNumberAreaWidth(0);

        // Emit zoom changed signal
        emit zoomChanged(m_zoom);
    }
}

int CodeEditor::zoom() const
{
    return m_zoom;
}

void CodeEditor::setSyntaxHighlighting(bool enabled)
{
    if (m_syntaxHighlighting == enabled)
        return;

    m_syntaxHighlighting = enabled;

    if (enabled) {
        highlighter->rehighlight();
    } else {
        // Clear formatting
        QTextCharFormat defaultFormat;
        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(defaultFormat);
    }
}

bool CodeEditor::syntaxHighlighting() const
{
    return m_syntaxHighlighting;
}

QRectF CodeEditor::blockViewportRect(int blockNumber) const
{
    QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return QRectF();
    return blockBoundingGeometry(block).translated(contentOffset());
}
