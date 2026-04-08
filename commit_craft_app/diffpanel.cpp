#include "diffpanel.h"

#include <QTextBlock>
#include <QScrollBar>
#include <QSignalBlocker>

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
    setExtraSelections({});
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

    // Добавляем diff-подсветку поверх
    QTextCursor cursor(document());
    QTextBlock block = document()->begin();
    int blockNumber = 0;

    const QColor removedColor(255, 200, 200);
    const QColor addedColor(200, 255, 200);
    const QColor modifiedColor(180, 200, 255);

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
                cursor.setPosition(block.position());
                cursor.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);

                QTextEdit::ExtraSelection selection;
                selection.cursor = cursor;
                selection.format.setBackground(bgColor);
                selection.format.setProperty(QTextFormat::FullWidthSelection, true);

                selections.append(selection);
            }
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
