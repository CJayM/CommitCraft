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
├── DiffPanel* leftPanel       ← staged версия (или HEAD)
├── DiffPanel* rightPanel      ← current версия (рабочая директория)
├── LineNumberArea* leftLineNumbers
├── LineNumberArea* rightLineNumbers
├── DiffHighlighter* leftHighlighter
├── DiffHighlighter* rightHighlighter
├── SyntaxHighlighter* leftSyntaxHighlighter  ← зависит от типа файла
├── SyntaxHighlighter* rightSyntaxHighlighter
└── QScrollBar* sharedVScrollBar  (опционально)

DiffPanel (QWidget)
├── QTextEdit или QPlainTextEdit (базовый виджет для отображения текста)
└── Переопределённый paintEvent для рисования фона строк

DiffHunk (struct)
├── DiffType type              ← Added, Removed, Modified, Unchanged
├── int leftStartLine          ← начальный номер строки в левой панели
├── int leftLineCount
├── int rightStartLine         ← начальный номер строки в правой панели
├── int rightLineCount
└── QVector<IntraLineDiff> intraLineDiffs  ← для intra-line (синий фон)

IntraLineDiff (struct)
├── int lineNumber
├── QVector<QPair<int, int>> changedRanges  ← (start, length) символов
└── DiffType type
```

---

## Шаг 1: Создание файлов нового виджета

Создать в `commit_craft_app/`:

| Файл | Назначение |
|---|---|
| `diffeditor.h` | Объявление класса `DiffEditor` |
| `diffeditor.cpp` | Реализация `DiffEditor` |
| `diffpanel.h` | Объявление класса `DiffPanel` (одна панель отображения) |
| `diffpanel.cpp` | Реализация `DiffPanel` с переопределённым `paintEvent` |
| `diffhighlighter.h` | Объявление `DiffHighlighter` (подсветка diff-изменений) |
| `diffhighlighter.cpp` | Реализация `DiffHighlighter` |

Добавить в `commit_craft_app.pro`:

```qmake
SOURCES += \
    codeeditor.cpp \
    diffeditor.cpp \
    diffpanel.cpp \
    diffhighlighter.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    syntaxhighlighter.cpp

HEADERS += \
    codeeditor.h \
    diffeditor.h \
    diffpanel.h \
    diffhighlighter.h \
    linenumberarea.h \
    mainwindow.h \
    settingsdialog.h \
    syntaxhighlighter.h
```

---

## Шаг 2: Реализация алгоритма diff

### 2.1 Выбор алгоритма

Использовать **алгоритм Myers diff** — стандартный алгоритм для поиска наименьшей последовательности правок (edit distance). Он даёт оптимальное разбиение на добавления/удаления.

Создать файл `myersdiff.h` / `myersdiff.cpp` (или в составе `diffhighlighter`):

```cpp
struct DiffOp {
    enum Type { Equal, Insert, Delete, Replace };
    Type type;
    int leftStart, leftCount;
    int rightStart, rightCount;
};

class MyersDiff {
public:
    static QVector<DiffOp> compute(const QStringList& leftLines,
                                   const QStringList& rightLines);
};
```

### 2.2 Intra-line diff (для синего фона — модификации внутри строки)

Для строк, которые были изменены (не полностью удалены/добавлены), вычислить **посимвольный diff** внутри строки. Можно использовать простой LCS (Longest Common Subsequence) на уровне символов или слов:

```cpp
struct IntraLineDiff {
    enum Type { Unchanged, Added, Removed };
    Type type;
    int start;
    int length;
};

class IntraLineDiffCalculator {
public:
    static QVector<IntraLineDiff> compute(const QString& oldLine,
                                          const QString& newLine);
};
```

### 2.3 Структуры данных

```cpp
// diffhighlighter.h

enum class DiffType {
    Unchanged,
    Added,       // зелёный фон
    Removed,     // красный фон
    Modified     // синий фон (intra-line diff)
};

struct DiffHunk {
    DiffType type;
    int leftStartLine;    // -1 если строка только добавлена
    int leftLineCount;
    int rightStartLine;   // -1 если строка только удалена
    int rightLineCount;
    QVector<IntraLineDiff> leftIntraDiffs;   // для Modified строк слева
    QVector<IntraLineDiff> rightIntraDiffs;  // для Modified строк справа
};

