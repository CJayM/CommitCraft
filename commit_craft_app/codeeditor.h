#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
class QWheelEvent;
class QTextDocument;
QT_END_NAMESPACE

class LineNumberArea;
class Highlighter;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    virtual void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
    void setZoom(int zoom);
    int zoom() const;
    
    void setSyntaxHighlighting(bool enabled);
    bool syntaxHighlighting() const;

signals:
    void zoomChanged(int zoom);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    QWidget *lineNumberArea;

private:
    Highlighter *highlighter;
    int m_zoom;
    bool m_syntaxHighlighting;
};

#endif // CODEEDITOR_H