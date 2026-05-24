#include "commititemdelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QFontMetrics>
#include <QLinearGradient>

// Цвет ветки по индексу колонки (современная палитра)
static QColor branchColorForColumn(int col)
{
    static const QVector<QColor> colors = {
        QColor(207, 34, 46),    // красный
        QColor(9, 133, 48),     // зелёный
        QColor(9, 105, 218),    // синий
        QColor(130, 80, 223),   // фиолетовый
        QColor(204, 164, 16),   // жёлтый
        QColor(219, 102, 21),   // оранжевый
        QColor(17, 159, 137),   // бирюзовый
        QColor(186, 84, 136),   // розовый
        QColor(42, 154, 192),   // голубой
        QColor(140, 160, 60),   // оливковый
    };
    return colors[col % colors.size()];
}

CommitItemDelegate::CommitItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void CommitItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Получаем данные коммита из модели
    QVariant data = index.data(Qt::UserRole);
    if (!data.canConvert<CommitData>()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    CommitData commit = data.value<CommitData>();

    QRect rect = option.rect;
    painter->setRenderHint(QPainter::Antialiasing, true);

    // --- Фон ---
    // Чередование фона для чётных/нечётных строк
    if (index.row() % 2 == 1) {
        painter->fillRect(rect, QColor(248, 249, 251));
    }

    // Фон выделения
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(221, 244, 255)); // #ddf4ff
    }

    // --- Параметры графа ---
    const int dotRadius = 5;
    const int lineWidth = 2;
    const int columnSpacing = 18;
    const int leftMargin = 8;
    const int topPadding = 4;

    int centerY = rect.center().y();
    int dotX = leftMargin + commit.graphColumn * columnSpacing + dotRadius;
    int dotY = centerY;

    int firstParentCol = commit.parentColumns.isEmpty() ? -1 : commit.parentColumns[0];
    bool isBranchCreation = (!commit.parents.isEmpty()
                             && firstParentCol >= 0
                             && firstParentCol != commit.graphColumn);

    // 1. Вертикальные линии для активных колонок
    for (int col : commit.activeColumns) {
        int colX = leftMargin + col * columnSpacing + dotRadius;
        QColor lineColor = branchColorForColumn(col);
        lineColor.setAlpha(120); // полупрозрачные линии
        painter->setPen(QPen(lineColor, lineWidth));
        if (col == commit.graphColumn && isBranchCreation)
            painter->drawLine(colX, rect.top(), colX, dotY - dotRadius);
        else
            painter->drawLine(colX, rect.top(), colX, rect.bottom());
    }

    // 2. Диагональ ответвления
    if (isBranchCreation) {
        int parentDotX = leftMargin + firstParentCol * columnSpacing + dotRadius;
        QColor lineColor = branchColorForColumn(firstParentCol);
        lineColor.setAlpha(120);
        painter->setPen(QPen(lineColor, lineWidth));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 3. Диагонали слияния
    for (int p = 1; p < commit.parents.size(); ++p) {
        int parentCol = (p < commit.parentColumns.size()) ? commit.parentColumns[p] : -1;
        if (parentCol < 0) continue;
        int parentDotX = leftMargin + parentCol * columnSpacing + dotRadius;
        QColor lineColor = branchColorForColumn(parentCol);
        lineColor.setAlpha(120);
        painter->setPen(QPen(lineColor, lineWidth));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 4. Точка коммита
    QColor dotColor = commit.branchColor;
    painter->setBrush(dotColor);
    painter->setPen(QPen(dotColor.darker(120), 1));
    painter->drawEllipse(QPoint(dotX, dotY), dotRadius, dotRadius);

    // --- Текст ---

    // Хэш коммита — сразу после последней активной колонки
    int maxActiveCol = commit.graphColumn;
    for (int col : commit.activeColumns)
        if (col > maxActiveCol) maxActiveCol = col;
    int hashX = leftMargin + (maxActiveCol + 1) * columnSpacing + dotRadius + 8;

    QFont normFont = option.widget ? option.widget->font() : painter->font();
    QFont smallFont = normFont;
    {
        int ps = normFont.pointSize();
        if (ps <= 1) {
            // Font was set via pixel size (stylesheet)
            int px = qMax(1, normFont.pixelSize() - 1);
            smallFont.setPixelSize(px);
        } else {
            smallFont.setPointSize(qMax(1, ps - 1));
        }
    }
    QFont boldFont = normFont;
    boldFont.setBold(true);

    QFontMetrics fmBold(boldFont);
    QFontMetrics fmNorm(normFont);
    QFontMetrics fmSmall(smallFont);

    int hashWidth = fmBold.horizontalAdvance(commit.shortHash);
    int baseY = rect.top() + topPadding;

    // --- Строка 1: хэш + сообщение + refs ---

    // Рисуем хэш (моноширинный, полупрозрачный)
    painter->setFont(smallFont);
    painter->setPen(QColor(148, 155, 164)); // серый хэш
    painter->drawText(hashX, baseY + fmSmall.ascent(), commit.shortHash);

    // Вычисляем ширину refs
    int refsTotalWidth = 0;
    QList<int> refWidths;
    for (const QString &ref : commit.refs) {
        int rw = fmNorm.horizontalAdvance(ref) + 14;
        refWidths.append(rw);
        refsTotalWidth += rw + 4;
    }
    if (!refWidths.isEmpty()) refsTotalWidth -= 4;

    int msgX = hashX + hashWidth + 10;
    int msgMaxWidth = rect.right() - 8 - msgX - refsTotalWidth - 4;

    // Рисуем сообщение коммита
    // HEAD или selected → bold
    bool isHead = false;
    for (const QString &ref : commit.refs) {
        if (ref.contains("HEAD", Qt::CaseInsensitive)) {
            isHead = true;
            break;
        }
    }

    if (isHead || (option.state & QStyle::State_Selected)) {
        painter->setFont(boldFont);
    } else {
        painter->setFont(normFont);
    }

    painter->setPen(QColor(31, 35, 40)); // #1f2328 - основной текст
    QString elidedMsg = fmNorm.elidedText(commit.message, Qt::ElideRight,
                                          qMax(msgMaxWidth, 10));
    painter->drawText(msgX, baseY + fmBold.ascent(), elidedMsg);

    // Рисуем refs справа на той же строке — как современные badge
    QColor refBg = branchColorForColumn(commit.graphColumn);
    int refsX = rect.right() - 8;
    for (int i = commit.refs.size() - 1; i >= 0; --i) {
        const QString &ref = commit.refs[i];
        int rw = refWidths[i];
        refsX -= rw;

        // Badge фон — современный полупрозрачный стиль
        QColor bg = refBg;
        bg.setAlpha(isHead ? 25 : 18);
        painter->setBrush(bg);
        painter->setPen(QPen(refBg, 1.0));

        // Скруглённый прямоугольник
        int refHeight = fmNorm.height() + 5;
        int refY = baseY - 1;
        QRect refRect(refsX, refY, rw, refHeight);
        painter->drawRoundedRect(refRect, 4, 4);

        // Текст badge
        if (isHead) {
            painter->setFont(boldFont);
            painter->setPen(refBg);
        } else {
            painter->setFont(normFont);
            painter->setPen(refBg);
        }
        painter->drawText(refRect.adjusted(6, 0, -6, 0), Qt::AlignVCenter, ref);

        refsX -= 4;
    }

    // --- Строка 2: дата и автор ---
    painter->setFont(smallFont);
    int line2Y = baseY + fmBold.height() + 2;
    painter->setPen(QColor(101, 109, 118)); // #656d76 - серый текст
    QString dateAuthor = commit.author + ", " + commit.date;
    QString elidedDA = fmNorm.elidedText(dateAuthor, Qt::ElideRight, rect.width() - hashX - 10);
    painter->drawText(hashX, line2Y + fmSmall.ascent(), elidedDA);
}

QSize CommitItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(42);
    return size;
}
