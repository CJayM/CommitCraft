# План реализации Side-by-Side Diff Editor с подсветкой синтаксиса

## Обзор

Заменить текущие `stagedContentTextEdit` и `currentContentTextEdit` (обычные `QPlainTextEdit`) на специализированный виджет **DiffEditor**, который отображает две панели рядом (staged vs current) с:
- **Красный фон** — удалённые строки
- **Зелёный фон** — добавленные строки
- **Синий фон** — модифицированные строки (intra-line diff)
- **Синхронный скролл** между левой и правой панелями
- **Подсветка синтаксиса** с учётом типа файла (по расширению)

По образцу SmartGit / Meld.

---

## Архитектура

```
DiffEditor (QWidget)
├── DiffPanel* leftPanel       ← staged версия (или HEAD), наследуется от CodeEditor
├── DiffPanel* rightPanel      ← current версия (рабочая директория), наследуется от CodeEditor
├── DiffHighlighter* diffHighlighter  ← один на обе панели, хранит LineDiffInfo
├── Highlighter* leftSyntaxHighlighter   ← подсветка синтаксиса (уже есть в проекте)
├── Highlighter* rightSyntaxHighlighter
└── (номера строк и зум — наследуются от CodeEditor)

DiffPanel : public CodeEditor
├── Переопределённый paintEvent (рисуем фоны ПОВЕРХ текста)
└── QMap<int, LineDiffInfo> m_lineDiffMap  ← diff-данные

LineDiffInfo (struct)
├── DiffType type              ← Unchanged, Added, Removed, Modified
├── int lineNumber             ← номер строки в панели (0-based)
└── QVector<QPair<int, int>> changedRanges  ← (start, length) для intra-line
```

> **Важно:** `DiffPanel` наследуется от **CodeEditor**, а не от `QPlainTextEdit`. Это даёт:
> - Готовые номера строк (`LineNumberArea`)
> - Готовый зум по Ctrl+колесо (`zoomIn/zoomOut/setZoom`)
> - Готовую подсветку синтаксиса (`Highlighter`)
> - Отключённый word wrap

---

## Шаг 1: Создание файлов нового виджета

Создать в `commit_craft_app/`:

| Файл | Назначение |
|---|---|
| `diffeditor.h` | Объявление класса `DiffEditor` |
| `diffeditor.cpp` | Реализация `DiffEditor` |
| `diffpanel.h` | Объявление класса `DiffPanel` (наследуется от CodeEditor) |
| `diffpanel.cpp` | Реализация `DiffPanel` с переопределённым `paintEvent` |
| `diffhighlighter.h` | Объявление `DiffHighlighter` (адаптер GitParser → LineDiffInfo) |
| `diffhighlighter.cpp` | Реализация `DiffHighlighter` |
| `intralinediff.h` | LCS-алгоритм для intra-line подсветки |
| `intralinediff.cpp` | Реализация `IntraLineDiffCalculator` |

Добавить в `commit_craft_app.pro`:

```qmake
SOURCES += \
    codeeditor.cpp \
    diffeditor.cpp \
    diffpanel.cpp \
    diffhighlighter.cpp \
    intralinediff.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    syntaxhighlighter.cpp

HEADERS += \
    codeeditor.h \
    diffeditor.h \
    diffpanel.h \
    diffhighlighter.h \
    intralinediff.h \
    linenumberarea.h \
    mainwindow.h \
    settingsdialog.h \
    syntaxhighlighter.h
```

---

## Шаг 2: Использование GitParser (не пишем diff с нуля!)

### 2.1 НЕ реализуем свой diff-алгоритм

**Git уже делает diff за нас.** Команда `git diff <файл>` возвращает unified diff, который **уже парсится** существующим `GitParser` из `commit_craft_lib`.

Структуры `Hunk` и `HunkLine` уже содержат всю необходимую информацию:

```cpp
// Уже есть в gitparser.h
struct HunkLine {
    enum Type { Unchanged = 0, Removed = 1, Added = 2 };
    Type type;
    QString content;
};

struct Hunk {
    int leftStart, rightStart;
    int leftSize, rightSize;
    QList<HunkLine> lines;  // ← каждая строка: добавлена, удалена или неизменна
};
```

