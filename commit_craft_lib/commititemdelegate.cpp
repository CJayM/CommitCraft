#include "commititemdelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QFontMetrics>

// Цвет ветки по индексу колонки (должен совпадать с моделью)
static QColor branchColorForColumn(int col)
{
    static const QVector<QColor> colors = {
        QColor(230, 57, 70),    // красный
        QColor(46, 204, 113),   // зеленый
        QColor(52, 152, 219),   // синий
        QColor(155, 89, 182),   // фиолетовый
        QColor(241, 196, 15),   // желтый
        QColor(230, 126, 34),   // оранжевый
        QColor(26, 188, 156),   // бирюзовый
        QColor(241, 128, 23),   // темно-оранжевый
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

    // Фон выделения
    QStyleOptionViewItem opt = option;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

    QRect rect = option.rect;
    painter->setRenderHint(QPainter::Antialiasing, true);

    // --- Параметры графа ---
    const int dotRadius = 6;
    const int lineHeight = 3;
    const int columnSpacing = 20;
    const int leftMargin = 8;

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
        painter->setPen(QPen(branchColorForColumn(col), lineHeight));
        if (col == commit.graphColumn && isBranchCreation)
            painter->drawLine(colX, rect.top(), colX, dotY - dotRadius);
        else
            painter->drawLine(colX, rect.top(), colX, rect.bottom());
    }

    // 2. Диагональ ответвления
    if (isBranchCreation) {
        int parentDotX = leftMargin + firstParentCol * columnSpacing + dotRadius;
        painter->setPen(QPen(branchColorForColumn(firstParentCol), lineHeight));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 3. Диагонали слияния
    for (int p = 1; p < commit.parents.size(); ++p) {
        int parentCol = (p < commit.parentColumns.size()) ? commit.parentColumns[p] : -1;
        if (parentCol < 0) continue;
        int parentDotX = leftMargin + parentCol * columnSpacing + dotRadius;
        painter->setPen(QPen(branchColorForColumn(parentCol), lineHeight));
        painter->drawLine(dotX, dotY, parentDotX, rect.bottom());
    }

    // 4. Точка коммита
    painter->setBrush(commit.branchColor);
    painter->setPen(commit.branchColor);
    painter->drawEllipse(QPoint(dotX, dotY), dotRadius, dotRadius);

    // --- Текст (хэш + сообщение + refs + дата) ---

    // Хэш коммита — сразу после последней активной колонки
    int maxActiveCol = commit.graphColumn;
    for (int col : commit.activeColumns)
        if (col > maxActiveCol) maxActiveCol = col;
    int hashX = leftMargin + (maxActiveCol + 1) * columnSpacing + dotRadius + 8;

    QFont normFont = painter->font();
    QFont boldFont = normFont;
    boldFont.setBold(true);
    QFontMetrics fmBold(boldFont);
    QFontMetrics fmNorm(normFont);

    int hashWidth = fmBold.horizontalAdvance(commit.shortHash);
    int baseY = rect.top() + 2; // отступ сверху

    // Рисуем хэш
    painter->setFont(boldFont);
    painter->setPen(commit.branchColor);
    painter->drawText(hashX, baseY + fmBold.ascent(), commit.shortHash);

    // --- Строка 1: сообщение + refs ---
    // Вычисляем ширину refs
    int refsTotalWidth = 0;
    QList<int> refWidths;
    for (const QString &ref : commit.refs) {
        int rw = fmNorm.horizontalAdvance(ref) + 12;
        refWidths.append(rw);
        refsTotalWidth += rw + 4;
    }
    if (!refWidths.isEmpty()) refsTotalWidth -= 4;

    int msgX = hashX + hashWidth + 8;
    int msgMaxWidth = rect.right() - 5 - msgX - refsTotalWidth - 4;

    painter->setFont(normFont);

    // Определяем цвет текста: HEAD → Accent, иначе обычный
    QColor textColor = option.palette.color(QPalette::Text);
    bool isHead = false;
    for (const QString &ref : commit.refs) {
        if (ref.contains("HEAD", Qt::CaseInsensitive)) {
            isHead = true;
            break;
        }
    }
    if (isHead) {
        textColor = option.palette.color(QPalette::Accent);
    }

    // Выделенная строка — перебиваем цвет на HighlightedText
    if ((option.state & QStyle::State_Selected) | (isHead)) {
        painter->setFont(boldFont);
    }

    painter->setPen(textColor);
    QString elidedMsg = fmNorm.elidedText(commit.message, Qt::ElideRight,
                                          qMax(msgMaxWidth, 10));
    painter->drawText(msgX, baseY + fmBold.ascent(), elidedMsg);

    painter->setFont(normFont);

    // Рисуем refs справа на той же строке — цветом ветки коммита
    QColor refColor = branchColorForColumn(commit.graphColumn);
    int refsX = rect.right() - 5;
    for (int i = 0; i < commit.refs.size(); ++i) {
        const QString &ref = commit.refs[i];
        int rw = refWidths[i];
        refsX -= rw;

        int refHeight = fmNorm.height() + 6;
        int refY = baseY - 2;
        QRect refRect(refsX, refY, rw, refHeight);

        painter->setPen(refColor);
        painter->setBrush(QColor(refColor.red(), refColor.green(), refColor.blue(), 30));
        painter->drawRoundedRect(refRect, 3, 3);

        painter->setPen(refColor);
        painter->setFont(normFont);
        painter->drawText(refRect.adjusted(6, 0, -6, 0), Qt::AlignVCenter, ref);

        refsX -= 4; // промежуток между бейджами
    }

    // --- Строка 2: дата и автор ---
    int line2Y = baseY + fmBold.height() + 2;
    painter->setPen(Qt::gray);
    QString dateAuthor = commit.author + ", " + commit.date;
    QString elidedDA = fmNorm.elidedText(dateAuthor, Qt::ElideRight, rect.width() - 10);
    painter->drawText(hashX, line2Y + fmNorm.ascent(), elidedDA);
}

QSize CommitItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(36);
    return size;
}
