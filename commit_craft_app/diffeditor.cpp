#include "diffeditor.h"

#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTextBlock>
#include <QSet>

DiffEditor::DiffEditor(QWidget *parent)
    : QWidget(parent)
    , m_leftPanel(new DiffPanel(this))
    , m_rightPanel(new DiffPanel(this))
    , m_currentHunkIndex(-1)
    , m_syncingScroll(false)
    , m_layout(new QVBoxLayout(this))
    , m_splitter(new QSplitter(Qt::Horizontal, this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Добавить панели в splitter
    m_splitter->addWidget(m_leftPanel);
    m_splitter->addWidget(m_rightPanel);

    m_layout->addWidget(m_splitter);
    setLayout(m_layout);

    // Подключить синхронизацию скролла
    connect(m_leftPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollLeftToRight);
    connect(m_rightPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollRightToLeft);

    // Подключить синхронизацию зума (через CodeEditor)
    connect(m_leftPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);
    connect(m_rightPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);

    // Подключить синхронизацию подсветки текущей строки (перемещение курсора)
    connect(m_leftPanel, &DiffPanel::panelCursorMoved, this, [this](int blockNum) {
        m_rightPanel->setCursorToLine(blockNum);
    });
    connect(m_rightPanel, &DiffPanel::panelCursorMoved, this, [this](int blockNum) {
        m_leftPanel->setCursorToLine(blockNum);
    });
}

void DiffEditor::setContents(const QString &leftContent,
                              const QString &rightContent,
                              const QString &fileName)
{
    m_leftFullContent = leftContent;
    m_rightFullContent = rightContent;
    m_fileName = fileName;

    // Очистить предыдущие diff-данные
    m_leftPanel->clearDiffData();
    m_rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;

    // Показать полное содержимое (будет заменено на side-by-side diff в applyDiffData)
    m_leftPanel->setContent(leftContent);
    m_rightPanel->setContent(rightContent);

    // Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);
}

void DiffEditor::applyDiffData(const QList<Hunk> &hunks)
{
    if (hunks.isEmpty()) {
        m_leftPanel->clearDiffData();
        m_rightPanel->clearDiffData();
        return;
    }

    QStringList leftLines = m_leftFullContent.split('\n');
    QStringList rightLines = m_rightFullContent.split('\n');

    QVector<SyncedLine> syncedLines = buildSyncedLines(hunks, leftLines, rightLines);

    // Сохранить ханки для навигации
    m_hunkPositions.clear();
    for (const auto &hunk : hunks) {
        m_hunkPositions.append(qMakePair(hunk.leftStart, hunk.rightStart));
    }

    // Заполнить панели синхронизированным содержимым
    populatePanels(syncedLines);

    // Извлечь и установить реальные номера строк
    QVector<int> leftNums, rightNums;
    leftNums.reserve(syncedLines.size());
    rightNums.reserve(syncedLines.size());
    for (const auto &sl : syncedLines) {
        leftNums.append(sl.leftLineNum);
        rightNums.append(sl.rightLineNum);
    }
    m_leftPanel->setLineNumbers(leftNums);
    m_rightPanel->setLineNumbers(rightNums);

    // Вычислить intra-line diff для Modified строк
    auto leftMap = DiffHighlighter::buildLineDiffMapLeft(hunks);
    auto rightMap = DiffHighlighter::buildLineDiffMapRight(hunks);
    DiffHighlighter::computeIntraLineDiffs(leftMap, rightMap, hunks);

    // НО: номера строк в leftMap/rightMap — это оригинальные номера файлов (0-based),
    // а syncedLines — это синхронизированные строки. Нужно пересчитать.
    QMap<int, LineDiffInfo> leftDiffMap, rightDiffMap;
    QSet<int> leftPlaceholders, rightPlaceholders;

    for (int i = 0; i < syncedLines.size(); ++i) {
        const auto &sl = syncedLines[i];
        if (sl.isRemoved) {
            leftDiffMap[i] = {DiffType::Removed, i, {}};
        }
        if (sl.isAdded) {
            rightDiffMap[i] = {DiffType::Added, i, {}};
        }
        if (sl.isModified) {
            // Полностью изменённые строки: слева красные (удалённые), справа зелёные (добавленные)
            leftDiffMap[i] = {DiffType::Removed, i, {}};
            rightDiffMap[i] = {DiffType::Added, i, {}};
            // Вставить intra-line ranges если есть (хотя при таком подходе они не нужны)
            auto lit = leftMap.find(sl.leftLineNum);
            auto rit = rightMap.find(sl.rightLineNum);
            if (lit != leftMap.end()) {
                leftDiffMap[i].changedRanges = lit->changedRanges;
            }
            if (rit != rightMap.end()) {
                rightDiffMap[i].changedRanges = rit->changedRanges;
            }
        }
        // Собираем placeholder-строки
        if (sl.leftIsPlaceholder)
            leftPlaceholders.insert(i);
        if (sl.rightIsPlaceholder)
            rightPlaceholders.insert(i);
    }

    m_leftPanel->setDiffData(leftDiffMap);
    m_rightPanel->setDiffData(rightDiffMap);
    m_leftPanel->setPlaceholderLines(leftPlaceholders);
    m_rightPanel->setPlaceholderLines(rightPlaceholders);
}

void DiffEditor::applyFontSettings(const QString &fontFamily, int fontSize)
{
    QFont font(fontFamily, fontSize);
    m_leftPanel->setFont(font);
    m_rightPanel->setFont(font);

    // Сброс зума на 100% чтобы избежать конфликта с новым размером шрифта
    m_leftPanel->setZoom(100);
    m_rightPanel->setZoom(100);
}

QVector<SyncedLine> DiffEditor::buildSyncedLines(const QList<Hunk> &hunks,
                                                  const QStringList &leftLines,
                                                  const QStringList &rightLines)
{
    QVector<SyncedLine> result;
    const int CONTEXT_LINES = 3;

    int prevLeftEnd = 0; // 0-based индекс следующей необработанной строки слева
    int prevRightEnd = 0;

    for (int hunkIdx = 0; hunkIdx < hunks.size(); ++hunkIdx) {
        const auto &hunk = hunks[hunkIdx];

        // Пределы hunk (1-based из GitParser)
        int hunkLeftStart = hunk.leftStart;   // 1-based
        int hunkRightStart = hunk.rightStart; // 1-based

        // --- Контекстные строки ДО hunk ---
        int contextStartLeft = qMax(prevLeftEnd, hunkLeftStart - 1 - CONTEXT_LINES);
        int contextStartRight = qMax(prevRightEnd, hunkRightStart - 1 - CONTEXT_LINES);

        int ctxLeft = contextStartLeft;
        int ctxRight = contextStartRight;
        bool isFirstChunk = (hunkIdx == 0);
        while (ctxLeft < hunkLeftStart - 1 && ctxRight < hunkRightStart - 1
               && ctxLeft < leftLines.size() && ctxRight < rightLines.size()) {
            result.append({
                leftLines[ctxLeft],
                rightLines[ctxRight],
                false, false, false, true, // context
                false, false,              // not placeholders
                ctxLeft, ctxRight,
                hunkIdx,                   // hunkIndex
                !isFirstChunk && (ctxLeft == contextStartLeft), // isHunkBoundary (first line of context after gap)
                false                      // isSeparator
            });
            ctxLeft++;
            ctxRight++;
        }

        // --- Строки внутри hunk ---
        // Обрабатываем строки hunk последовательно, сохраняя порядок как в diff.
        // Removed и Added синхронизируем попарно, Unchanged добавляем как контекст
        // на свои позиции.
        struct ChangeGroup {
            QVector<QString> removedContents;
            QVector<int> removedLeftNums;
            QVector<QString> addedContents;
            QVector<int> addedRightNums;
        };
        
        ChangeGroup currentGroup;
        int leftIdx = hunkLeftStart - 1; // 0-based
        int rightIdx = hunkRightStart - 1; // 0-based

        // Вспомогательная функция для сброса текущей группы изменений в result
        auto flushChangeGroup = [&]() {
            if (!currentGroup.removedContents.isEmpty() || !currentGroup.addedContents.isEmpty()) {
                int maxChanged = qMax(currentGroup.removedContents.size(), currentGroup.addedContents.size());
                bool isFirstInHunk = true;
                for (int i = 0; i < maxChanged; ++i) {
                    QString leftText = (i < currentGroup.removedContents.size()) ? currentGroup.removedContents[i] : "";
                    QString rightText = (i < currentGroup.addedContents.size()) ? currentGroup.addedContents[i] : "";
                    bool isRemoved = (i < currentGroup.removedContents.size());
                    bool isAdded = (i < currentGroup.addedContents.size());
                    bool isModified = isRemoved && isAdded;
                    bool leftPlaceholder = !isRemoved;  // левая сторона — пустой заполнитель
                    bool rightPlaceholder = !isAdded;   // правая сторона — пустой заполнитель

                    result.append({
                        leftText,
                        rightText,
                        isRemoved, isAdded, isModified, false,
                        leftPlaceholder, rightPlaceholder,
                        isRemoved ? currentGroup.removedLeftNums[i] : -1,
                        isAdded ? currentGroup.addedRightNums[i] : -1,
                        hunkIdx,                        // hunkIndex
                        isFirstInHunk,                  // isHunkBoundary (first changed line in hunk)
                        false                           // isSeparator
                    });
                    isFirstInHunk = false;
                }
                currentGroup = ChangeGroup{};
            }
        };

        for (const auto &line : hunk.lines) {
            switch (line.type) {
            case HunkLine::Removed:
                if (leftIdx >= 0 && leftIdx < leftLines.size()) {
                    currentGroup.removedContents.append(leftLines[leftIdx]);
                } else {
                    currentGroup.removedContents.append(line.content);
                }
                currentGroup.removedLeftNums.append(qMax(0, leftIdx));
                leftIdx++;
                break;
            case HunkLine::Added:
                if (rightIdx >= 0 && rightIdx < rightLines.size()) {
                    currentGroup.addedContents.append(rightLines[rightIdx]);
                } else {
                    currentGroup.addedContents.append(line.content);
                }
                currentGroup.addedRightNums.append(qMax(0, rightIdx));
                rightIdx++;
                break;
            case HunkLine::Unchanged:
                // Сначала сбрасываем накопленные изменения (они были ДО этой контекстной строки)
                flushChangeGroup();
                // Добавляем контекстную строку
                if (leftIdx >= 0 && leftIdx < leftLines.size() && rightIdx >= 0 && rightIdx < rightLines.size()) {
                    result.append({
                        leftLines[leftIdx],
                        rightLines[rightIdx],
                        false, false, false, true,
                        false, false,          // not placeholders
                        leftIdx, rightIdx,
                        hunkIdx,               // hunkIndex
                        false,                 // isHunkBoundary (context lines are not boundaries)
                        false                  // isSeparator
                    });
                }
                leftIdx++;
                rightIdx++;
                break;
            }
        }
        // Сбрасываем последнюю группу изменений (после последнего контекста)
        flushChangeGroup();

        prevLeftEnd = leftIdx;
        prevRightEnd = rightIdx;
    }

    // --- Контекстные строки ПОСЛЕ последнего hunk ---
    if (!hunks.isEmpty()) {
        const auto &lastHunk = hunks.last();
        int lastLeftEnd = qMax(0, lastHunk.leftStart - 1 + lastHunk.leftSize);
        int lastRightEnd = qMax(0, lastHunk.rightStart - 1 + lastHunk.rightSize);

        int contextEndLeft = qMin(lastLeftEnd + CONTEXT_LINES, leftLines.size());
        int contextEndRight = qMin(lastRightEnd + CONTEXT_LINES, rightLines.size());

        int ctxLeft = lastLeftEnd;
        int ctxRight = lastRightEnd;
        int lastHunkIdx = hunks.size() - 1;
        while (ctxLeft >= 0 && ctxLeft < contextEndLeft && ctxRight >= 0 && ctxRight < contextEndRight) {
            result.append({
                leftLines[ctxLeft],
                rightLines[ctxRight],
                false, false, false, true,
                false, false,              // not placeholders
                ctxLeft, ctxRight,
                lastHunkIdx,               // hunkIndex (last hunk)
                false,                     // isHunkBoundary
                false                      // isSeparator
            });
            ctxLeft++;
            ctxRight++;
        }
    }

    return result;
}

void DiffEditor::populatePanels(const QVector<SyncedLine> &syncedLines)
{
    QString leftText, rightText;
    for (int i = 0; i < syncedLines.size(); ++i) {
        const auto &sl = syncedLines[i];
        leftText += sl.leftText;
        rightText += sl.rightText;
        if (i < syncedLines.size() - 1) {
            leftText += '\n';
            rightText += '\n';
        }
    }

    m_leftPanel->setContent(leftText);
    m_rightPanel->setContent(rightText);
}

void DiffEditor::clear()
{
    m_leftPanel->setPlainText("");
    m_rightPanel->setPlainText("");
    m_leftPanel->clearDiffData();
    m_rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;
    m_leftFullContent.clear();
    m_rightFullContent.clear();
    m_fileName.clear();
}

void DiffEditor::navigateToNextHunk()
{
    if (m_hunkPositions.isEmpty())
        return;

    m_currentHunkIndex++;
    if (m_currentHunkIndex >= m_hunkPositions.size())
        m_currentHunkIndex = 0;

    // Прокрутить к позиции ханка
    scrollToHunk(m_currentHunkIndex);
    emit hunkNavigated(m_currentHunkIndex);
}

void DiffEditor::navigateToPrevHunk()
{
    if (m_hunkPositions.isEmpty())
        return;

    m_currentHunkIndex--;
    if (m_currentHunkIndex < 0)
        m_currentHunkIndex = m_hunkPositions.size() - 1;

    scrollToHunk(m_currentHunkIndex);
    emit hunkNavigated(m_currentHunkIndex);
}

void DiffEditor::scrollToHunk(int hunkIndex)
{
    Q_UNUSED(hunkIndex);
    // Прокрутить к началу документа (в будущем можно точнее определять позицию)
    QTextCursor leftCursor(m_leftPanel->document());
    leftCursor.movePosition(QTextCursor::Start);
    m_leftPanel->setTextCursor(leftCursor);

    QTextCursor rightCursor(m_rightPanel->document());
    rightCursor.movePosition(QTextCursor::Start);
    m_rightPanel->setTextCursor(rightCursor);
}

void DiffEditor::synchronizeScrollLeftToRight(int value)
{
    if (m_syncingScroll)
        return;
    
    m_syncingScroll = true;
    m_rightPanel->verticalScrollBar()->setValue(value);
    m_syncingScroll = false;
}

void DiffEditor::synchronizeScrollRightToLeft(int value)
{
    if (m_syncingScroll)
        return;
    
    m_syncingScroll = true;
    m_leftPanel->verticalScrollBar()->setValue(value);
    m_syncingScroll = false;
}

void DiffEditor::synchronizeZoom(int zoom)
{
    bool leftBlocked = m_leftPanel->blockSignals(true);
    bool rightBlocked = m_rightPanel->blockSignals(true);

    if (sender() == m_leftPanel)
        m_rightPanel->setZoom(zoom);
    else if (sender() == m_rightPanel)
        m_leftPanel->setZoom(zoom);

    m_leftPanel->blockSignals(leftBlocked);
    m_rightPanel->blockSignals(rightBlocked);
}

void DiffEditor::applySyntaxHighlighting(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    Q_UNUSED(ext);

    m_leftPanel->setSyntaxHighlighting(true);
    m_rightPanel->setSyntaxHighlighting(true);
}