`GitParser::parseDiffOutput()` уже вызывается в `MainWindow::extractHunkPositions()`.

### 2.2 Адаптация данных: Hunk → LineDiffInfo

Нужно преобразовать `QList<Hunk>` (из GitParser) в формат, удобный для отрисовки в DiffPanel:

```cpp
// diffhighlighter.h

enum class DiffType {
    Unchanged,
    Added,       // зелёный фон  (HunkLine::Added)
    Removed,     // красный фон   (HunkLine::Removed)
    Modified     // синий фон     (intra-line diff, см. шаг 2.3)
};

struct LineDiffInfo {
    DiffType type;
    int lineNumber;              // номер строки в панели (0-based)
    // Для intra-line подсветки (Modified)
    QVector<QPair<int, int>> changedRanges; // (start, length) символов
};

// Преобразование Hunk → LineDiffInfo для каждой панели
QMap<int, LineDiffInfo> buildLineDiffMapLeft(const QList<Hunk> &hunks);
QMap<int, LineDiffInfo> buildLineDiffMapRight(const QList<Hunk> &hunks);
```

**Алгоритм `buildLineDiffMapLeft`:**
1. Пройти по всем ханкам и их `HunkLine`
2. Для каждой строки вычислить номер строки в **левой** панели (используя `hunk.leftStart` + смещение)
3. Определить тип:
   - `HunkLine::Removed` → `DiffType::Removed`
   - `HunkLine::Added` → строка не отображается в левой панели (пропуск)
   - `HunkLine::Unchanged` → `DiffType::Unchanged`
4. **Обнаружение Modified:** если подряд идут `Removed` → `Added` (в том же ханке) — проверить схожесть строк. Если схожесть >50%, пометить обе как `DiffType::Modified`

Аналогично для `buildLineDiffMapRight`, но `Added` строки отображаются, а `Removed` — нет.

### 2.3 Intra-line diff (для синего фона — модификации внутри строки)

**Важно:** unified diff от git **не содержит** информации о том, какие именно символы внутри строки изменились — только что строка удалена/добавлена.

Для строк, помеченных как `Modified`, нужно вычислить **посимвольный diff** — какие конкретные символы изменились:

```cpp
// intralinediff.h
class IntraLineDiffCalculator {
public:
    // Вычислить диапазоны изменённых символов
    // Возвращает (removedRanges для левой строки, addedRanges для правой)
    static QPair<QVector<QPair<int,int>>, QVector<QPair<int,int>>>
        computeChangedRanges(const QString &oldLine, const QString &newLine);

private:
    // LCS (Longest Common Subsequence) на уровне символов
    static QVector<QPair<int,int>> findCommonSubsequences(const QString &a, const QString &b);
};
```

**Оптимизация:** для длинных строк (>200 символов) использовать diff на уровне **слов**, а не символов, чтобы не тормозить.

### 2.4 Полный асинхронный пайплайн

```
DiffEditor::requestDiffHighlighting(fileName)
    │
    ▼
git->getDiff(fileName)   ← асинхронный вызов (QProcess)
    │
    │  (сигнал Git::diffReady)
    ▼
DiffEditor::onGitDiffReady(diffOutput)
    │
    ├── GitParser::parseDiffOutput(diffOutput)
    │       │
    │       ▼
    │   QList<Hunk> hunks
    │       │
    │       ▼
    │   buildLineDiffMapLeft(hunks)  → QMap<int, LineDiffInfo>
    │   buildLineDiffMapRight(hunks) → QMap<int, LineDiffInfo>
    │       │
    │       ▼
    │   Для Modified строк: IntraLineDiffCalculator::computeChangedRanges()
    │       │
    │       ▼
    │   diffHighlighter->setDiffData(leftMap, rightMap)
    │       │
    │       ▼
    │   leftPanel->setDiffData(leftMap)
    │   rightPanel->setDiffData(rightMap)
    │       │
    │       ▼
    │   leftPanel->update()   ← перерисовка с фонами
    │   rightPanel->update()
    ▼
(пользователь видит подсвеченный diff)
```

