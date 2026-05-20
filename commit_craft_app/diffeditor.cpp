#include "diffeditor.h"
#include "ui_diffeditor.h"

#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTextBlock>
#include <QSet>
#include <QSettings>
#include <QPixmap>
#include <QDir>

DiffEditor::DiffEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DiffEditor)
    , m_isImageFile(false)
    , m_currentHunkIndex(-1)
    , m_syncingScroll(false)
{
    ui->setupUi(this);

    // Загрузить настройки расширений
    QSettings settings("CommitCraft", "AppSettings");
    QStringList defaultImageExts = {"png", "jpg", "jpeg", "gif", "bmp", "svg", "ico", "webp", "tiff"};
    QStringList defaultSyntaxExts = {"cpp", "h", "hpp", "c", "java", "py", "js", "ts", "html", "css",
                                     "xml", "json", "yaml", "yml", "md", "sh", "bat", "ps1", "go", "rs"};
    m_imageExtensions = settings.value("imageExtensions", defaultImageExts).toStringList();
    m_syntaxExtensions = settings.value("syntaxExtensions", defaultSyntaxExts).toStringList();

    // Подключить синхронизацию скролла
    connect(ui->leftPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollLeftToRight);
    connect(ui->rightPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollRightToLeft);

    // Подключить синхронизацию зума (через CodeEditor)
    connect(ui->leftPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);
    connect(ui->rightPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);

    // Подключить синхронизацию подсветки текущей строки (перемещение курсора)
    connect(ui->leftPanel, &DiffPanel::panelCursorMoved, this, [this](int blockNum) {
        ui->rightPanel->setCursorToLine(blockNum);
    });
    connect(ui->rightPanel, &DiffPanel::panelCursorMoved, this, [this](int blockNum) {
        ui->leftPanel->setCursorToLine(blockNum);
    });
}

DiffEditor::~DiffEditor()
{
    delete ui;
}

void DiffEditor::setContents(const QString &leftContent,
                              const QString &rightContent,
                              const QString &fileName,
                              const QString &repositoryPath)
{
    // Убираем лишние кавычки если есть
    QString cleanFileName = fileName.trimmed();
    if (cleanFileName.startsWith('"') && cleanFileName.endsWith('"')) {
        cleanFileName = cleanFileName.mid(1, cleanFileName.length() - 2);
    }

    m_leftFullContent = leftContent;
    m_rightFullContent = rightContent;
    m_fileName = cleanFileName;
    m_repositoryPath = repositoryPath;

    // Очистить предыдущие diff-данные
    ui->leftPanel->clearDiffData();
    ui->rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;

    // Проверить тип файла
    bool isImage = checkFileType(cleanFileName);

    if (isImage) {
        // Для графических файлов - показываем изображение
        ui->stackedWidget->setCurrentWidget(ui->imagePreviewPage);
        
        // Очищаем старые данные
        ui->imageDisplayLabel->clear();
        ui->imageInfoLabel->clear();
        
        // Пытаемся загрузить изображение
        QPixmap pixmap;
        QFileInfo fi(cleanFileName);
        QString fullPath;
        
        if (fi.isAbsolute()) {
            fullPath = cleanFileName;
        } else if (!m_repositoryPath.isEmpty()) {
            // Используем QDir для корректной сборки пути
            QDir dir(m_repositoryPath);
            fullPath = dir.filePath(cleanFileName);
            // Преобразуем в нативный формат для Windows
            fullPath = QDir::toNativeSeparators(fullPath);
        } else {
            fullPath = QDir::current().absoluteFilePath(cleanFileName);
        }

        // Для Qt 6 используем load с QString напрямую
        if (QFile::exists(fullPath)) {
            pixmap.load(fullPath);
        }
        
        if (!pixmap.isNull()) {
            // Показываем изображение
            ui->imageDisplayLabel->setPixmap(pixmap);
            
            // Информация о файле
            QString sizeText = QString("%1 x %2 px").arg(pixmap.width()).arg(pixmap.height());
            QString infoText = QString("<b>%1</b><br>%2: %3")
                                    .arg(cleanFileName,
                                         QStringLiteral("Размер изображения"),
                                         sizeText);
            ui->imageInfoLabel->setText(infoText);
        } else {
            ui->imageInfoLabel->setText(QStringLiteral("Не удалось загрузить изображение: %1<br>Путь: %2").arg(cleanFileName, fullPath));
        }
    } else {
        // Для текстовых файлов - используем обычный diff
        ui->stackedWidget->setCurrentWidget(ui->textDiffPage);
        
        // Показать полное содержимое (будет заменено на side-by-side diff в applyDiffData)
        ui->leftPanel->setContent(leftContent);
        ui->rightPanel->setContent(rightContent);
    }

    // Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);
}

