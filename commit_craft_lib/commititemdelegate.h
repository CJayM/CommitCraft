#ifndef COMMITITEMDELEGATE_H
#define COMMITITEMDELEGATE_H

#include <QStyledItemDelegate>
#include "commithistorymodel.h"

class CommitItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommitItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // COMMITITEMDELEGATE_H