---

## Шаг 3: Реализация DiffPanel

`DiffPanel` — виджет, отображающий одну сторону diff. **Наследуется от CodeEditor**, а не от `QPlainTextEdit`.

### 3.1 Объявление

```cpp
// diffpanel.h
#include "codeeditor.h"

class DiffPanel : public CodeEditor {
    Q_OBJECT

public:
    explicit DiffPanel(QWidget *parent = nullptr);

    // Установка содержимого
    void setContent(const QString &text);

    // Установка diff-данных (вызывается после setContent)
    void setDiffData(const QMap<int, LineDiffInfo> &lineDiffMap);

    // Переопределение paintEvent: сначала текст, потом фоны ПОВЕРХ
    void paintEvent(QPaintEvent *event) override;

private:
    void drawLineBackgrounds(QPainter &painter);
    void drawIntraLineHighlights(QPainter &painter);

    QMap<int, LineDiffInfo> m_lineDiffMap;
};
```

> **Почему CodeEditor, а не QPlainTextEdit?**
> - Уже есть `LineNumberArea` — номера строк с серым фоном
> - Уже есть `zoomIn/zoomOut/setZoom` + сигнал `zoomChanged`
> - Уже есть `Highlighter` — подсветка синтаксиса C++
> - Уже есть `setLineWrapMode(NoWrap)` — перенос строк отключён
> - Не нужно дублировать код номеров строк, зума и syntax highlighting

### 3.2 paintEvent — правильный порядок рисования

**Ключевой момент:** `QPlainTextEdit::paintEvent` рисует текст на viewport. Если рисовать фоны **до** него — фоны будут затёрты. Поэтому порядок:

1. Вызываем `QPlainTextEdit::paintEvent` (рисуем текст)
2. Рисуем фоны строк **поверх** текста (полупрозрачные цвета)
3. Рисуем intra-line подсветку **поверх** фонов

```cpp
// diffpanel.cpp
void DiffPanel::paintEvent(QPaintEvent *event)
{
    // 1. Сначала стандартная отрисовка (текст, курсор, selection)
    CodeEditor::paintEvent(event);

    // 2. Поверх текста — фоны строк
    QPainter painter(viewport());
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    drawLineBackgrounds(painter);
    drawIntraLineHighlights(painter);
}
```

### 3.3 Отрисовка фона строк

```cpp
void DiffPanel::drawLineBackgrounds(QPainter &painter)
{
    // Цвета фонов
    const QColor removedColor(255, 200, 200);  // красный (удалено)
    const QColor addedColor(200, 255, 200);    // зелёный (добавлено)
    const QColor modifiedColor(180, 200, 255); // синий (модифицировано)

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());

    while (block.isValid() && top <= viewport()->height()) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            block = block.next();
            blockNumber++;
            continue;
        }

        int bottom = top + qRound(blockBoundingRect(block).height());

        // Найти diff-тип для этой строки
        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end()) {
            QColor bgColor;
            switch (it->type) {
                case DiffType::Removed:  bgColor = removedColor; break;
                case DiffType::Added:    bgColor = addedColor;   break;
                case DiffType::Modified: bgColor = modifiedColor; break;
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
```

### 3.4 Intra-line подсветка (тёмно-синие фрагменты на Modified строках)

```cpp
void DiffPanel::drawIntraLineHighlights(QPainter &painter)
{
    const QColor intraColor(140, 170, 255, 120); // полупрозрачный синий

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());

    while (block.isValid() && top <= viewport()->height()) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            block = block.next();
            blockNumber++;
            continue;
        }

        auto it = m_lineDiffMap.find(blockNumber);
        if (it != m_lineDiffMap.end() && it->type == DiffType::Modified) {
            for (const auto &range : it->changedRanges) {
                // Получить QRect для диапазона символов
                QTextCursor cursor(document());
                cursor.setPosition(block.position() + range.first);
                QRect startRect = cursorRect(cursor);

                cursor.setPosition(block.position() + range.first + range.second);
                QRect endRect = cursorRect(cursor);

                QRect highlightRect(startRect.left(), top,
                                   endRect.right() - startRect.left(),
                                   qRound(blockBoundingRect(block).height()));

                painter.fillRect(highlightRect, intraColor);
            }
        }

        top += qRound(blockBoundingRect(block).height());
        block = block.next();
        blockNumber++;
    }
}
```

