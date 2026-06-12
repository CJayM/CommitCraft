#include "diffeditor.h"
#include "ui_diffeditor.h"

#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTimer>
#include <QEvent>
#include <QTextBlock>
#include <QSet>
#include <QSettings>
#include <QPixmap>
#include <QDir>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

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

    // Connect partial staging buttons
    connect(ui->stageSelectedButton, &QPushButton::clicked, this, &DiffEditor::onStageSelectedClicked);
    connect(ui->revertSelectedButton, &QPushButton::clicked, this, &DiffEditor::onRevertSelectedClicked);

    // Connect context menu signals from panels
    connect(ui->leftPanel, &DiffPanel::stageSelectedRequested, this, &DiffEditor::onStageSelectedClicked);
    connect(ui->rightPanel, &DiffPanel::stageSelectedRequested, this, &DiffEditor::onStageSelectedClicked);
    connect(ui->leftPanel, &DiffPanel::revertSelectedRequested, this, &DiffEditor::onRevertSelectedClicked);
    connect(ui->rightPanel, &DiffPanel::revertSelectedRequested, this, &DiffEditor::onRevertSelectedClicked);

    // Connect encoding combobox
    connect(ui->encodingComboBox, &QComboBox::activated,
            this, &DiffEditor::onEncodingChanged);

    // --- HunkActionPanel (overlay between left and right panels) ---
    // Создаём как дочерний элемент textDiffPage, а не splitter'а,
    // чтобы быть выше native-детей splitter'а в Z-order
    m_hunkActionPanel = new HunkActionPanel(ui->textDiffPage);
    m_hunkActionPanel->setSourcePanel(ui->rightPanel);
    m_hunkActionPanel->winId(); // принудительно создать HWND
    m_hunkActionPanel->show();

    // Позиционировать панель между leftPanel и rightPanel при изменении splitter
    auto positionActionPanel = [this]() {
        int splitterX = ui->diffSplitter->x();
        int splitterY = ui->diffSplitter->y();
        int leftEdge = splitterX + ui->leftPanel->x() + ui->leftPanel->width();
        m_hunkActionPanel->setGeometry(leftEdge, splitterY, m_hunkActionPanel->width(), ui->diffSplitter->height());
        m_hunkActionPanel->raise();
    };
    positionActionPanel();
    connect(ui->diffSplitter, &QSplitter::splitterMoved, this, positionActionPanel);
    ui->diffSplitter->installEventFilter(this);

    connect(m_hunkActionPanel, &HunkActionPanel::stageHunkRequested,
            this, &DiffEditor::onStageHunkClicked);
    connect(m_hunkActionPanel, &HunkActionPanel::revertHunkRequested,
            this, &DiffEditor::onRevertHunkClicked);

    // Отложенная инициализация позиции (после первого layout)
    QTimer::singleShot(0, this, [this]() {
        int splitterX = ui->diffSplitter->x();
        int splitterY = ui->diffSplitter->y();
        int leftEdge = splitterX + ui->leftPanel->x() + ui->leftPanel->width();
        if (leftEdge > 0) {
            m_hunkActionPanel->setGeometry(leftEdge, splitterY, m_hunkActionPanel->width(), ui->diffSplitter->height());
            m_hunkActionPanel->raise();
        }
    });
}

DiffEditor::~DiffEditor()
{
    delete ui;
}

void DiffEditor::setContentsRaw(const QByteArray &leftData,
                                 const QByteArray &rightData,
                                 const QString &fileName,
                                 const QString &repositoryPath,
                                 const QString &encoding)
{
    // Убираем лишние кавычки если есть
    QString cleanFileName = fileName.trimmed();
    if (cleanFileName.startsWith('"') && cleanFileName.endsWith('"')) {
        cleanFileName = cleanFileName.mid(1, cleanFileName.length() - 2);
    }

    // Store raw bytes for re-encoding
    m_leftRawData = leftData;
    m_rightRawData = rightData;
    m_currentEncoding = encoding;
    m_fileName = cleanFileName;
    m_repositoryPath = repositoryPath;

    // Decode with specified encoding
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec *codec = QTextCodec::codecForName(encoding.toUtf8());
    if (!codec) {
        codec = QTextCodec::codecForName("UTF-8");
    }
    m_leftFullContent = codec->toUnicode(leftData);
    m_rightFullContent = codec->toUnicode(rightData);
#else
    // Qt6: QTextCodec removed from core, using QTextDecoder from Qt5Compat (if available)
    // Fallback: try UTF-8 directly, then Latin-1
    if (encoding == "UTF-8") {
        m_leftFullContent = QString::fromUtf8(leftData);
        m_rightFullContent = QString::fromUtf8(rightData);
    } else if (encoding == "ISO-8859-1" || encoding == "Latin-1") {
        m_leftFullContent = QString::fromLatin1(leftData);
        m_rightFullContent = QString::fromLatin1(rightData);
    } else {
        // For other encodings in Qt6, try UTF-8 as fallback
        m_leftFullContent = QString::fromUtf8(leftData);
        m_rightFullContent = QString::fromUtf8(rightData);
    }
#endif

    
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
        ui->leftPanel->setContent(m_leftFullContent);
        ui->rightPanel->setContent(m_rightFullContent);
    }

    // Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);

    updateButtonsVisibility();
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

    // Store as UTF-8 bytes for potential re-encoding
    m_leftRawData = leftContent.toUtf8();
    m_rightRawData = rightContent.toUtf8();
    m_currentEncoding = "UTF-8";
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

    updateButtonsVisibility();
}

