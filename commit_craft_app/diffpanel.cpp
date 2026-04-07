#include "diffpanel.h"

#include <QTextBlock>
#include <QScrollBar>

DiffPanel::DiffPanel(QWidget *parent)
    : CodeEditor(parent)
{
    // DiffPanel всегда только для чтения (diff viewer)
    setReadOnly(true);

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
    setExtraSelections({});
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
