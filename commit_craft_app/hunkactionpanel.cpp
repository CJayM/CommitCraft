#include "hunkactionpanel.h"
#include "diffpanel.h"

#include <QScrollBar>
#include <QWheelEvent>
#include <QApplication>

HunkActionPanel::HunkActionPanel(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(kPanelWidth);
    setMinimumHeight(1);
    setMouseTracking(true);
    // Явно запрашиваем paint-события
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
}

HunkActionPanel::~HunkActionPanel() = default;

void HunkActionPanel::setSourcePanel(DiffPanel *panel)
{
    if (m_sourcePanel) {
        disconnect(m_sourcePanel->verticalScrollBar(), &QScrollBar::valueChanged,
                   this, &HunkActionPanel::updateButtonPositions);
    }

    m_sourcePanel = panel;

    if (m_sourcePanel) {
        connect(m_sourcePanel->verticalScrollBar(), &QScrollBar::valueChanged,
                this, &HunkActionPanel::updateButtonPositions);
    }
}

void HunkActionPanel::setHunkPositions(const QVector<int> &positions)
{
    m_hunkPositions = positions;
    updateButtonPositions();
}

void HunkActionPanel::setDiffMode(DiffMode mode)
{
    m_diffMode = mode;
    updateButtonPositions();
}

void HunkActionPanel::setFileIsNew(bool isNew)
{
    m_fileIsNew = isNew;
    updateButtonPositions();
}

void HunkActionPanel::clear()
{
    m_hunkPositions.clear();
    m_buttonRects.clear();
    update();
}

void HunkActionPanel::updateButtonPositions()
{
    if (!m_sourcePanel) {
        return;
    }

    m_buttonRects.clear();

    // Определяем, какие кнопки показывать
    bool showStage = (m_diffMode == UnstagedDiff) && !m_fileIsNew;
    bool showRevert = true; // показывается во всех режимах, кроме new+unstaged
    if (m_diffMode == UnstagedDiff && m_fileIsNew) {
        showStage = true;
        showRevert = false;
    } else if (m_diffMode == StagedDiff) {
        showStage = false;
        showRevert = true;
    }

    // Центрирование: 2 кнопки или 1 кнопка
    int btnCount = (showStage && showRevert) ? 2 : 1;
    int totalW = (btnCount == 2) ? (kButtonSize * 2 + 4) : kButtonSize;
    int cx = (width() - totalW) / 2;

    for (int i = 0; i < m_hunkPositions.size(); ++i) {
        int lineIndex = m_hunkPositions[i];
        QRectF blockRect = m_sourcePanel->blockViewportRect(lineIndex);
        if (blockRect.isNull())
            continue;

        int y = qRound(blockRect.top());

        HunkButtonRect br;
        br.hunkIndex = i;

        if (showStage && showRevert) {
            // Обе кнопки: Revert слева, Stage справа
            br.revertRect = QRect(cx, y, kButtonSize, kButtonSize);
            br.stageRect = QRect(cx + kButtonSize + 4, y, kButtonSize, kButtonSize);
        } else if (showStage) {
            // Только Stage (центрирована)
            br.stageRect = QRect(cx, y, kButtonSize, kButtonSize);
        } else {
            // Только Revert (центрирована)
            br.revertRect = QRect(cx, y, kButtonSize, kButtonSize);
        }

        m_buttonRects.append(br);
    }

    update();
}

void HunkActionPanel::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);

    // 1. Заливка фона — светло-серый
    p.fillRect(rect(), QColor(240, 240, 240));

    // 2. Тонкие рамки слева и справа
    p.setPen(QPen(QColor(200, 200, 200), 1));
    p.drawLine(0, 0, 0, height());
    p.drawLine(width() - 1, 0, width() - 1, height());

    // 3. Рисуем кнопки для каждого ханка
    for (int i = 0; i < m_buttonRects.size(); ++i) {
        const auto &br = m_buttonRects[i];

        // Stage кнопка (зелёная ◀)
        QRect sr = br.stageRect;
        if (sr.isValid() && !sr.isEmpty()) {
            bool hover = (m_hoveredButton == i && m_hoveredIsStage);
            p.setBrush(hover ? QColor(102, 187, 106) : QColor(76, 175, 80));
            p.setPen(QPen(QColor(56, 142, 60), 1));
            p.drawRoundedRect(sr.adjusted(0, 0, -1, -1), 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 7, QFont::Bold));
            p.drawText(sr, Qt::AlignCenter, QStringLiteral("\u25C0"));  // ◀
        }

        // Revert кнопка (красная ▶)
        QRect rr = br.revertRect;
        if (rr.isValid() && !rr.isEmpty()) {
            bool hover = (m_hoveredButton == i && !m_hoveredIsStage);
            p.setBrush(hover ? QColor(239, 83, 80) : QColor(244, 67, 54));
            p.setPen(QPen(QColor(211, 47, 47), 1));
            p.drawRoundedRect(rr.adjusted(0, 0, -1, -1), 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 7, QFont::Bold));
            p.drawText(rr, Qt::AlignCenter, QStringLiteral("\u25B6"));  // ▶
        }
    }
}

void HunkActionPanel::mousePressEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    for (const auto &br : m_buttonRects) {
        if (br.stageRect.contains(pos)) {
            emit stageHunkRequested(br.hunkIndex);
            return;
        }
        if (br.revertRect.contains(pos)) {
            emit revertHunkRequested(br.hunkIndex);
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void HunkActionPanel::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    int oldHover = m_hoveredButton;
    bool oldStage = m_hoveredIsStage;

    m_hoveredButton = -1;
    for (int i = 0; i < m_buttonRects.size(); ++i) {
        const auto &br = m_buttonRects[i];
        if (br.stageRect.contains(pos)) {
            m_hoveredButton = i;
            m_hoveredIsStage = true;
            setCursor(Qt::PointingHandCursor);
            break;
        }
        if (br.revertRect.contains(pos)) {
            m_hoveredButton = i;
            m_hoveredIsStage = false;
            setCursor(Qt::PointingHandCursor);
            break;
        }
    }
    if (m_hoveredButton == -1) {
        setCursor(Qt::ArrowCursor);
    }

    if (oldHover != m_hoveredButton || oldStage != m_hoveredIsStage) {
        update(); // перерисовать при изменении ховера
    }
}

void HunkActionPanel::wheelEvent(QWheelEvent *event)
{
    if (m_sourcePanel) {
        QWheelEvent proxyEvent(
            event->position(), event->globalPosition(),
            event->pixelDelta(), event->angleDelta(),
            event->buttons(), event->modifiers(),
            event->phase(), event->inverted(),
            event->source()
        );
        QApplication::sendEvent(m_sourcePanel->viewport(), &proxyEvent);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}
