#ifndef DIFFEDITOR_H
#define DIFFEDITOR_H

#include "diffpanel.h"
#include "diffhighlighter.h"

#include <gitparser.h>

#include <QWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QScrollBar>
#include <QLabel>
#include <QFileInfo>
#include <QTextDocument>

class Highlighter;

/// Структура для синхронизированной строки diff
struct SyncedLine {
    QString leftText;   // Текст для левой панели (пустая если строка добавлена)
    QString rightText;  // Текст для правой панели (пустая если строка удалена)
    bool isRemoved;     // Строка удалена (отображается слева)
    bool isAdded;       // Строка добавлена (отображается справа)
    bool isModified;    // Строка модифицирована
    bool isContext;     // Контекстная строка (без изменений)
    bool leftIsPlaceholder;  // Левая строка — пустой заполнитель (серый фон)
    bool rightIsPlaceholder; // Правая строка — пустой заполнитель (серый фон)
    int leftLineNum;    // Номер строки в левом файле (-1 если не применяется)
    int rightLineNum;   // Номер строки в правом файле (-1 если не применяется)
};

/// Виджет side-by-side diff с синхронным скроллом, зумом и подсветкой синтаксиса.
class DiffEditor : public QWidget
{
    Q_OBJECT

public:
    explicit DiffEditor(QWidget *parent = nullptr);

    /// Установка содержимого для сравнения.
    /// \param leftContent  Содержимое левой панели (staged / HEAD)
    /// \param rightContent Содержимое правой панели (current / working tree)
    /// \param fileName     Имя файла (для определения типа подсветки синтаксиса)
    void setContents(const QString &leftContent,
                     const QString &rightContent,
                     const QString &fileName);

    /// Очистить содержимое
    void clear();

    /// Применить diff-данные и перестроить side-by-side view
    void applyDiffData(const QList<Hunk> &hunks);

signals:
    void hunkNavigated(int hunkIndex);

public slots:
    void navigateToNextHunk();
    void navigateToPrevHunk();
    int currentHunkIndex() const { return m_currentHunkIndex; }
    int hunkCount() const { return m_hunkPositions.size(); }

private slots:
    void synchronizeScrollLeftToRight(int value);
    void synchronizeScrollRightToLeft(int value);
    void synchronizeZoom(int zoom);

private:
    /// Построить синхронизированные строки из hunks и полного содержимого
    QVector<SyncedLine> buildSyncedLines(const QList<Hunk> &hunks,
                                         const QStringList &leftLines,
                                         const QStringList &rightLines);

    /// Заполнить панели синхронизированным содержимым
    void populatePanels(const QVector<SyncedLine> &syncedLines);

    /// Прокрутить к позиции ханка
    void scrollToHunk(int hunkIndex);

    /// Применить подсветку синтаксиса по типу файла
    void applySyntaxHighlighting(const QString &fileName);

    DiffPanel *m_leftPanel;
    DiffPanel *m_rightPanel;

    // Навигация по ханкам
    QList<QPair<int, int>> m_hunkPositions; // (leftLine, rightLine)
    int m_currentHunkIndex;

    // Флаг для предотвращения рекурсии при синхронизации скролла
    bool m_syncingScroll;

    // Layout
    QVBoxLayout *m_layout;
    QSplitter *m_splitter;

    // Полное содержимое файлов (для side-by-side diff)
    QString m_leftFullContent;
    QString m_rightFullContent;
    QString m_fileName;
};

#endif // DIFFEDITOR_H