struct IntraLineDiff {
    int start;
    int length;
    DiffType type;  // Added (внутри новой строки) или Removed (внутри старой)
};
```

---

## Шаг 3: Реализация DiffPanel

`DiffPanel` — это виджет, отображающий одну из двух сторон diff.

### 3.1 Базовый класс

Использовать `QPlainTextEdit` как базу (наследуемся от него):

```cpp
// diffpanel.h
class DiffPanel : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit DiffPanel(QWidget *parent = nullptr);

    // Установка содержимого и diff-данных
    void setContent(const QString &text);
    void setDiffData(const QVector<DiffHunk> &hunks, bool isLeftPanel);

    // Переопределение paintEvent для рисования фона строк
    void paintEvent(QPaintEvent *event) override;

    // Получение видимых блоков для рисования номеров строк
    friend class DiffLineNumberArea;

signals:
    void scrollChanged(int value);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawLineBackgrounds(QPainter &painter);
    void drawIntraLineHighlights(QPainter &painter);

    QVector<DiffHunk> m_diffHunks;
    bool m_isLeftPanel;

    friend class DiffLineNumberArea;
    DiffLineNumberArea *m_lineNumberArea;
};
```

### 3.2 Отрисовка фона строк (paintEvent)

В `paintEvent` перед вызовом базового класса рисуем фоны строк:

```cpp
// diffpanel.cpp
void DiffPanel::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());

    // 1. Рисуем фоны строк (красный/зелёный/синий)
    drawLineBackgrounds(painter);

    // 2. Вызываем базовый paintEvent (отрисовка текста)
    QPlainTextEdit::paintEvent(event);

    // 3. Рисуем intra-line подсветку (синие фрагменты) поверх текста
    drawIntraLineHighlights(painter);
}
```

**drawLineBackgrounds:**

```cpp
void DiffPanel::drawLineBackgrounds(QPainter &painter)
{
    const auto colors = QColor(255, 200, 200); // красный (удалено)
    const auto colorA = QColor(200, 255, 200); // зелёный (добавлено)
    const auto colorM = QColor(180, 200, 255); // синий (модифицировано)

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());

    for (; block.isValid() && top <= viewport()->height(); block = block.next()) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            blockNumber++;
            continue;
        }

        int bottom = top + qRound(blockBoundingRect(block).height());

        // Найти diff-тип для этой строки
        for (const auto &hunk : m_diffHunks) {
            int lineInHunk = -1;
            if (m_isLeftPanel && hunk.leftStartLine >= 0) {
                if (blockNumber >= hunk.leftStartLine &&
                    blockNumber < hunk.leftStartLine + hunk.leftLineCount) {
                    lineInHunk = blockNumber - hunk.leftStartLine;
                }
            } else if (!m_isLeftPanel && hunk.rightStartLine >= 0) {
                if (blockNumber >= hunk.rightStartLine &&
                    blockNumber < hunk.rightStartLine + hunk.rightLineCount) {
                    lineInHunk = blockNumber - hunk.rightStartLine;
                }
            }

            if (lineInHunk >= 0) {
                QColor bgColor;
                switch (hunk.type) {
                    case DiffType::Removed:   bgColor = colors; break;
                    case DiffType::Added:     bgColor = colorA; break;
                    case DiffType::Modified:  bgColor = colorM; break;
                    default: break;
                }

                if (bgColor.isValid()) {
                    painter.fillRect(0, top, viewport()->width(), bottom - top, bgColor);
                }
                break;
            }
        }

        top = bottom;
        blockNumber++;
    }
}
```

### 3.3 Intra-line подсветка (синие фрагменты)

```cpp
void DiffPanel::drawIntraLineHighlights(QPainter &painter)
{
    // Для строк типа Modified: рисуем более тёмный синий
    // на участках, которые конкретно изменились
    const QColor intraColor(140, 170, 255, 120); // полупрозрачный синий

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());

    for (; block.isValid() && top <= viewport()->height(); block = block.next()) {
        if (!block.isVisible()) {
            top += qRound(blockBoundingRect(block).height());
            blockNumber++;
            continue;
        }

        for (const auto &hunk : m_diffHunks) {
            if (hunk.type != DiffType::Modified) continue;

            const auto &intraDiffs = m_isLeftPanel ? hunk.leftIntraDiffs : hunk.rightIntraDiffs;
            int startLine = m_isLeftPanel ? hunk.leftStartLine : hunk.rightStartLine;

            if (blockNumber == startLine) {
                for (const auto &diff : intraDiffs) {
                    // Получить QRect для диапазона символов
                    QTextCursor cursor(document());
                    cursor.setPosition(block.position() + diff.start);
                    QRect startRect = cursorRect(cursor);

                    cursor.setPosition(block.position() + diff.start + diff.length);
                    QRect endRect = cursorRect(cursor);

                    QRect highlightRect(startRect.left(), top,
                                       endRect.right() - startRect.left(),
                                       blockBoundingRect(block).height());

                    painter.fillRect(highlightRect, intraColor);
                }
            }
        }

        top += qRound(blockBoundingRect(block).height());
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

private:
    void computeDiff();
    void applySyntaxHighlighting(const QString &fileName);
    void synchronizeScrollLeftToRight(int value);
    void synchronizeScrollRightToLeft(int value);
    void synchronizeZoom(int zoom);

    DiffPanel *m_leftPanel;
    DiffPanel *m_rightPanel;
    DiffLineNumberArea *m_leftLineNumbers;
    DiffLineNumberArea *m_rightLineNumbers;
    DiffHighlighter *m_diffHighlighter;
    Highlighter *m_leftSyntaxHighlighter;  // существующий Highlighter
    Highlighter *m_rightSyntaxHighlighter;

    QVector<DiffHunk> m_diffHunks;
    int m_currentHunkIndex;

    // Для синхронизации зума
    int m_currentZoom;
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

    // 2. Вычислить diff
    computeDiff();

    // 3. Применить diff-подсветку
    m_diffHighlighter->setDiffData(m_diffHunks);
    m_leftPanel->setDiffData(m_diffHunks, true);
    m_rightPanel->setDiffData(m_diffHunks, false);

    // 4. Применить подсветку синтаксиса
    applySyntaxHighlighting(fileName);

    // 5. Сбросить навигацию по ханкам
    m_currentHunkIndex = -1;
}
```

### 4.3 Подсветка синтаксиса по типу файла

```cpp
void DiffEditor::applySyntaxHighlighting(const QString &fileName)
{
    // Определить тип файла по расширению
    QString ext = QFileInfo(fileName).suffix().toLower();

    // Маппинг расширений на режимы подсветки
    static const QHash<QString, QStringList> langMap = {
        {"cpp", {"cpp", "cxx", "cc", "c++", "h", "hpp", "hxx"}},
        {"python", {"py", "pyw"}},
        {"java", {"java"}},
        {"javascript", {"js", "jsx", "mjs"}},
        {"typescript", {"ts", "tsx"}},
        {"html", {"html", "htm"}},
        {"css", {"css", "scss", "sass", "less"}},
        {"json", {"json"}},
        {"xml", {"xml", "xaml"}},
        {"yaml", {"yaml", "yml"}},
        {"shell", {"sh", "bash", "zsh"}},
        {"markdown", {"md", "markdown"}},
        {"sql", {"sql"}},
    };

    // Создать соответствующий SyntaxHighlighter для каждого языка
    // (потребуется расширить существующий Highlighter или создать
    // фабрику SyntaxHighlighter)

    // Пока используем существующий Highlighter (C++) как дефолтный
    m_leftSyntaxHighlighter->rehighlight();
    m_rightSyntaxHighlighter->rehighlight();
}
```

### 4.4 Синхронизация скролла

```cpp
// В конструкторе DiffEditor:
connect(m_leftPanel->verticalScrollBar(), &QScrollBar::valueChanged,
        this, &DiffEditor::synchronizeScrollLeftToRight);