---

## Шаг 4: Реализация DiffEditor

### 4.1 Объявление

```cpp
// diffeditor.h
class DiffEditor : public QWidget {
    Q_OBJECT

public:
    explicit DiffEditor(QWidget *parent = nullptr);

    // Установка содержимого для сравнения
    void setContents(const QString &leftContent,
                     const QString &rightContent,
                     const QString &fileName);

    // Очистка
    void clear();

signals:
    void hunkNavigated(int hunkIndex);

public slots:
    void navigateToNextHunk();
    void navigateToPrevHunk();
    int currentHunkIndex() const;
    int hunkCount() const;

private slots:
    void onGitDiffReady(const QString &diffOutput);
    void synchronizeScrollLeftToRight(int value);
    void synchronizeScrollRightToLeft(int value);
    void synchronizeZoom(int zoom);

private:
    void requestDiffHighlighting(const QString &fileName);
    void applyDiffData(const QList<Hunk> &hunks);
    void applySyntaxHighlighting(const QString &fileName);

    DiffPanel *m_leftPanel;
    DiffPanel *m_rightPanel;
    DiffHighlighter *m_diffHighlighter;

    // Syntax highlighters (переиспользуем существующий Highlighter)
    Highlighter *m_leftSyntaxHighlighter;
    Highlighter *m_rightSyntaxHighlighter;

    // Навигация по ханкам
    QList<QPair<int, int>> m_hunkPositions; // (leftLine, rightLine)
    int m_currentHunkIndex;
};
```

### 4.2 Метод setContents

```cpp
void DiffEditor::setContents(const QString &leftContent,
                              const QString &rightContent,
                              const QString &fileName)
{
    // 1. Установить содержимое в панели
    m_leftPanel->setContent(leftContent);
    m_rightPanel->setContent(rightContent);

    // 2. Запросить diff через Git (асинхронно)
    requestDiffHighlighting(fileName);

    // 3. Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);

    // 4. Сбросить навигацию по ханкам
    m_currentHunkIndex = -1;
    m_hunkPositions.clear();
}
```

### 4.3 Асинхронная обработка diff

```cpp
void DiffEditor::requestDiffHighlighting(const QString &fileName)
{
    // Очистить предыдущие diff-данные
    m_leftPanel->setDiffData({});
    m_rightPanel->setDiffData({});

    // Запустить git diff (асинхронно, через Git из MainWindow)
    // В реальности это делается через сигнал Git::diffReady
    // DiffEditor должен получить ссылатель на Git или сигнал
    // Вариант: MainWindow подключает git->diffReady к DiffEditor::onGitDiffReady
}

void DiffEditor::onGitDiffReady(const QString &diffOutput)
{
    // 1. Парсим diff через существующий GitParser
    GitParser parser;
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);

    // 2. Преобразуем и применяем diff-данные
    applyDiffData(hunks);
}

void DiffEditor::applyDiffData(const QList<Hunk> &hunks)
{
    // 1. Строим LineDiffMap для каждой панели
    auto leftMap = m_diffHighlighter->buildLineDiffMapLeft(hunks);
    auto rightMap = m_diffHighlighter->buildLineDiffMapRight(hunks);

    // 2. Для Modified строк вычисляем intra-line diff
    m_diffHighlighter->computeIntraLineDiffs(leftMap, rightMap, hunks);

    // 3. Сохраняем ханки для навигации
    m_hunkPositions.clear();
    for (const auto &hunk : hunks) {
        m_hunkPositions.append(qMakePair(hunk.leftStart, hunk.rightStart));
    }

    // 4. Применяем данные к панелям
    m_leftPanel->setDiffData(leftMap);
    m_rightPanel->setDiffData(rightMap);

    // 5. Перерисовка
    m_leftPanel->update();
    m_rightPanel->update();
}
```

