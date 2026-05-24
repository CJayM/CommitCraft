#include "brancheswidget.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QFont>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include "git.h"

BranchesWidget::BranchesWidget(QWidget *parent)
    : QFrame(parent)
    , m_git(nullptr)
    , m_contextMenuItem(nullptr)
    , m_lastFailedDeleteBranch("")
    , m_contextRemoteName("")
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    // Применяем стиль к фрейму
    setStyleSheet(R"(
        BranchesWidget {
            background-color: #ffffff;
            border: none;
        }
        QToolButton#branchesRefreshBtn {
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 2px 6px;
            font-size: 14px;
            background: transparent;
            color: #656d76;
        }
        QToolButton#branchesRefreshBtn:hover {
            background-color: #e8eaed;
            border-color: #d0d7de;
            color: #1f2328;
        }
        QLabel#branchesTitle {
            font-weight: 700;
            font-size: 12px;
            color: #656d76;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            padding: 2px 4px;
        }
    )");

    // Верхняя панель с заголовком и кнопкой Refresh
    QWidget *topBar = new QWidget(this);
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(4);

    QLabel *titleLabel = new QLabel(tr("Branches"), this);
    titleLabel->setObjectName("branchesTitle");
    topLayout->addWidget(titleLabel);

    topLayout->addStretch();

    m_refreshButton = new QToolButton(this);
    m_refreshButton->setObjectName("branchesRefreshBtn");
    m_refreshButton->setText(QString::fromUtf8("\u21BB")); // ↻
    m_refreshButton->setToolTip(tr("Refresh branches"));
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    connect(m_refreshButton, &QToolButton::clicked, this, &BranchesWidget::refresh);
    topLayout->addWidget(m_refreshButton);

    m_layout->addWidget(topBar);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setExpandsOnDoubleClick(true);
    m_treeWidget->setIndentation(20);
    m_treeWidget->setRootIsDecorated(true);
    
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &BranchesWidget::onTreeItemDoubleClicked);
    
    // Setup Context Menu
    m_contextMenu = new QMenu(this);
    m_checkoutAction = m_contextMenu->addAction(tr("Checkout"));
    m_createBranchAction = m_contextMenu->addAction(tr("Create New Branch Here"));
    m_renameBranchAction = m_contextMenu->addAction(tr("Rename"));
    m_contextMenu->addSeparator();
    m_deleteBranchAction = m_contextMenu->addAction(tr("Delete"));
    m_contextMenu->addSeparator();
    m_fetchAction = m_contextMenu->addAction(tr("Fetch"));
    m_pruneAction = m_contextMenu->addAction(tr("Prune"));
    m_contextMenu->addSeparator();
    m_applyStashAction = m_contextMenu->addAction(tr("Apply"));
    m_popStashAction = m_contextMenu->addAction(tr("Pop"));
    m_dropStashAction = m_contextMenu->addAction(tr("Drop"));
    m_contextMenu->addSeparator();
    m_showStashAction = m_contextMenu->addAction(tr("Show"));
    
    connect(m_checkoutAction, &QAction::triggered, this, &BranchesWidget::onCheckoutAction);
    connect(m_createBranchAction, &QAction::triggered, this, &BranchesWidget::onCreateBranchAction);
    connect(m_renameBranchAction, &QAction::triggered, this, &BranchesWidget::onRenameBranchAction);
    connect(m_deleteBranchAction, &QAction::triggered, this, &BranchesWidget::onDeleteBranchAction);
    connect(m_fetchAction, &QAction::triggered, this, &BranchesWidget::onFetchAction);
    connect(m_pruneAction, &QAction::triggered, this, &BranchesWidget::onPruneAction);
    connect(m_applyStashAction, &QAction::triggered, this, &BranchesWidget::onApplyStashAction);
    connect(m_popStashAction, &QAction::triggered, this, &BranchesWidget::onPopStashAction);
    connect(m_dropStashAction, &QAction::triggered, this, &BranchesWidget::onDropStashAction);
    connect(m_showStashAction, &QAction::triggered, this, &BranchesWidget::onShowStashAction);

    setupTree();
    m_layout->addWidget(m_treeWidget);
}

