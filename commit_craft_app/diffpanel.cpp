#include "diffpanel.h"

#include <QPaintEvent>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>

DiffPanel::DiffPanel(QWidget *parent)
    : CodeEditor(parent)
{
    // DiffPanel всегда только для чтения (diff viewer)
    setReadOnly(true);
}

void DiffPanel::setContent(const QString &text)
{
    setPlainText(text);
    // Сбросить позицию курсора в начало
    QTextCursor cursor(document());
    cursor.setPosition(0);
    setTextCursor(cursor);
}

void DiffPanel::setDiffData(const QMap<int, LineDiffInfo> &lineDiffMap)
{
    m_lineDiffMap = lineDiffMap;
    update(); // Перерисовать с новыми фонами
}

void DiffPanel::clearDiffData()
{
    m_lineDiffMap.clear();
    update();
}

void DiffPanel::paintEvent(QPaintEvent *event)
{
    // 1. Сначала стандартная отрисовка CodeEditor (текст, номера строк, selection, подсветка синтаксиса)
    CodeEditor::paintEvent(event);

    // 2. Поверх текста рисуем diff-фоны
    QPainter painter(viewport());

    drawLineBackgrounds(painter);
    drawIntraLineHighlights(painter);
}

void DiffPanel::drawLineBackgrounds(QPainter &painter)
{
    if (m_lineDiffMap.isEmpty())
        return;

    const QColor removed = removedColor();
    const QColor added = addedColor();
    const QColor modified = modifiedColor();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    const int viewportHeight = viewport()->height();

    while (block.isValid() && top <= viewportHeight) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            block = block.next();
            blockNumber++;
            continue;
        }

        const int bottom = top + qRound(blockBoundingRect(block).height());

        // Найти diff-тип для этой строки
        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end()) {
            QColor bgColor;
            switch (it->type) {
                case DiffType::Removed:  bgColor = removed;  break;
                case DiffType::Added:    bgColor = added;    break;
                case DiffType::Modified: bgColor = modified; break;
                default: break;
            }

            if (bgColor.isValid()) {
                painter.fillRect(0, top, viewport()->width(), bottom - top, bgColor);
            }
        }

        top = bottom;
        block = block.next();
        blockNumber++;
    }
}

void DiffPanel::drawIntraLineHighlights(QPainter &painter)
{
    if (m_lineDiffMap.isEmpty())
        return;

    const QColor intraColor = intraLineColor();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    const int viewportHeight = viewport()->height();

    while (block.isValid() && top <= viewportHeight) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            block = block.next();
            blockNumber++;
            continue;
        }

        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end() && it->type == DiffType::Modified && !it->changedRanges.isEmpty()) {
            const int blockPos = block.position();
            const int lineHeight = qRound(blockBoundingRect(block).height());

            for (const auto &range : it->changedRanges) {
                const int charStart = range.first;
                const int charLength = range.second;

                // Получить QRect для диапазона символов
                QTextCursor cursor(document());
                cursor.setPosition(blockPos + charStart);
                const QRect startRect = cursorRect(cursor);
                if (!startRect.isValid()) {
                    top += lineHeight;
                    block = block.next();
                    blockNumber++;
                    continue;
                }

                cursor.setPosition(blockPos + charStart + charLength);
                const QRect endRect = cursorRect(cursor);

                const int highlightLeft = startRect.left();
                const int highlightRight = endRect.right();
                const int highlightWidth = highlightRight - highlightLeft;

                const QRect highlightRect(highlightLeft, top, highlightWidth, lineHeight);
                painter.fillRect(highlightRect, intraColor);
            }
        }

        top += qRound(blockBoundingRect(block).height());
        block = block.next();
        blockNumber++;
    }
}

QColor DiffPanel::removedColor() const  { return QColor(255, 200, 200); } // красный
QColor DiffPanel::addedColor() const    { return QColor(200, 255, 200); } // зелёный
QColor DiffPanel::modifiedColor() const { return QColor(180, 200, 255); } // синий
QColor DiffPanel::intraLineColor() const { return QColor(140, 170, 255, 120); } // полупрозрачный синий