> **Примечание:** `DiffEditor` не владеет объектом `Git`. Связь `Git::diffReady → DiffEditor::onGitDiffReady` устанавливается в `MainWindow`. `DiffEditor` лишь предоставляет слот для приёма данных.

### 4.4 Подсветка синтаксиса по типу файла

```cpp
void DiffEditor::applySyntaxHighlighting(const QString &fileName)
{
    // Определить тип файла по расширению
    QString ext = QFileInfo(fileName).suffix().toLower();

    // Существующий Highlighter уже настроен для C++
    // Для других языков — либо расширить Highlighter, либо создать фабрику
    // (см. Шаг 6)

    m_leftSyntaxHighlighter->rehighlight();
    m_rightSyntaxHighlighter->rehighlight();
}
```

### 4.5 Синхронизация скролла

```cpp
// В конструкторе DiffEditor:
connect(m_leftPanel->verticalScrollBar(), &QScrollBar::valueChanged,
        this, &DiffEditor::synchronizeScrollLeftToRight);
connect(m_rightPanel->verticalScrollBar(), &QScrollBar::valueChanged,
        this, &DiffEditor::synchronizeScrollRightToLeft);

void DiffEditor::synchronizeScrollLeftToRight(int value)
{
    bool blocked = m_rightPanel->verticalScrollBar()->blockSignals(true);

    int leftRange = m_leftPanel->verticalScrollBar()->maximum()
                   - m_leftPanel->verticalScrollBar()->minimum();
    int rightRange = m_rightPanel->verticalScrollBar()->maximum()
                    - m_rightPanel->verticalScrollBar()->minimum();

    if (leftRange > 0 && rightRange > 0) {
        double percent = double(value - m_leftPanel->verticalScrollBar()->minimum()) / leftRange;
        int newValue = m_rightPanel->verticalScrollBar()->minimum()
                      + int(percent * rightRange);
        m_rightPanel->verticalScrollBar()->setValue(newValue);
    }

    m_rightPanel->verticalScrollBar()->blockSignals(blocked);
}

void DiffEditor::synchronizeScrollRightToLeft(int value)
{
    // Аналогично, в обратном направлении
    bool blocked = m_leftPanel->verticalScrollBar()->blockSignals(true);

    int leftRange = m_leftPanel->verticalScrollBar()->maximum()
                   - m_leftPanel->verticalScrollBar()->minimum();
    int rightRange = m_rightPanel->verticalScrollBar()->maximum()
                    - m_rightPanel->verticalScrollBar()->minimum();

    if (leftRange > 0 && rightRange > 0) {
        double percent = double(value - m_rightPanel->verticalScrollBar()->minimum()) / rightRange;
        int newValue = m_leftPanel->verticalScrollBar()->minimum()
                      + int(percent * leftRange);
        m_leftPanel->verticalScrollBar()->setValue(newValue);
    }

    m_leftPanel->verticalScrollBar()->blockSignals(blocked);
}
```

### 4.6 Синхронизация зума (через CodeEditor)

Поскольку `DiffPanel` наследуется от `CodeEditor`, он уже имеет сигнал `zoomChanged`:

```cpp
// В конструкторе DiffEditor:
connect(m_leftPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);
connect(m_rightPanel, &CodeEditor::zoomChanged, this, &DiffEditor::synchronizeZoom);

void DiffEditor::synchronizeZoom(int zoom)
{
    bool leftBlocked = m_leftPanel->blockSignals(true);
    bool rightBlocked = m_rightPanel->blockSignals(true);

    if (sender() == m_leftPanel)
        m_rightPanel->setZoom(zoom);
    else
        m_leftPanel->setZoom(zoom);

    m_leftPanel->blockSignals(leftBlocked);
    m_rightPanel->blockSignals(rightBlocked);
}
```

---

## Шаг 5: Интеграция с MainWindow

### 5.1 Изменения в `mainwindow.ui`

