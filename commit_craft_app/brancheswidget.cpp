#include "brancheswidget.h"
#include <QTreeWidget>
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
    
    m_layout->addWidget(m_treeWidget);
}
