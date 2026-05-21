#ifndef COMMITITEMDELEGATE_H
#define COMMITITEMDELEGATE_H

#include <QList>
#include <QMetaType>
#include <QStyledItemDelegate>
#include "commithistorymodel.h"

// Register CommitData for use with QVariant
Q_DECLARE_METATYPE(CommitData)

class CommitItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommitItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintGraphColumn(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const;
    void paintMessageColumn(QPainter *painter, const QStyleOptionViewItem &option, const CommitData &commit) const;
};

#endif // COMMITITEMDELEGATE_H
