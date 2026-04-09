#ifndef BRANCHESWIDGET_H
#define BRANCHESWIDGET_H

#include <QAction>
#include <QFrame>
#include <QMenu>
#include <QToolButton>

class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class QContextMenuEvent;
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
    
    /// Контекстное меню
    void contextMenuEvent(QContextMenuEvent *event) override;
    
    /// Возвращает элемент ветки под курсором, если есть (принимает глобальные координаты)
    QTreeWidgetItem* getBranchItemUnderCursor(const QPoint &globalPos) const;
    
    // Context menu slots
    void onCheckoutAction();
    void onCreateBranchAction();
    void onDeleteBranchAction();
    void onRenameBranchAction();
    void onFetchAction();
    void onPruneAction();
    void onApplyStashAction();
    void onPopStashAction();
    void onDropStashAction();
    void onShowStashAction();

    QVBoxLayout *m_layout;
    QToolButton *m_refreshButton; // Кнопка обновления
    QTreeWidget *m_treeWidget;
    Git *m_git;
    
    // Текущий элемент для контекстного меню
    QTreeWidgetItem *m_contextMenuItem;
    QString m_currentBranchName; // Для проверки, можно ли удалить/переименовать
    QString m_lastFailedDeleteBranch; // Для повторной попытки Force Delete

    // Корневые элементы дерева
    QTreeWidgetItem *m_localBranchesRoot;
    QTreeWidgetItem *m_remotesRoot;
    QTreeWidgetItem *m_tagsRoot;
    QTreeWidgetItem *m_stashesRoot;
    
    // Context menu
    QMenu *m_contextMenu;
    QAction *m_checkoutAction;
    QAction *m_createBranchAction;
    QAction *m_deleteBranchAction;
    QAction *m_renameBranchAction;
    QAction *m_fetchAction;
    QAction *m_pruneAction;
    QAction *m_applyStashAction;
    QAction *m_popStashAction;
    QAction *m_dropStashAction;
    QAction *m_showStashAction;
    
    QString m_contextRemoteName; // Для операций с remote
    QString m_contextStashRef;   // Для операций со stash
};

#endif // BRANCHESWIDGET_H
