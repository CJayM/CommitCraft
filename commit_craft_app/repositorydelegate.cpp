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

    bool isActive = (fullPath == m_activePath);

    QFont normFont = option.widget ? option.widget->font() : painter->font();
    QFont nameFont = normFont;
    if (isActive)
        nameFont.setBold(true);

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

    QFontMetrics fmName(nameFont);
    QFontMetrics fmSmall(smallFont);

    int padding = 6;
    int baseY = rect.top() + padding + fmName.ascent();

    // --- Фон ---
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(221, 244, 255));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, QColor(243, 244, 246));
    }

    // --- Название репозитория (жирный для активного, нормальный для остальных) ---
    painter->setFont(nameFont);
    painter->setPen(QColor(31, 35, 40));
    QString elidedName = fmName.elidedText(repoName, Qt::ElideRight, rect.width() - padding * 2);
    painter->drawText(padding + rect.left(), baseY, elidedName);

    // --- Путь (мелкий, серый) ---
    painter->setFont(smallFont);
    painter->setPen(QColor(101, 109, 118));
    QString elidedPath = fmSmall.elidedText(fullPath, Qt::ElideRight, rect.width() - padding * 2);
    int pathY = rect.top() + padding + fmName.height() + 4 + fmSmall.ascent();
    painter->drawText(padding + rect.left(), pathY, elidedPath);
}

void RepositoryDelegate::setActivePath(const QString &path)
{
    m_activePath = path;
}

QString RepositoryDelegate::activePath() const
{
    return m_activePath;
}

QSize RepositoryDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    QFont f = option.widget ? option.widget->font() : QApplication::font();
    QFontMetrics fm(f);
    return QSize(200, fm.height() + fm.height() + 12);
}
