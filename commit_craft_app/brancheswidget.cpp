#include "brancheswidget.h"
#include "git.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QFont>

BranchesWidget::BranchesWidget(QWidget *parent)
    : QFrame(parent)
    , m_git(nullptr)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setExpandsOnDoubleClick(true);
    m_treeWidget->setIndentation(20);
    m_treeWidget->setRootIsDecorated(true);
    
    setupTree();
    
    m_layout->addWidget(m_treeWidget);
}

void BranchesWidget::setGit(Git *git)
{
    m_git = git;
    
    // Подключаем сигналы Git к слотам виджета
    if (m_git) {
        connect(m_git, &Git::localBranchesReady, this, &BranchesWidget::populateLocalBranches);
        connect(m_git, &Git::remotesReady, this, &BranchesWidget::populateRemotes);
        connect(m_git, &Git::remoteBranchesReady, this, &BranchesWidget::populateRemoteBranches);
        connect(m_git, &Git::tagsReady, this, &BranchesWidget::populateTags);
        connect(m_git, &Git::stashesReady, this, &BranchesWidget::populateStashes);
    }
}

void BranchesWidget::refresh()
{
    if (!m_git)
        return;

    // Запрашиваем все данные асинхронно
    m_git->getLocalBranches();
    m_git->getRemotes();
    m_git->getTags();
    m_git->getStashes();
}

void BranchesWidget::setupTree()
{
    m_treeWidget->setColumnCount(1);
    createRootItems();
}

void BranchesWidget::createRootItems()
{
    // Local Branches
    m_localBranchesRoot = new QTreeWidgetItem(m_treeWidget);
    m_localBranchesRoot->setText(0, "Local Branches");
    m_localBranchesRoot->setExpanded(true);
    
    // Remotes
    m_remotesRoot = new QTreeWidgetItem(m_treeWidget);
    m_remotesRoot->setText(0, "Remotes");
    m_remotesRoot->setExpanded(false);
    
    // Tags
    m_tagsRoot = new QTreeWidgetItem(m_treeWidget);
    m_tagsRoot->setText(0, "Tags");
    m_tagsRoot->setExpanded(false);
    
    // Stashes
    m_stashesRoot = new QTreeWidgetItem(m_treeWidget);
    m_stashesRoot->setText(0, "Stashes");
    m_stashesRoot->setExpanded(false);
}

void BranchesWidget::clearChildren(QTreeWidgetItem *parent)
{
    while (parent->childCount() > 0) {
        delete parent->takeChild(0);
    }
}

void BranchesWidget::populateLocalBranches(const QList<QString> &branches, const QString &currentBranch)
{
    clearChildren(m_localBranchesRoot);
    
    // Обновляем заголовок с количеством
    m_localBranchesRoot->setText(0, QString("Local Branches (%1)").arg(branches.size()));
    
    for (const QString &branch : branches) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_localBranchesRoot);
        item->setText(0, branch);
        item->setData(0, Qt::UserRole, "branch"); // Тип для идентификации
        
        // Подсвечиваем текущую ветку жирным
        if (branch == currentBranch) {
            QFont font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
            item->setText(0, "● " + branch); // Маркер текущей ветки
        }
    }
}

void BranchesWidget::populateRemotes(const QList<QString> &remotes)
{
    clearChildren(m_remotesRoot);
    
    if (remotes.isEmpty()) {
        m_remotesRoot->setText(0, "Remotes");
        return;
    }
    
    m_remotesRoot->setText(0, QString("Remotes (%1)").arg(remotes.size()));
    
    for (const QString &remote : remotes) {
        QTreeWidgetItem *remoteItem = new QTreeWidgetItem(m_remotesRoot);
        remoteItem->setText(0, remote);
        remoteItem->setData(0, Qt::UserRole, "remote");
        remoteItem->setExpanded(false);
        
        // Запрашиваем ветки для этого remote
        if (m_git) {
            m_git->getRemoteBranches(remote);
        }
    }
}

void BranchesWidget::populateRemoteBranches(const QString &remote, const QList<QString> &branches)
{
    // Находим узел remote в дереве
    for (int i = 0; i < m_remotesRoot->childCount(); ++i) {
        QTreeWidgetItem *remoteItem = m_remotesRoot->child(i);
        if (remoteItem->text(0) == remote) {
            clearChildren(remoteItem);
            
            for (const QString &branch : branches) {
                QTreeWidgetItem *branchItem = new QTreeWidgetItem(remoteItem);
                // Убираем префикс remote/ из названия
                QString displayName = branch;
                if (displayName.startsWith(remote + "/")) {
                    displayName = displayName.mid(remote.length() + 1);
                }
                branchItem->setText(0, displayName);
                branchItem->setData(0, Qt::UserRole, "remote_branch");
            }
            
            // Обновляем заголовок remote с количеством веток
            remoteItem->setText(0, QString("%1 (%2)").arg(remote, QString::number(branches.size())));
            break;
        }
    }
}

void BranchesWidget::populateTags(const QList<QString> &tags)
{
    clearChildren(m_tagsRoot);
    
    m_tagsRoot->setText(0, QString("Tags (%1)").arg(tags.size()));
    
    for (const QString &tag : tags) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_tagsRoot);
        item->setText(0, tag);
        item->setData(0, Qt::UserRole, "tag");
    }
}

void BranchesWidget::populateStashes(const QList<QString> &stashes)
{
    clearChildren(m_stashesRoot);
    
    m_stashesRoot->setText(0, QString("Stashes (%1)").arg(stashes.size()));
    
    for (const QString &stash : stashes) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_stashesRoot);
        item->setText(0, stash);
        item->setData(0, Qt::UserRole, "stash");
    }
}
