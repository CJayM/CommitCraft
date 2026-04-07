#include "diffeditor.h"

#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>

DiffEditor::DiffEditor(QWidget *parent)
    : QWidget(parent)
    , m_leftPanel(new DiffPanel(this))
    , m_rightPanel(new DiffPanel(this))
    , m_currentHunkIndex(-1)
    , m_layout(new QVBoxLayout(this))
    , m_splitter(new QSplitter(Qt::Horizontal, this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Добавить панели в splitter
    m_splitter->addWidget(m_leftPanel);
    m_splitter->addWidget(m_rightPanel);

    m_layout->addWidget(m_splitter);

    // Начальное равное распределение
    // Будет пересчитано при показе
    setLayout(m_layout);

    // Подключить синхронизацию скролла
    connect(m_leftPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollLeftToRight);
    connect(m_rightPanel->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DiffEditor::synchronizeScrollRightToLeft);

    // Подключить синхронизацию зума (через CodeEditor)
    connect(m_leftPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);
    connect(m_rightPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);
}

void DiffEditor::setContents(const QString &leftContent,
                              const QString &rightContent,
                              const QString &fileName)
{
    // 1. Установить содержимое в панели
    m_leftPanel->setContent(leftContent);
    m_rightPanel->setContent(rightContent);

    // 2. Очистить предыдущие diff-данные
    m_leftPanel->clearDiffData();
    m_rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;

    // 3. Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);

    // 4. diff-данные будут применены позже через applyDiffData()
    //    когда Git завершит обработку
}

void DiffEditor::applyDiffData(const QList<Hunk> &hunks)
{
    // 1. Строим LineDiffMap для каждой панели
    auto leftMap = DiffHighlighter::buildLineDiffMapLeft(hunks);
    auto rightMap = DiffHighlighter::buildLineDiffMapRight(hunks);

    // 2. Для Modified строк вычисляем intra-line diff
    DiffHighlighter::computeIntraLineDiffs(leftMap, rightMap, hunks);

    // 3. Сохраняем ханки для навигации
    m_hunkPositions.clear();
    for (const auto &hunk : hunks) {
        m_hunkPositions.append(qMakePair(hunk.leftStart, hunk.rightStart));
    }

    // 4. Применяем данные к панелям
    m_leftPanel->setDiffData(leftMap);
    m_rightPanel->setDiffData(rightMap);

    // 5. Обновить подсветку синтаксиса (diff-фоны могут перекрыть)
    //    Highlighter уже установлен в CodeEditor, перерисовка произойдёт автоматически
}

void DiffEditor::clear()
{
    m_leftPanel->setPlainText("");
    m_rightPanel->setPlainText("");
    m_leftPanel->clearDiffData();
    m_rightPanel->clearDiffData();
    m_hunkPositions.clear();
    m_currentHunkIndex = -1;
}

void DiffEditor::navigateToNextHunk()
{
    if (m_hunkPositions.isEmpty())
        return;

    m_currentHunkIndex++;
    if (m_currentHunkIndex >= m_hunkPositions.size())
        m_currentHunkIndex = 0; // Wrap around

    const auto &pos = m_hunkPositions[m_currentHunkIndex];
    int leftLine = pos.first - 1;  // 1-based → 0-based
    int rightLine = pos.second - 1;

    // Прокрутить левую панель к ханку
    QTextCursor leftCursor(m_leftPanel->document());
    leftCursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < leftLine && leftCursor.movePosition(QTextCursor::Down); ++i) {}
    m_leftPanel->setTextCursor(leftCursor);

    // Прокрутить правую панель к ханку
    QTextCursor rightCursor(m_rightPanel->document());
    rightCursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < rightLine && rightCursor.movePosition(QTextCursor::Down); ++i) {}
    m_rightPanel->setTextCursor(rightCursor);

    emit hunkNavigated(m_currentHunkIndex);
}

void DiffEditor::navigateToPrevHunk()
{
    if (m_hunkPositions.isEmpty())
        return;

    m_currentHunkIndex--;
    if (m_currentHunkIndex < 0)
        m_currentHunkIndex = m_hunkPositions.size() - 1; // Wrap around

    const auto &pos = m_hunkPositions[m_currentHunkIndex];
    int leftLine = pos.first - 1;
    int rightLine = pos.second - 1;

    QTextCursor leftCursor(m_leftPanel->document());
    leftCursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < leftLine && leftCursor.movePosition(QTextCursor::Down); ++i) {}
    m_leftPanel->setTextCursor(leftCursor);

    QTextCursor rightCursor(m_rightPanel->document());
    rightCursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < rightLine && rightCursor.movePosition(QTextCursor::Down); ++i) {}
    m_rightPanel->setTextCursor(rightCursor);

    emit hunkNavigated(m_currentHunkIndex);
}

void DiffEditor::synchronizeScrollLeftToRight(int value)
{
    QScrollBar *leftBar = m_leftPanel->verticalScrollBar();
    QScrollBar *rightBar = m_rightPanel->verticalScrollBar();

    bool blocked = rightBar->blockSignals(true);

    int leftRange = leftBar->maximum() - leftBar->minimum();
    int rightRange = rightBar->maximum() - rightBar->minimum();

    if (leftRange > 0 && rightRange > 0) {
        double percent = double(value - leftBar->minimum()) / leftRange;
        int newValue = rightBar->minimum() + int(percent * rightRange);
        rightBar->setValue(newValue);
    }

    rightBar->blockSignals(blocked);
}

void DiffEditor::synchronizeScrollRightToLeft(int value)
{
    QScrollBar *leftBar = m_leftPanel->verticalScrollBar();
    QScrollBar *rightBar = m_rightPanel->verticalScrollBar();

    bool blocked = leftBar->blockSignals(true);

    int leftRange = leftBar->maximum() - leftBar->minimum();
    int rightRange = rightBar->maximum() - rightBar->minimum();

    if (leftRange > 0 && rightRange > 0) {
        double percent = double(value - rightBar->minimum()) / rightRange;
        int newValue = leftBar->minimum() + int(percent * leftRange);
        leftBar->setValue(newValue);
    }

    leftBar->blockSignals(blocked);
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
    // DiffPanel уже содержит Highlighter через CodeEditor.
    // Существующий Highlighter настроен для C++/Qt.
    // Для других языков можно расширить Highlighter или создать фабрику (Шаг 6).
    QFileInfo fi(fileName);
    QString ext = fi.suffix().toLower();

    // Отключить подсветку для файлов без подсветки
    // Пока оставляем C++ подсветку для всех текстовых файлов
    Q_UNUSED(ext);

    // Переподсветить обе панели
    m_leftPanel->setSyntaxHighlighting(true);
    m_rightPanel->setSyntaxHighlighting(true);
}
