#ifndef BRANCHESWIDGET_H
#define BRANCHESWIDGET_H

#include <QFrame>

class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

class BranchesWidget : public QFrame
{
    Q_OBJECT

public:
    explicit BranchesWidget(QWidget *parent = nullptr);

private:
    void setupTree();
    void createRootItems();

    QVBoxLayout *m_layout;
    QTreeWidget *m_treeWidget;

    // Корневые элементы дерева
    QTreeWidgetItem *m_localBranchesRoot;
    QTreeWidgetItem *m_remotesRoot;
    QTreeWidgetItem *m_tagsRoot;
    QTreeWidgetItem *m_stashesRoot;
};

#endif // BRANCHESWIDGET_H