void DiffEditor::setDiffMode(HunkActionPanel::DiffMode mode)
{
    m_hunkActionPanel->setDiffMode(mode);
}

void DiffEditor::setFileIsNew(bool isNew)
{
    m_fileIsNew = isNew;
    m_hunkActionPanel->setFileIsNew(isNew);
}

void DiffEditor::applyDiffData(const QList<Hunk> &hunks)
{
    if (hunks.isEmpty()) {
        // Новый (untracked) файл — показываем одну Stage-кнопку в начале
        if (m_fileIsNew && !m_fileName.isEmpty()) {
            m_currentHunks.clear();
            m_currentHunkIndex = 0;
            m_hunkPositions = {0};
            m_hunkActionPanel->setHunkPositions(m_hunkPositions);
            m_hunkActionPanel->show();
            updateButtonsVisibility();
            repositionActionPanel();
            return;
        }
        ui->leftPanel->clearDiffData();
        ui->rightPanel->clearDiffData();
        m_currentHunks.clear();
        m_currentHunkIndex = -1;
        m_hunkPositions.clear();
        m_hunkActionPanel->clear();
        m_hunkActionPanel->hide();
        updateButtonsVisibility();
        return;
    }

    m_hunkActionPanel->show();
    m_currentHunks = hunks;

    QStringList leftLines = m_leftFullContent.split('\n');
    QStringList rightLines = m_rightFullContent.split('\n');

    QVector<SyncedLine> syncedLines = buildSyncedLines(hunks, leftLines, rightLines);
    m_syncedLines = syncedLines;

    // Сохранить позиции ханков для навигации (индексы в syncedLines).
    // Для каждого ханка ищем первую строку с изменением (зелёный/красный фон),
    // а не контекстную строку.
    m_hunkPositions.clear();
    {
        int currentHunkIdx = -1;
        QSet<int> foundHunkChange;
        for (int i = 0; i < syncedLines.size(); ++i) {
            const auto &sl = syncedLines[i];
            int hIdx = sl.hunkIndex;
            if (hIdx < 0)
                continue; // разделитель
            if (hIdx != currentHunkIdx) {
                currentHunkIdx = hIdx;
                // Новый ханк — ещё не нашли его change-строку
            }
            if (!foundHunkChange.contains(currentHunkIdx)) {
                // Ищем первую строку с изменением (не контекст)
                if (sl.isRemoved || sl.isAdded || sl.isModified) {
                    m_hunkPositions.append(i);
                    foundHunkChange.insert(currentHunkIdx);
                }
            }
        }
        // Если для какого-то ханка не нашли change (только контекст),
        // используем первую строку ханка — подстраховка
        if (m_hunkPositions.size() < syncedLines.size()) {
            currentHunkIdx = -1;
            for (int i = 0; i < syncedLines.size(); ++i) {
                const auto &sl = syncedLines[i];
                if (sl.isSeparator) continue;
                if (sl.hunkIndex != currentHunkIdx) {
                    currentHunkIdx = sl.hunkIndex;
                    if (!foundHunkChange.contains(currentHunkIdx)) {
                        m_hunkPositions.append(i);
                        foundHunkChange.insert(currentHunkIdx);
                    }
                }
            }
        }
    }

    // Передать позиции ханков в HunkActionPanel
    m_hunkActionPanel->setHunkPositions(m_hunkPositions);

    // Обновить позицию панели (размеры могли измениться после загрузки контента)
    repositionActionPanel();

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

    updateButtonsVisibility();
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
    m_currentHunks.clear();
    m_leftFullContent.clear();
    m_rightFullContent.clear();
    m_leftRawData.clear();
    m_rightRawData.clear();
    m_currentEncoding.clear();
    m_fileName.clear();
    m_fileIsNew = false;
    m_hunkActionPanel->clear();
    m_hunkActionPanel->hide();

    // Показать текстовый diff
    ui->stackedWidget->setCurrentWidget(ui->textDiffPage);
    m_isImageFile = false;    

    updateButtonsVisibility();
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

void DiffEditor::updateButtonsVisibility()
{
    bool visible = !m_isImageFile && (!m_currentHunks.isEmpty() || m_fileIsNew);
    ui->stageSelectedButton->setVisible(visible);
    ui->revertSelectedButton->setVisible(visible);
}

void DiffEditor::repositionActionPanel()
{
    int splitterX = ui->diffSplitter->x();
    int splitterY = ui->diffSplitter->y();
    int leftEdge = splitterX + ui->leftPanel->x() + ui->leftPanel->width();
    m_hunkActionPanel->setGeometry(leftEdge, splitterY, m_hunkActionPanel->width(), ui->diffSplitter->height());
    m_hunkActionPanel->raise();
}

QStringList DiffEditor::getSelectedLines() const
{
    QStringList lines;
    QTextCursor cursor = ui->rightPanel->textCursor();  // Use right panel (working tree)
    if (cursor.hasSelection()) {
        QString selectedText = cursor.selectedText();
        lines = selectedText.split('\n', Qt::SkipEmptyParts);
    }
    return lines;
}

QList<int> DiffEditor::getSelectedHunkIndexes() const
{
    QList<int> selectedHunks;
    QTextCursor cursor = ui->rightPanel->textCursor();
    const DiffPanel *panel = ui->rightPanel;
    if (!cursor.hasSelection()) {
        cursor = ui->leftPanel->textCursor();
        panel = ui->leftPanel;
    }
    if (!cursor.hasSelection()) {
        return selectedHunks;
    }

    int startBlock = panel->document()->findBlock(cursor.selectionStart()).blockNumber();
    int endBlock = panel->document()->findBlock(cursor.selectionEnd()).blockNumber();
    if (endBlock < startBlock) {
        qSwap(startBlock, endBlock);
    }

    for (int block = startBlock; block <= endBlock && block < m_syncedLines.size(); ++block) {
        int hunkIndex = m_syncedLines[block].hunkIndex;
        if (hunkIndex >= 0 && !selectedHunks.contains(hunkIndex)) {
            selectedHunks.append(hunkIndex);
        }
    }
    return selectedHunks;
}

QString DiffEditor::buildPatchForHunks(const QString &fileName, const QList<int> &hunkIndexes) const
{
    if (fileName.isEmpty() || hunkIndexes.isEmpty() || m_currentHunks.isEmpty()) {
        return QString();
    }

    QString cleanFileName = fileName;
    if (cleanFileName.startsWith('"') && cleanFileName.endsWith('"')) {
        cleanFileName = cleanFileName.mid(1, cleanFileName.length() - 2);
    }
    cleanFileName.replace('\\', '/');

    QString patch;
    patch += QStringLiteral("diff --git a/%1 b/%1\n").arg(cleanFileName);
    patch += QStringLiteral("--- a/%1\n").arg(cleanFileName);
    patch += QStringLiteral("+++ b/%1\n").arg(cleanFileName);

    for (int idx : hunkIndexes) {
        if (idx < 0 || idx >= m_currentHunks.size()) {
            continue;
        }

        const Hunk &hunk = m_currentHunks[idx];
        patch += QStringLiteral("@@ -%1,%2 +%3,%4 @@").arg(hunk.leftStart).arg(hunk.leftSize).arg(hunk.rightStart).arg(hunk.rightSize);
        if (!hunk.caption.isEmpty()) {
            patch += QStringLiteral(" ") + hunk.caption;
        }
        patch += QStringLiteral("\n");

        for (const HunkLine &line : hunk.lines) {
            QChar prefix = ' ';
            switch (line.type) {
            case HunkLine::Unchanged:
                prefix = ' ';
                break;
            case HunkLine::Removed:
                prefix = '-';
                break;
            case HunkLine::Added:
                prefix = '+';
                break;
            }
            patch += prefix + line.content + QStringLiteral("\n");
        }
    }

    return patch;
}

bool DiffEditor::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->diffSplitter && event->type() == QEvent::Resize) {
        repositionActionPanel();
    }
    return QWidget::eventFilter(obj, event);
}

void DiffEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *stageAction = menu.addAction("Stage Selected Lines");
    QAction *revertAction = menu.addAction("Revert Selected Lines");

    // Подключить сигналы к существующим слотам
    connect(stageAction, &QAction::triggered, this, &DiffEditor::onStageSelectedClicked);
    connect(revertAction, &QAction::triggered, this, &DiffEditor::onRevertSelectedClicked);

    menu.exec(event->globalPos());
}

void DiffEditor::onStageSelectedClicked()
{
    QList<int> selectedHunks = getSelectedHunkIndexes();
    if (selectedHunks.isEmpty()) {
        // No selected hunks, do nothing
        return;
    }

    QString patch = buildPatchForHunks(m_fileName, selectedHunks);
    if (!patch.isEmpty()) {
        emit stageSelectedPatch(m_fileName, patch);
    }
}

void DiffEditor::onRevertSelectedClicked()
{
    QList<int> selectedHunks = getSelectedHunkIndexes();
    if (selectedHunks.isEmpty()) {
        // No selected hunks, do nothing
        return;
    }

    QString patch = buildPatchForHunks(m_fileName, selectedHunks);
    if (!patch.isEmpty()) {
        emit revertSelectedPatch(m_fileName, patch);
    }
}

void DiffEditor::onStageHunkClicked(int hunkIndex)
{
    QList<int> hunks = {hunkIndex};
    QString patch = buildPatchForHunks(m_fileName, hunks);
    if (!patch.isEmpty()) {
        emit stageSelectedPatch(m_fileName, patch);
    } else if (m_fileIsNew) {
        // Новый (untracked) файл — стейджим целиком
        emit stageNewFileRequested(m_fileName);
    }
}