void DiffEditor::applyDiffData(const QList<Hunk> &hunks)
{
    if (hunks.isEmpty()) {
        ui->leftPanel->clearDiffData();
        ui->rightPanel->clearDiffData();
        return;
    }

    QStringList leftLines = m_leftFullContent.split('\n');
    QStringList rightLines = m_rightFullContent.split('\n');

    QVector<SyncedLine> syncedLines = buildSyncedLines(hunks, leftLines, rightLines);

    // Сохранить позиции ханков для навигации (индексы в syncedLines)
    m_hunkPositions.clear();
    int currentHunkIdx = -1;
    for (int i = 0; i < syncedLines.size(); ++i) {
        const auto &sl = syncedLines[i];
        if (!sl.isSeparator && sl.hunkIndex != currentHunkIdx) {
            // Нашли первую строку нового ханка
            currentHunkIdx = sl.hunkIndex;
            m_hunkPositions.append(i);
        }
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
    ui->leftPanel->setLineNumbers(leftNums);
    ui->rightPanel->setLineNumbers(rightNums);

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

        // Обработка разделителей чанков
        if (sl.isSeparator) {
            leftDiffMap[i] = {DiffType::Separator, i, {}};
            rightDiffMap[i] = {DiffType::Separator, i, {}};
        }
    }

    ui->leftPanel->setDiffData(leftDiffMap);
    ui->rightPanel->setDiffData(rightDiffMap);
    ui->leftPanel->setPlaceholderLines(leftPlaceholders);
    ui->rightPanel->setPlaceholderLines(rightPlaceholders);
}

void DiffEditor::applyFontSettings(const QString &fontFamily, int fontSize)
{
    QFont font(fontFamily, fontSize);
    ui->leftPanel->setFont(font);
    ui->rightPanel->setFont(font);

    // Сброс зума на 100% чтобы избежать конфликта с новым размером шрифта
    ui->leftPanel->setZoom(100);
    ui->rightPanel->setZoom(100);
}

bool DiffEditor::isImageFile() const
{
    return m_isImageFile;
}

bool DiffEditor::isSyntaxFile() const
{
    return !m_isImageFile && !m_syntaxExtensions.isEmpty();
}

void DiffEditor::updateFileTypeSettings()
{
    QSettings settings("CommitCraft", "AppSettings");
    QStringList defaultImageExts = {"png", "jpg", "jpeg", "gif", "bmp", "svg", "ico", "webp", "tiff"};
    QStringList defaultSyntaxExts = {"cpp", "h", "hpp", "c", "java", "py", "js", "ts", "html", "css",
                                     "xml", "json", "yaml", "yml", "md", "sh", "bat", "ps1", "go", "rs"};
    m_imageExtensions = settings.value("imageExtensions", defaultImageExts).toStringList();
    m_syntaxExtensions = settings.value("syntaxExtensions", defaultSyntaxExts).toStringList();
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

        // Если есть разрыв между предыдущим чанком и текущим, добавляем разделитель
        if (hunkIdx > 0 && (contextStartLeft > prevLeftEnd || contextStartRight > prevRightEnd)) {
            result.append({
                "", "",                 // empty text
                false, false, false, false, // flags
                false, false,           // not placeholders
                -1, -1,                 // line nums
                -1,                     // hunkIndex
                false,                  // isHunkBoundary
                true                    // isSeparator
            });
        }

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

        if (sl.isSeparator) {
            // Текст-разделитель между чанками
            leftText += "...";
            rightText += "...";
        } else {
            leftText += sl.leftText;
            rightText += sl.rightText;
        }

        if (i < syncedLines.size() - 1) {
            leftText += '\n';
            rightText += '\n';
        }
    }

    ui->leftPanel->setContent(leftText);
    ui->rightPanel->setContent(rightText);
}

