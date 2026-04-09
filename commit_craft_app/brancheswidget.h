#ifndef BRANCHESWIDGET_H
#define BRANCHESWIDGET_H

#include <QFrame>

class QTreeWidget;
class QVBoxLayout;

class BranchesWidget : public QFrame
{
    Q_OBJECT

public:
    explicit BranchesWidget(QWidget *parent = nullptr);

private:
    QVBoxLayout *m_layout;
    QTreeWidget *m_treeWidget;
};

#endif // BRANCHESWIDGET_H