Заменить в `mainwindow.ui`:
```xml
<widget class="QPlainTextEdit" name="stagedContentTextEdit"/>
<widget class="QPlainTextEdit" name="currentContentTextEdit"/>
```

На **widget promotion**:
```xml
<widget class="DiffEditor" name="diffEditor">
 <property name="sizePolicy">
  <sizepolicy hsizetype="Expanding" vsizetype="Expanding">
   <horstretch>0</horstretch>
   <verstretch>0</verstretch>
  </sizepolicy>
 </property>
</widget>
```

В секцию `<customwidgets>` добавить:
```xml
<customwidget>
 <class>DiffEditor</class>
 <extends>QWidget</extends>
 <header>diffeditor.h</header>
 <container>1</container>
</customwidget>
```

### 5.2 Изменения в `mainwindow.h`

Заменить:
```cpp
CodeEditor *stagedContentEditor;
CodeEditor *currentContentEditor;
```

На:
```cpp
DiffEditor *diffEditor;
```

Также добавить forward declaration:
```cpp
class DiffEditor;
```

### 5.3 Изменения в `mainwindow.cpp`

**В конструкторе** — удалить старую инициализацию CodeEditor:

```cpp
// УДАЛИТЬ весь блок:
// delete ui->stagedContentTextEdit;
// delete ui->currentContentTextEdit;
// stagedContentEditor = new CodeEditor(this);
// currentContentEditor = new CodeEditor(this);
// ui->diffSplitter->addWidget(stagedContentEditor);
// ui->diffSplitter->addWidget(currentContentEditor);
// connect(stagedContentEditor, &CodeEditor::zoomChanged, ...)
// connect(currentContentEditor, &CodeEditor::zoomChanged, ...)

// DiffEditor уже создан через UI (promoted widget)
// Подключаем Git сигнал к DiffEditor слоту
connect(git, &Git::diffReady, diffEditor, &DiffEditor::onGitDiffReady);
```

**В методе `updateDiffPanel`:**

```cpp
void MainWindow::updateDiffPanel(const QString &fileName)
{
    QString stagedContent = getFileContent(fileName, true);
    QString currentContent = getFileContent(fileName, false);

    diffEditor->setContents(stagedContent, currentContent, fileName);
}
```

**Навигация по ханкам:**

```cpp
void MainWindow::navigateToNextHunk()
{
    diffEditor->navigateToNextHunk();
}

void MainWindow::navigateToPrevHunk()
{
    diffEditor->navigateToPrevHunk();
}
```

**Удалить** (больше не нужно — перенесено в DiffEditor):
- `parseAndApplyDiffHighlighting()`
- `extractHunkPositions()`
- `synchronizeZoom()`
- `synchronizeScroll()`
- `applyDiffHighlighting()`

---

## Шаг 6: Расширение подсветки синтаксиса

Существующий `Highlighter` поддерживает только C++/Qt. Нужно поддержать другие языки.

### 6.1 Рекомендуемый подход: Фабрика SyntaxHighlighter

```cpp
// syntaxhighlighterfactory.h
class SyntaxHighlighterFactory {
public:
    static QSyntaxHighlighter* createHighlighter(const QString &fileName,
                                                  QTextDocument *document);
};

// syntaxhighlighterfactory.cpp
QSyntaxHighlighter* SyntaxHighlighterFactory::createHighlighter(
    const QString &fileName, QTextDocument *document)
{
    QString ext = QFileInfo(fileName).suffix().toLower();

    if (ext == "py" || ext == "pyw")
        return new PythonHighlighter(document);
    if (ext == "js" || ext == "jsx" || ext == "mjs")
        return new JavaScriptHighlighter(document);
    if (ext == "json")
        return new JsonHighlighter(document);
    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "h" || ext == "hpp")
        return new CppHighlighter(document);  // можно переиспользовать существующий Highlighter
    if (ext == "xml" || ext == "xaml")
        return new XmlHighlighter(document);

    return nullptr; // Без подсветки (plain text)
}
```

### 6.2 Расширение существующего Highlighter

Можно переиспользовать существующий `Highlighter` как `CppHighlighter`, добавив параметр языка:

