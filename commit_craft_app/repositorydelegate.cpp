#include "repositorydelegate.h"

#include <QPainter>
#include <QApplication>
#include <QFontMetrics>
#include <QFileInfo>

RepositoryDelegate::RepositoryDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void RepositoryDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    QString fullPath = index.data(Qt::DisplayRole).toString();
    QString repoName = QFileInfo(fullPath).fileName();
    if (repoName.isEmpty())
        repoName = fullPath;

    QFont normFont = option.widget ? option.widget->font() : painter->font();
    QFont boldFont = normFont;
    boldFont.setBold(true);

    // Размеры для smallFont — на 1px/1pt меньше
    QFont smallFont = normFont;
    {
        int ps = normFont.pointSize();
        if (ps <= 1) {
            int px = qMax(1, normFont.pixelSize() - 2);
            smallFont.setPixelSize(px);
        } else {
            smallFont.setPointSize(qMax(1, ps - 1));
        }
    }

    QFontMetrics fmBold(boldFont);
    QFontMetrics fmSmall(smallFont);

    int padding = 6;
    int baseY = rect.top() + padding + fmBold.ascent();

    // --- Фон ---
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(221, 244, 255));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, QColor(243, 244, 246));
    }

    // --- Название репозитория (жирный) ---
    painter->setFont(boldFont);
    painter->setPen(QColor(31, 35, 40));
    QString elidedName = fmBold.elidedText(repoName, Qt::ElideRight, rect.width() - padding * 2);
    painter->drawText(padding + rect.left(), baseY, elidedName);

    // --- Путь (мелкий, серый) ---
    painter->setFont(smallFont);
    painter->setPen(QColor(101, 109, 118));
    QString elidedPath = fmSmall.elidedText(fullPath, Qt::ElideRight, rect.width() - padding * 2);
    int pathY = rect.top() + padding + fmBold.height() + 4 + fmSmall.ascent();
    painter->drawText(padding + rect.left(), pathY, elidedPath);
}

QSize RepositoryDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    QFont f = option.widget ? option.widget->font() : QApplication::font();
    QFontMetrics fm(f);
    return QSize(200, fm.height() + fm.height() + 12); // две строки + отступы
}
