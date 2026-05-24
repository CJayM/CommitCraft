#ifndef REPOSITORYDELEGATE_H
#define REPOSITORYDELEGATE_H

#include <QStyledItemDelegate>

class RepositoryDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RepositoryDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // REPOSITORYDELEGATE_H