void BranchesWidget::setGit(Git *git)
{
    m_git = git;
    
    if (m_git) {
        connect(m_git, &Git::localBranchesReady, this, &BranchesWidget::populateLocalBranches);
        connect(m_git, &Git::remotesReady, this, &BranchesWidget::populateRemotes);
        connect(m_git, &Git::remoteBranchesReady, this, &BranchesWidget::populateRemoteBranches);
        connect(m_git, &Git::tagsReady, this, &BranchesWidget::populateTags);
        connect(m_git, &Git::stashesReady, this, &BranchesWidget::populateStashes);
        
        // Подключаем сигналы завершения операций модификации веток
        connect(m_git, &Git::branchCreated, this, [this](bool success, const QString &message) {
            if (success) {
                refresh(); // Обновляем дерево
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
        
        connect(m_git, &Git::branchDeleted, this, [this](bool success, const QString &message) {
            if (success) {
                refresh(); // Обновляем дерево
            } else {
                // Если ветка не слита, предлагаем Force Delete
                if (message.contains("not fully merged") && !m_lastFailedDeleteBranch.isEmpty()) {
                    QMessageBox::StandardButton reply = QMessageBox::question(
                        this, tr("Branch Not Merged"),
                        tr("The branch '%1' is not fully merged.\nForce delete it?").arg(m_lastFailedDeleteBranch),
                        QMessageBox::Yes | QMessageBox::No
                    );
                    
                    if (reply == QMessageBox::Yes) {
                        m_git->deleteBranch(m_lastFailedDeleteBranch, true);
                    }
                } else {
                    QMessageBox::warning(this, tr("Error"), message);
                }
            }
        });
        
        connect(m_git, &Git::branchRenamed, this, [this](bool success, const QString &message) {
            if (success) {
                refresh(); // Обновляем дерево
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
        
        // Remote signals
        connect(m_git, &Git::fetchReady, this, [this](bool success, const QString &message) {
            if (success) {
                refresh();
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
        
        connect(m_git, &Git::pruneReady, this, [this](bool success, const QString &message) {
            if (success) {
                refresh();
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
        
        // Stash signals
        connect(m_git, &Git::stashApplied, this, [this](bool success, const QString &message) {
            if (success) {
                refresh();
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
        
        connect(m_git, &Git::stashDropped, this, [this](bool success, const QString &message) {
            if (success) {
                refresh();
            } else {
                QMessageBox::warning(this, tr("Error"), message);
            }
        });
    }
}

void BranchesWidget::clear()
{
    clearChildren(m_localBranchesRoot);
    clearChildren(m_remotesRoot);
    clearChildren(m_tagsRoot);
    clearChildren(m_stashesRoot);

    m_localBranchesRoot->setText(0, "Local Branches");
    m_remotesRoot->setText(0, "Remotes");
    m_tagsRoot->setText(0, "Tags");
    m_stashesRoot->setText(0, "Stashes");

    m_currentBranchName.clear();
}

void BranchesWidget::refresh()
{
    if (!m_git)
        return;

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
    m_localBranchesRoot = new QTreeWidgetItem(m_treeWidget);
    m_localBranchesRoot->setText(0, "Local Branches");
    m_localBranchesRoot->setExpanded(true);

    m_remotesRoot = new QTreeWidgetItem(m_treeWidget);
    m_remotesRoot->setText(0, "Remotes");
    m_remotesRoot->setExpanded(false);

    m_tagsRoot = new QTreeWidgetItem(m_treeWidget);
    m_tagsRoot->setText(0, "Tags");
    m_tagsRoot->setExpanded(false);

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
    m_currentBranchName = currentBranch;
    
    m_localBranchesRoot->setText(0, QString("Local Branches (%1)").arg(branches.size()));
    
    for (const QString &branch : branches) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_localBranchesRoot);
        item->setText(0, branch);
        item->setData(0, Qt::UserRole, "branch");
        
        if (branch == currentBranch) {
            QFont font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
            item->setText(0, "● " + branch);
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

    if (m_git) {
        m_git->getRemoteBranches(""); 
    }
}

void BranchesWidget::populateRemoteBranches(const QString &remote, const QList<QString> &branches)
{
    Q_UNUSED(remote);
    
    for (const QString &fullBranch : branches) {
        QStringList parts = fullBranch.split('/');
        if (parts.size() >= 2) {
            QString remoteName = parts[0];
            QString branchName = fullBranch.mid(remoteName.length() + 1);
            
            QTreeWidgetItem *remoteItem = nullptr;
            for (int i = 0; i < m_remotesRoot->childCount(); ++i) {
                QTreeWidgetItem *child = m_remotesRoot->child(i);
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
    
    for (int i = 0; i < m_remotesRoot->childCount(); ++i) {
        QTreeWidgetItem *item = m_remotesRoot->child(i);
        int count = item->childCount();
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

    if (type == "branch") {
        branchName = item->text(0);
        if (branchName.startsWith("● ")) branchName = branchName.mid(2);
    } else if (type == "remote_branch") {
        QTreeWidgetItem *parent = item->parent();
        if (parent && parent->data(0, Qt::UserRole).toString() == "remote") {
            QString remoteName = parent->text(0);
            int bracketPos = remoteName.indexOf(" (");
            if (bracketPos != -1) remoteName = remoteName.left(bracketPos);
            branchName = remoteName + "/" + item->text(0);
        }
    }

    if (!branchName.isEmpty()) {
        emit checkoutAttempted(branchName);
        m_git->checkoutBranch(branchName);
    }
}

void BranchesWidget::contextMenuEvent(QContextMenuEvent *event)
{
    // Передаем глобальные координаты для корректного преобразования
    QTreeWidgetItem *item = getBranchItemUnderCursor(event->globalPos());
    m_contextMenuItem = item;

    if (!item) return;

    QString type = item->data(0, Qt::UserRole).toString();
    bool isCurrentBranch = false;
    
    if (type == "branch") {
        QString branchName = item->text(0);
        if (branchName.startsWith("● ")) branchName = branchName.mid(2);
        isCurrentBranch = (branchName == m_currentBranchName);
    }

    if (type == "branch") {
        m_checkoutAction->setVisible(!isCurrentBranch);
        m_checkoutAction->setEnabled(!isCurrentBranch);
        
        m_createBranchAction->setVisible(true);
        m_renameBranchAction->setVisible(!isCurrentBranch);
        m_renameBranchAction->setEnabled(!isCurrentBranch);
        
        m_deleteBranchAction->setVisible(!isCurrentBranch);
        m_deleteBranchAction->setEnabled(!isCurrentBranch);
        
        m_fetchAction->setVisible(false);
        m_pruneAction->setVisible(false);
    } else if (type == "remote") {
        // Для Remote показываем только Fetch/Prune
        m_checkoutAction->setVisible(false);
        m_createBranchAction->setVisible(false);
        m_renameBranchAction->setVisible(false);
        m_deleteBranchAction->setVisible(false);
        
        m_fetchAction->setVisible(true);
        m_pruneAction->setVisible(true);
        
        // Сохраняем имя remote
        m_contextRemoteName = item->text(0);
        int bracketPos = m_contextRemoteName.indexOf(" (");
        if (bracketPos != -1) m_contextRemoteName = m_contextRemoteName.left(bracketPos);
    } else if (type == "stash") {
        // Для Stash показываем Apply/Pop/Drop/Show
        m_checkoutAction->setVisible(false);
        m_createBranchAction->setVisible(false);
        m_renameBranchAction->setVisible(false);
        m_deleteBranchAction->setVisible(false);
        m_fetchAction->setVisible(false);
        m_pruneAction->setVisible(false);
        
        m_applyStashAction->setVisible(true);
        m_popStashAction->setVisible(true);
        m_dropStashAction->setVisible(true);
        m_showStashAction->setVisible(true);
        
        // Сохраняем ссылку на stash (первая часть строки, например "stash@{0}")
        QString stashText = item->text(0);
        int colonPos = stashText.indexOf(':');
        if (colonPos != -1) {
            m_contextStashRef = stashText.left(colonPos).trimmed();
        } else {
            m_contextStashRef = stashText;
        }
    } else {
        m_checkoutAction->setVisible(false);
        m_createBranchAction->setVisible(false);
        m_renameBranchAction->setVisible(false);
        m_deleteBranchAction->setVisible(false);
        m_fetchAction->setVisible(false);
        m_pruneAction->setVisible(false);
        return;
    }

    m_contextMenu->exec(event->globalPos());
}

QTreeWidgetItem* BranchesWidget::getBranchItemUnderCursor(const QPoint &globalPos) const
{
    // Преобразуем глобальные координаты в локальные координаты дерева
    QPoint localPos = m_treeWidget->viewport()->mapFromGlobal(globalPos);
    
    // Проверяем, находится ли точка внутри области дерева
    if (!m_treeWidget->viewport()->rect().contains(localPos)) {
        return nullptr;
    }

    QTreeWidgetItem *item = m_treeWidget->itemAt(localPos);
    if (!item) return nullptr;
    
    QString type = item->data(0, Qt::UserRole).toString();
    if (type == "branch" || type == "remote" || type == "stash") return item;
    return nullptr;
}

void BranchesWidget::onCheckoutAction()
{
    if (!m_contextMenuItem || !m_git) return;
    QString branchName = m_contextMenuItem->text(0);
    if (branchName.startsWith("● ")) branchName = branchName.mid(2);
    
    emit checkoutAttempted(branchName);
    m_git->checkoutBranch(branchName);
}

void BranchesWidget::onCreateBranchAction()
{
    if (!m_contextMenuItem || !m_git) return;
    QString baseBranch = m_contextMenuItem->text(0);
    if (baseBranch.startsWith("● ")) baseBranch = baseBranch.mid(2);

    bool ok;
    QString newBranchName = QInputDialog::getText(this, tr("Create New Branch"),
                                                  tr("New branch name (from \"%1\"):").arg(baseBranch),
                                                  QLineEdit::Normal, "", &ok);
    if (ok && !newBranchName.isEmpty()) {
        m_git->createBranch(newBranchName, baseBranch);
    }
}

void BranchesWidget::onRenameBranchAction()
{
    if (!m_contextMenuItem || !m_git) return;
    QString oldName = m_contextMenuItem->text(0);
    if (oldName.startsWith("● ")) oldName = oldName.mid(2);

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename Branch"),
                                            tr("New name for \"%1\":").arg(oldName),
                                            QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        m_git->renameBranch(oldName, newName);
    }
}

void BranchesWidget::onDeleteBranchAction()
{
    if (!m_contextMenuItem || !m_git) return;
    QString branchName = m_contextMenuItem->text(0);
    if (branchName.startsWith("● ")) branchName = branchName.mid(2);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Delete Branch"),
        tr("Are you sure you want to delete branch \"%1\"?").arg(branchName),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_lastFailedDeleteBranch = branchName;
        m_git->deleteBranch(branchName, false);
    }
}

void BranchesWidget::onFetchAction()
{
    if (!m_git || m_contextRemoteName.isEmpty()) return;
    m_git->fetchRemote(m_contextRemoteName);
}

void BranchesWidget::onPruneAction()
{
    if (!m_git || m_contextRemoteName.isEmpty()) return;
    m_git->pruneRemote(m_contextRemoteName);
}

void BranchesWidget::onApplyStashAction()
{
    if (!m_git || m_contextStashRef.isEmpty()) return;
    m_git->applyStash(m_contextStashRef, false); // false = apply only, don't drop
}

void BranchesWidget::onPopStashAction()
{
    if (!m_git || m_contextStashRef.isEmpty()) return;
    m_git->applyStash(m_contextStashRef, true); // true = pop (apply + drop)
}

void BranchesWidget::onDropStashAction()
{
    if (!m_git || m_contextStashRef.isEmpty()) return;
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Drop Stash"),
        tr("Are you sure you want to drop stash \"%1\"?").arg(m_contextStashRef),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        m_git->dropStash(m_contextStashRef);
    }
}

void BranchesWidget::onShowStashAction()
{
    if (!m_git || m_contextStashRef.isEmpty()) return;
    // TODO: Show stash diff in a dialog or panel
    QMessageBox::information(this, tr("Show Stash"), 
        tr("Showing stash \"%1\" is not implemented yet.\nDiff would be shown here.").arg(m_contextStashRef));
}