void DiffEditor::onRevertHunkClicked(int hunkIndex)
{
    QList<int> hunks = {hunkIndex};
    QString patch = buildPatchForHunks(m_fileName, hunks);
    if (!patch.isEmpty()) {
        emit revertSelectedPatch(m_fileName, patch);
    }
}

 void DiffEditor::onEncodingChanged(int index)
{
    QString encoding = ui->encodingComboBox->itemText(index);
    // Update the encoding label to reflect the current selection
    m_currentEncoding = encoding;

    // Re-decode raw bytes with new encoding if we have raw data
    if (!m_leftRawData.isEmpty() || !m_rightRawData.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QTextCodec *codec = QTextCodec::codecForName(encoding.toUtf8());
        if (!codec) {
            codec = QTextCodec::codecForUtfText(m_leftRawData);
            if (!codec) {
                codec = QTextCodec::codecForHtml(m_leftRawData);
            }
            if (!codec) {
                codec = QTextCodec::codecForName("UTF-8");
            }
        }

        // Decode left content
        if (!m_leftRawData.isEmpty()) {
            m_leftFullContent = codec->toUnicode(m_leftRawData);
        }

        // Decode right content
        if (!m_rightRawData.isEmpty()) {
            m_rightFullContent = codec->toUnicode(m_rightRawData);
        }
#else
        // Qt6: QTextCodec removed from core, using QTextDecoder from Qt5Compat (if available)
        // Fallback: try UTF-8 directly, then Latin-1
        if (encoding == "UTF-8") {
            if (!m_leftRawData.isEmpty())
                m_leftFullContent = QString::fromUtf8(m_leftRawData);
            if (!m_rightRawData.isEmpty())
                m_rightFullContent = QString::fromUtf8(m_rightRawData);
        } else if (encoding == "ISO-8859-1" || encoding == "Latin-1") {
            if (!m_leftRawData.isEmpty())
                m_leftFullContent = QString::fromLatin1(m_leftRawData);
            if (!m_rightRawData.isEmpty())
                m_rightFullContent = QString::fromLatin1(m_rightRawData);
        } else {
            // For other encodings in Qt6, try UTF-8 as fallback
            if (!m_leftRawData.isEmpty())
                m_leftFullContent = QString::fromUtf8(m_leftRawData);
            if (!m_rightRawData.isEmpty())
                m_rightFullContent = QString::fromUtf8(m_rightRawData);
        }
#endif

        // Re-apply content to panels
        ui->leftPanel->setPlainText(m_leftFullContent);
        ui->rightPanel->setPlainText(m_rightFullContent);

        // Re-apply diff data if available
        if (!m_currentHunks.isEmpty()) {
            applyDiffData(m_currentHunks);
        }
    }
}
