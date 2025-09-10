#ifndef COMMITITEMDELEGATE_H
#define COMMITITEMDELEGATE_H

#include <QList>
#include <QMetaType>
#include <QStyledItemDelegate>

// Register QList<QString> for use with QVariant
Q_DECLARE_METATYPE(QList<QString>)

class CommitItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CommitItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintCommitItem(QPainter *painter, const QStyleOptionViewItem &option, const QList<QString> &commitData) const;
};

#endif // COMMITITEMDELEGATE_H