void DiffEditor::clear()
{
    ui->leftPanel->setPlainText("");
    ui->rightPanel->setPlainText("");
    ui->leftPanel->clearDiffData();
    ui->rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;
    m_leftFullContent.clear();
    m_rightFullContent.clear();
    m_fileName.clear();
    
    // Показать текстовый diff
    ui->stackedWidget->setCurrentWidget(ui->textDiffPage);
    m_isImageFile = false;
}

bool DiffEditor::navigateToNextHunk()
{
    if (m_hunkPositions.isEmpty())
        return false;

    if (m_currentHunkIndex >= m_hunkPositions.size() - 1)
        return false; // Достигнут конец, не зацикливаем

    m_currentHunkIndex++;
    scrollToHunk(m_currentHunkIndex);
    emit hunkNavigated(m_currentHunkIndex);
    return true;
}

bool DiffEditor::navigateToPrevHunk()
{
    if (m_hunkPositions.isEmpty())
        return false;

    if (m_currentHunkIndex <= 0)
        return false; // Достигнуто начало, не зацикливаем

    m_currentHunkIndex--;
    scrollToHunk(m_currentHunkIndex);
    emit hunkNavigated(m_currentHunkIndex);
    return true;
}

void DiffEditor::navigateToFirstHunk()
{
    if (m_hunkPositions.isEmpty())
        return;
    m_currentHunkIndex = 0;
    scrollToHunk(0);
    emit hunkNavigated(0);
}

void DiffEditor::navigateToLastHunk()
{
    if (m_hunkPositions.isEmpty())
        return;
    m_currentHunkIndex = m_hunkPositions.size() - 1;
    scrollToHunk(m_currentHunkIndex);
    emit hunkNavigated(m_currentHunkIndex);
}

void DiffEditor::scrollToHunk(int hunkIndex)
{
    if (hunkIndex < 0 || hunkIndex >= m_hunkPositions.size())
        return;

    int lineIndex = m_hunkPositions[hunkIndex];

    // Прокрутить к строке ханка
    QTextBlock leftBlock = ui->leftPanel->document()->findBlockByLineNumber(lineIndex);
    if (leftBlock.isValid()) {
        QTextCursor leftCursor(leftBlock);
        ui->leftPanel->setTextCursor(leftCursor);
        ui->leftPanel->ensureCursorVisible();
    }

    QTextBlock rightBlock = ui->rightPanel->document()->findBlockByLineNumber(lineIndex);
    if (rightBlock.isValid()) {
        QTextCursor rightCursor(rightBlock);
        ui->rightPanel->setTextCursor(rightCursor);
        ui->rightPanel->ensureCursorVisible();
    }
}

void DiffEditor::synchronizeScrollLeftToRight(int value)
{
    if (m_syncingScroll)
        return;

    m_syncingScroll = true;
    ui->rightPanel->verticalScrollBar()->setValue(value);
    m_syncingScroll = false;
}

void DiffEditor::synchronizeScrollRightToLeft(int value)
{
    if (m_syncingScroll)
        return;

    m_syncingScroll = true;
    ui->leftPanel->verticalScrollBar()->setValue(value);
    m_syncingScroll = false;
}

void DiffEditor::synchronizeZoom(int zoom)
{
    bool leftBlocked = ui->leftPanel->blockSignals(true);
    bool rightBlocked = ui->rightPanel->blockSignals(true);

    if (sender() == ui->leftPanel)
        ui->rightPanel->setZoom(zoom);
    else if (sender() == ui->rightPanel)
        ui->leftPanel->setZoom(zoom);

    ui->leftPanel->blockSignals(leftBlocked);
    ui->rightPanel->blockSignals(rightBlocked);
}

void DiffEditor::applySyntaxHighlighting(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    
    bool useSyntax = m_syntaxExtensions.contains(ext);
    ui->leftPanel->setSyntaxHighlighting(useSyntax);
    ui->rightPanel->setSyntaxHighlighting(useSyntax);
}

bool DiffEditor::checkFileType(const QString &fileName)
{
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();
    
    m_isImageFile = m_imageExtensions.contains(ext);
    return m_isImageFile;
}