connect(m_rightPanel->verticalScrollBar(), &QScrollBar::valueChanged,
        this, &DiffEditor::synchronizeScrollRightToLeft);

void DiffEditor::synchronizeScrollLeftToRight(int value)
{
    // Блокируем сигнал правой панели, чтобы не было рекурсии
    bool blocked = m_rightPanel->verticalScrollBar()->blockSignals(true);

    // Синхронизация по процентам (как в текущей реализации)
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
    // Аналогично, но в обратном направлении
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

---

## Шаг 5: Интеграция с MainWindow

### 5.1 Изменения в `mainwindow.ui`

Удалить из `mainwindow.ui`:
```xml
<widget class="QPlainTextEdit" name="stagedContentTextEdit"/>
<widget class="QPlainTextEdit" name="currentContentTextEdit"/>
```

Заменить на **widget promotion**:

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

### 5.3 Изменения в `mainwindow.cpp`

**В конструкторе** — удалить старую инициализацию `CodeEditor`:

```cpp
// УДАЛИТЬ:
// delete ui->stagedContentTextEdit;
// delete ui->currentContentTextEdit;
// stagedContentEditor = new CodeEditor(this);
// currentContentEditor = new CodeEditor(this);
// ui->diffSplitter->addWidget(stagedContentEditor);
// ui->diffSplitter->addWidget(currentContentEditor);

// DiffEditor уже создан через UI (promoted widget), просто используем:
// diffEditor = ui->diffEditor;  // если добавить в .h
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

**Удалить** (больше не нужно):
- `parseAndApplyDiffHighlighting()` — теперь внутри DiffEditor
- `extractHunkPositions()` — теперь внутри DiffEditor
- `synchronizeZoom()` — теперь внутри DiffEditor
- `synchronizeScroll()` — теперь внутри DiffEditor
- `applyDiffHighlighting()` — теперь внутри DiffEditor

---

## Шаг 6: Расширение подсветки синтаксиса

Существующий `Highlighter` поддерживает только C++/Qt. Нужно расширить его или создать фабрику highlighter'ов.

### 6.1 Вариант А: Расширить Highlighter

Добавить в `Highlighter` конструктор с параметром языка:

```cpp
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

### 6.2 Вариант Б: Фабрика SyntaxHighlighter

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
    if (ext == "js" || ext == "jsx")
        return new JavaScriptHighlighter(document);
    if (ext == "json")
        return new JsonHighlighter(document);
    if (ext == "cpp" || ext == "h" || ext == "hpp")
        return new CppHighlighter(document);

    return nullptr; // Без подсветки
}
```

### 6.3 Рекомендуемый подход

Создать базовый класс `BaseSyntaxHighlighter` и наследников для каждого языка:

```
BaseSyntaxHighlighter (abstract)
├── CppHighlighter
├── PythonHighlighter
├── JavaScriptHighlighter
├── JsonHighlighter
├── XmlHighlighter
├── PlainTextHighlighter (пустой)
```

Это позволит легко добавлять новые языки в будущем.

---

## Шаг 7: Обработка edge-кейсов

### 7.1 Файл не существует в HEAD (новый файл)

Если файл ещё не был закоммичен, `git show HEAD:file` вернёт ошибку. В этом случае:
- **Левая панель**: пустая (или placeholder "Новый файл")
- **Правая панель**: полное содержимое файла с зелёным фоном

### 7.2 Файл удалён

Если файл удалён (но ещё в working directory):
- **Левая панель**: содержимое из HEAD с красным фоном
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

### 8.1 Unit-тесты для diff-алгоритма

В `commit_craft_tests/` создать `tst_myersdiff.cpp`:

```cpp
void TestMyersDiff::testSimpleAddition()
{
    QStringList left = {"a", "b", "c"};
    QStringList right = {"a", "b", "x", "c"};

    auto ops = MyersDiff::compute(left, right);

    QCOMPARE(ops.size(), 3);
    QCOMPARE(ops[1].type, DiffOp::Insert);
}

