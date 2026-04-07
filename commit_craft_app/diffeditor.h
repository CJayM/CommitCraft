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

class Highlighter;

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

    /// Применить diff-данные (вызывается из MainWindow при получении сигнала от Git)
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
    /// Применить подсветку синтаксиса по типу файла
    void applySyntaxHighlighting(const QString &fileName);

    DiffPanel *m_leftPanel;
    DiffPanel *m_rightPanel;

    // Синтаксис подсветка (переиспользуем существующий Highlighter из CodeEditor)
    // Каждая DiffPanel уже содержит свой Highlighter через CodeEditor
    // Здесь храним ссылки для rehighlight

    // Навигация по ханкам
    QList<QPair<int, int>> m_hunkPositions; // (leftLine, rightLine)
    int m_currentHunkIndex;

    // Layout
    QVBoxLayout *m_layout;
    QSplitter *m_splitter;
};

#endif // DIFFEDITOR_H