```cpp
// В существующий syntaxhighlighter.h добавить:
class Highlighter : public QSyntaxHighlighter {
public:
    enum Language {
        Cpp,
        Python,
        JavaScript,
        Json,
        PlainText
    };

    explicit Highlighter(QTextDocument *parent = nullptr, Language lang = Cpp);
    // ...
};
```

### 6.3 Поддерживаемые языки (MVP)

| Язык | Расширения | Приоритет |
|---|---|---|
| C/C++ | `.cpp`, `.cxx`, `.cc`, `.c++`, `.h`, `.hpp`, `.hxx` | Высокий (уже есть) |
| Python | `.py`, `.pyw` | Средний |
| JavaScript | `.js`, `.jsx`, `.mjs` | Средний |
| JSON | `.json` | Низкий (простая подсветка) |
| XML | `.xml`, `.xaml` | Низкий |
| Plain text | остальные | Дефолт (без подсветки) |

---

## Шаг 7: Обработка edge-кейсов

### 7.1 Файл не существует в HEAD (новый файл)

Если файл ещё не был закоммичен, `git show HEAD:file` вернёт ошибку. В этом случае:
- **Левая панель**: пустая (или placeholder "Новый файл")
- **Правая панель**: полное содержимое файла с зелёным фоном (все строки `Added`)

### 7.2 Файл удалён

Если файл удалён (но ещё в working directory):
- **Левая панель**: содержимое из HEAD с красным фоном (все строки `Removed`)
- **Правая панель**: пустая

### 7.3 Бинарные файлы

Для бинарных файлов (определяется по расширению или `git diff --binary`):
- Показать сообщение "Бинарный файл — diff недоступен"
- Не пытаться отобразить содержимое

### 7.4 Большие файлы

Для файлов > N строк (например, > 10000):
- Показать предупреждение "Файл слишком большой для отображения diff"
- Предложить открыть файл во внешнем редакторе

### 7.5 Файлы с разными кодировками

Использовать `QTextStream` с автоопределением кодировки, или использовать `QTextCodec::codecForLocale()`.

---

## Шаг 8: Тестирование

### 8.1 Unit-тесты для intra-line diff

В `commit_craft_tests/` создать `tst_intralinediff.cpp`:

```cpp
void TestIntraLineDiff::testSimpleChange()
{
    QString oldLine = "int x = 42;";
    QString newLine = "int x = 100;";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    // "42" изменено на "100"
    QVERIFY(!oldRanges.isEmpty());
    QVERIFY(!newRanges.isEmpty());
}

void TestIntraLineDiff::testIdenticalLines()
{
    QString oldLine = "same line";
    QString newLine = "same line";

    auto [oldRanges, newRanges] =
        IntraLineDiffCalculator::computeChangedRanges(oldLine, newLine);

    QVERIFY(oldRanges.isEmpty());
    QVERIFY(newRanges.isEmpty());
}
```

### 8.2 Unit-тесты для buildLineDiffMap

```cpp
void TestDiffHighlighter::testAddedFile()
{
    // Новый файл — все строки Added
    QList<Hunk> hunks = createTestHunks({
        {HunkLine::Added, "line1"},
        {HunkLine::Added, "line2"},
    });

    auto rightMap = buildLineDiffMapRight(hunks);

    QCOMPARE(rightMap[0].type, DiffType::Added);
    QCOMPARE(rightMap[1].type, DiffType::Added);
}

void TestDiffHighlighter::testModifiedLines()
{
    // Строка изменена: Removed → Added подряд
    QList<Hunk> hunks = createTestHunks({
        {HunkLine::Removed, "old line"},
        {HunkLine::Added, "new line"},
    });

    auto leftMap = buildLineDiffMapLeft(hunks);
    auto rightMap = buildLineDiffMapRight(hunks);

    QCOMPARE(leftMap[0].type, DiffType::Modified);
    QCOMPARE(rightMap[0].type, DiffType::Modified);
}
```

### 8.3 Integration-тесты

