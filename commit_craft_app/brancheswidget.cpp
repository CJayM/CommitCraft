#include "brancheswidget.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

BranchesWidget::BranchesWidget(QWidget *parent)
    : QFrame(parent)
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

void BranchesWidget::setupTree()
{
    // Устанавливаем количество колонок (1 для простоты)
    m_treeWidget->setColumnCount(1);
    
    // Создаем корневые элементы
    createRootItems();
}

void BranchesWidget::createRootItems()
{
    // Local Branches
    m_localBranchesRoot = new QTreeWidgetItem(m_treeWidget);
    m_localBranchesRoot->setText(0, "Local Branches");
    m_localBranchesRoot->setExpanded(true); // Развернут по умолчанию
    
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
