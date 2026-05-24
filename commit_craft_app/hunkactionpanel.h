#ifndef HUNKACTIONPANEL_H
#define HUNKACTIONPANEL_H

#include <QWidget>
#include <QVector>
#include <QPainter>
#include <QMouseEvent>

class DiffPanel;

/// Панель между leftPanel и rightPanel в DiffEditor.
/// Для каждого ханка отображает две маленькие кнопки:
///   Stage ▲  — добавить ханк в index
///   Revert ▼ — откатить ханк в working tree
/// Кнопки рисуются вручную в paintEvent для обхода проблем с native-окнами.
class HunkActionPanel : public QWidget
{
    Q_OBJECT

public:
    enum DiffMode {
        UnstagedDiff,  ///< working tree diff — только Stage кнопки
        StagedDiff     ///< staged vs HEAD diff — только Revert кнопки
    };

    explicit HunkActionPanel(QWidget *parent = nullptr);
    ~HunkActionPanel() override;

    void setSourcePanel(DiffPanel *panel);
    void setHunkPositions(const QVector<int> &positions);
    void setDiffMode(DiffMode mode);
    void setFileIsNew(bool isNew);
    void clear();

signals:
    void stageHunkRequested(int hunkIndex);
    void revertHunkRequested(int hunkIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void updateButtonPositions();

private:
    struct HunkButtonRect {
        QRect stageRect;
        QRect revertRect;
        int hunkIndex = 0;
    };

    QVector<HunkButtonRect> m_buttonRects;
    int m_hoveredButton = -1;     // индекс кнопки под мышью (-1 = нет)
    bool m_hoveredIsStage = false; // true = stage, false = revert

    DiffPanel *m_sourcePanel = nullptr;
    QVector<int> m_hunkPositions;
    DiffMode m_diffMode = UnstagedDiff;
    bool m_fileIsNew = false;

    static constexpr int kButtonSize = 18;
    static constexpr int kPanelWidth = 44;
};

#endif // HUNKACTIONPANEL_H
