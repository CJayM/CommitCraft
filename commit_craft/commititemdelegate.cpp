#include "commititemdelegate.h"
#include <QPainter>
#include <QApplication>
#include <QStyleOptionViewItem>
#include <QFontMetrics>

CommitItemDelegate::CommitItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void CommitItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Get the commit data from the model
    QVariant data = index.data(Qt::UserRole);
    if (data.canConvert<QList<QString>>()) {
        QList<QString> commitData = data.value<QList<QString>>();
        paintCommitItem(painter, option, commitData);
    } else {
        // Fallback to default painting
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize CommitItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(40); // Fixed height for two lines of text
    return size;
}

void CommitItemDelegate::paintCommitItem(QPainter *painter, const QStyleOptionViewItem &option, const QList<QString> &commitData) const
{
    // Draw selection background
    QStyleOptionViewItem opt = option;
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter);

    // Set up colors
    QColor textColor = opt.palette.color(QPalette::Text);
    if (opt.state & QStyle::State_Selected) {
        textColor = opt.palette.color(QPalette::HighlightedText);
    }

    painter->save();
    painter->setPen(textColor);

    // Get the rect for drawing
    QRect rect = opt.rect;
    rect.setLeft(rect.left() + 5);
    rect.setRight(rect.right() - 5);

    // Extract commit data (hash, author, date, message)
    QString hash = commitData.size() > 0 ? commitData[0] : "";
    QString author = commitData.size() > 1 ? commitData[1] : "";
    QString date = commitData.size() > 2 ? commitData[2] : "";
    QString message = commitData.size() > 3 ? commitData[3] : "";

    // Truncate hash to 8 characters for better readability
    if (hash.length() > 8) {
        hash = hash.left(8);
    }

    // Format the display text
    QString firstLine = QString("%1 - %2 (%3)").arg(hash, message, author);
    QString secondLine = date;

    // Draw the commit info
    QFont font = painter->font();
    QFont boldFont = font;
    boldFont.setBold(true);

    // Draw first line (hash - message (author))
    painter->setFont(boldFont);
    QFontMetrics fm1(boldFont);
    QString elidedFirstLine = fm1.elidedText(firstLine, Qt::ElideRight, rect.width());
    painter->drawText(rect.left(), rect.top() + fm1.ascent() + 5, elidedFirstLine);

    // Draw second line (date)
    painter->setFont(font);
    QFontMetrics fm2(font);
    QString elidedSecondLine = fm2.elidedText(secondLine, Qt::ElideRight, rect.width());
    painter->drawText(rect.left(), rect.top() + fm1.height() + fm2.ascent() + 5, elidedSecondLine);

    painter->restore();
}