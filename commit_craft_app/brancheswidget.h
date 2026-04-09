#ifndef BRANCHESWIDGET_H
#define BRANCHESWIDGET_H

#include <QFrame>

class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class Git;

class BranchesWidget : public QFrame
{
    Q_OBJECT

public:
    explicit BranchesWidget(QWidget *parent = nullptr);
    
    /// Установить Git-объект для получения данных
    void setGit(Git *git);
    
    /// Обновить все данные дерева
    void refresh();

signals:
    /// Сигнал о том, что ветка была изменена (checkout)
    void branchChanged(const QString &branchName);
    
    /// Сигнал попытки checkout (для сохранения имени ветки в MainWindow)
    void checkoutAttempted(const QString &branchName);

public slots:
    void populateLocalBranches(const QList<QString> &branches, const QString &currentBranch);
    void populateRemotes(const QList<QString> &remotes);
    void populateRemoteBranches(const QString &remote, const QList<QString> &branches);
    void populateTags(const QList<QString> &tags);
    void populateStashes(const QList<QString> &stashes);

private:
    void setupTree();
    void createRootItems();
    void clearChildren(QTreeWidgetItem *parent);
    
    /// Обработка двойного клика по элементу дерева
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

    QVBoxLayout *m_layout;
    QTreeWidget *m_treeWidget;
    Git *m_git;

    // Корневые элементы дерева
    QTreeWidgetItem *m_localBranchesRoot;
    QTreeWidgetItem *m_remotesRoot;
    QTreeWidgetItem *m_tagsRoot;
    QTreeWidgetItem *m_stashesRoot;
};

#endif // BRANCHESWIDGET_H