void TestMyersDiff::testSimpleDeletion()
{
    QStringList left = {"a", "b", "c"};
    QStringList right = {"a", "c"};

    auto ops = MyersDiff::compute(left, right);

    QCOMPARE(ops[1].type, DiffOp::Delete);
}

void TestMyersDiff::testModification()
{
    QStringList left = {"a", "b", "c"};
    QStringList right = {"a", "x", "c"};

    auto ops = MyersDiff::compute(left, right);

    // b -> x должно быть определено как Replace
    QCOMPARE(ops[1].type, DiffOp::Replace);
}
```

### 8.2 Integration-тесты

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
| **1** | Создать `myersdiff.h/cpp` с алгоритмом Myers diff | Working diff-алгоритм, покрытый тестами |
| **2** | Создать `diffpanel.h/cpp` с отрисовкой фонов строк | Панель, показывающая красные/зелёные/синие строки |
| **3** | Создать `diffeditor.h/cpp` с двумя панелями и синхронным скроллом | Две панели рядом с синхронизацией |
| **4** | Интегрировать `DiffEditor` в `MainWindow` вместо `CodeEditor` | Рабочий diff в главном окне |
| **5** | Добавить intra-line diff (синий фон для модификаций) | Полноценная трёхцветная подсветка |
| **6** | Добавить подсветку синтаксиса для разных языков | Syntax highlighting по типу файла |
| **7** | Обработать edge-кейсы (новые файлы, удалённые, бинарные) | Стабильная работа со всеми случаями |
| **8** | Написать unit-тесты | Покрытие тестами |
| **9** | Визуальные улучшения (gutter, fold, minimap) | UI уровня SmartGit/Meld |

---

## Зависимости

- **Qt Modules**: `QtWidgets`, `QtGui`, `QtCore` (уже подключены)
- **Внешние библиотеки**: не требуются (алгоритм Myers diff реализуется вручную)

## Риски и сложности

| Риск | Оценка | Митигация |
|---|---|---|
| Производительность на больших файлах | Средняя | Ограничить max размер, использовать ленивое вычисление |
| Сложность intra-line diff | Средняя | Использовать простой LCS на словах, не символах |
| Конфликт diff-подсветки и syntax highlighting | Низкая | Diff-фон рисуется в paintEvent, syntax — через QTextCharFormat (поверх) |
| Рассинхронизация скролла при разной высоте контента | Низкая | Синхронизация по процентам, не по пикселям |