Создать тестовые файлы в `commit_craft_tests/sample_data/`:
- `diff_test_original.cpp` — исходная версия
- `diff_test_modified.cpp` — изменённая версия (с добавлениями, удалениями, модификациями)

Протестировать `DiffEditor::setContents()` на этих файлах.

---

## Шаг 9: Визуальные улучшения (опционально, после базовой реализации)

### 9.1 Линии-связки между панелями

Рисовать горизонтальные линии между соответствующими строками левой и правой панелей (как в Meld), чтобы визуально связать изменённые блоки.

### 9.2 Миникарта (minimap)

Добавить справа от каждой панели миникарту — уменьшенное представление всего файла с цветными маркерами изменений.

### 9.3 Fold-регионы

Сворачивать неизменённые блоки текста (как в SmartGit), показывая только изменённые ханки.

### 9.4 Нумерация строк с gutter-разделителем

Добавить gutter (полосу между номерами строк и текстом) с маркерами:
- `-` для удалённых строк
- `+` для добавленных строк
- `~` для модифицированных

### 9.5 Настройка цветов

Вынести цвета в QSettings или тему:

```cpp
struct DiffColors {
    QColor addedBg      = QColor(200, 255, 200);
    QColor removedBg    = QColor(255, 200, 200);
    QColor modifiedBg   = QColor(180, 200, 255);
    QColor modifiedIntra = QColor(140, 170, 255, 120);
    // ...
};
```

---

## Порядок реализации (итерации)

| Итерация | Что делаем | Результат |
|---|---|---|
| **1** | Создать `diffpanel.h/cpp` (наследуется от CodeEditor) с paintEvent | Панель с фонами строк, номерами строк и зумом из коробки |
| **2** | Создать `diffhighlighter.h/cpp` — адаптер GitParser → LineDiffInfo | Маппинг Hunk/HunkLine на DiffType (Added/Removed/Modified) |
| **3** | Создать `intralinediff.h/cpp` — LCS для intra-line подсветки | Синий фон на конкретных изменённых символах |
| **4** | Создать `diffeditor.h/cpp` с двумя панелями, синхронным скроллом и зумом | Две панели рядом с полной синхронизацией |
| **5** | Интегрировать `DiffEditor` в `MainWindow` (promoted widget, подключение Git сигнала) | Рабочий diff в главном окне |
| **6** | Добавить подсветку синтаксиса для разных языков (фабрика highlighter'ов) | Syntax highlighting по типу файла |
| **7** | Обработать edge-кейсы (новые файлы, удалённые, бинарные) | Стабильная работа со всеми случаями |
| **8** | Написать unit-тесты (IntraLineDiffCalculator, buildLineDiffMap) | Покрытие тестами |
| **9** | Визуальные улучшения (gutter, fold, minimap) | UI уровня SmartGit/Meld |

---

## Зависимости

- **Qt Modules**: `QtWidgets`, `QtGui`, `QtCore` (уже подключены)
- **commit_craft_lib**: `GitParser` (уже есть, парсит `git diff`), `Git` (уже есть, асинхронные вызовы)
- **commit_craft_app**: `CodeEditor` (уже есть — номера строк, зум, syntax highlighting)
- **Внешние библиотеки**: не требуются

---

## Риски и сложности

| Риск | Оценка | Митигация |
|---|---|---|
| Производительность intra-line diff на больших строках | Средняя | Порог >200 символов → diff на уровне слов |
| Рассинхронизация скролла при разной высоте контента | Низкая | Синхронизация по процентам, не по пикселям |
| Конфликт diff-подсветки и syntax highlighting | Низкая | Diff-фон рисуется в paintEvent поверх текста (CompositionMode_SourceOver), syntax — через QTextCharFormat (отдельный слой Qt) |
| Неправильное определение Modified (Removed+Added ≠ модификация) | Низкая | Эвристика: схожесть строк >50% → Modified, иначе Separate |
| paintEvent затирает фоны | **Устранён** | Фоны рисуются **после** вызова `CodeEditor::paintEvent(event)` |
| Дублирование кода номеров строк/зума | **Устранён** | DiffPanel наследуется от CodeEditor — всё готово |
