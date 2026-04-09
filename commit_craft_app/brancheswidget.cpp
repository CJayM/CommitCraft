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
    
    // Подключаем обработку двойного клика
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &BranchesWidget::onTreeItemDoubleClicked);
    
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
    }

    // Запрашиваем ВСЕ удаленные ветки одним запросом
    if (m_git) {
        m_git->getRemoteBranches(""); 
    }
}

void BranchesWidget::populateRemoteBranches(const QString &remote, const QList<QString> &branches)
{
    Q_UNUSED(remote);
    // Если remote пустой, значит нам пришли ВСЕ ветки (формат "remote/branch")
    // Нам нужно распределить их по узлам.
    
    for (const QString &fullBranch : branches) {
        QStringList parts = fullBranch.split('/');
        if (parts.size() >= 2) {
            QString remoteName = parts[0];
            QString branchName = fullBranch.mid(remoteName.length() + 1);
            
            // Ищем узел remote
            QTreeWidgetItem *remoteItem = nullptr;
            for (int i = 0; i < m_remotesRoot->childCount(); ++i) {
                QTreeWidgetItem *child = m_remotesRoot->child(i);
                // Ищем по точному совпадению имени
                if (child->text(0) == remoteName) {
                    remoteItem = child;
                    break;
                }
            }
            
            if (remoteItem) {
                QTreeWidgetItem *branchItem = new QTreeWidgetItem(remoteItem);
                branchItem->setText(0, branchName);
                branchItem->setData(0, Qt::UserRole, "remote_branch");
            }
        }
    }
    
    // Обновляем заголовки remote-узлов с количеством веток
    for (int i = 0; i < m_remotesRoot->childCount(); ++i) {
        QTreeWidgetItem *item = m_remotesRoot->child(i);
        int count = item->childCount();
        // Текст сейчас содержит чистое имя remote (из populateRemotes)
        item->setText(0, QString("%1 (%2)").arg(item->text(0), QString::number(count)));
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

void BranchesWidget::onTreeItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !m_git)
        return;

    QString type = item->data(0, Qt::UserRole).toString();
    QString branchName;

    // Определяем имя ветки в зависимости от типа элемента
    if (type == "branch") {
        // Локальная ветка: убираем маркер текущей ветки если есть
        branchName = item->text(0);
        if (branchName.startsWith("● ")) {
            branchName = branchName.mid(2);
        }
    } else if (type == "remote_branch") {
        // Remote ветка: нужно добавить префикс remote
        // Находим родительский узел (remote name)
        QTreeWidgetItem *parent = item->parent();
        if (parent && parent->data(0, Qt::UserRole).toString() == "remote") {
            QString remoteName = parent->text(0);
            // Убираем счетчик в скобках если есть
            int bracketPos = remoteName.indexOf(" (");
            if (bracketPos != -1) {
                remoteName = remoteName.left(bracketPos);
            }
            branchName = remoteName + "/" + item->text(0);
        }
    }

    if (!branchName.isEmpty()) {
        emit checkoutAttempted(branchName);
        m_git->checkoutBranch(branchName);
    }
}
