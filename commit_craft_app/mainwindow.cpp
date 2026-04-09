#include "mainwindow.h"
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <QGuiApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include "./ui_mainwindow.h"
#include "codeeditor.h"
#include "diffeditor.h"
#include <commithistorymodel.h>
#include <commititemdelegate.h>
#include <filemodel.h>
#include <git.h>
#include <gitparser.h>
#include "settingsdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("CommitCraft", "MainWindow"))
    , settingsDialog(nullptr)
    , repositoryPath(QDir::currentPath())
    , diffEditor(nullptr)
    , unstagedFilesModel(new FileModel(this))
    , stagedFilesModel(new FileModel(this))
    , commitHistoryModel(new CommitHistoryModel(this))
    , commitItemDelegate(new CommitItemDelegate(this))
    , git(new Git(this))
    , m_lastSelectedFileName("")
    , m_lastSelectionSource(SelectionSource::Unstaged)
    , m_fsWatcher(new QFileSystemWatcher(this))
    , m_fsDebounceTimer(new QTimer(this))
{
    ui->setupUi(this);
    restoreSplitterState();
    
    // Load repository path from settings
    QVariant savedRepoPath = settings->value("repositoryPath");
    if (savedRepoPath.isValid() && !savedRepoPath.toString().isEmpty()) {
        QString path = savedRepoPath.toString();
        // Check if the saved directory is still a Git repository
        if (isGitRepository(path)) {
            repositoryPath = path;
        }
    }
    
    // Set repository path for Git operations
    git->setRepositoryPath(repositoryPath);

    // Setup file system watcher for auto-refresh
    setupFileSystemWatcher();

    // DiffEditor создан через promoted widget в mainwindow.ui
    // Получаем ссылатель на него
    diffEditor = ui->diffEditor;

    // Применяем сохранённые настройки шрифта
    QPair<QString, int> fontSettings = loadFontSettings();
    diffEditor->applyFontSettings(fontSettings.first, fontSettings.second);

    // Set up the table views with models
    ui->filesTable->setModel(unstagedFilesModel);
    ui->stagedFilesTable->setModel(stagedFilesModel);
    ui->commitHistoryList->setModel(commitHistoryModel);
    ui->commitHistoryList->setItemDelegate(commitItemDelegate);

    // Фиксированная ширина колонки статуса (только для символа)
    ui->filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->filesTable->setColumnWidth(0, 20);
    ui->filesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->filesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    ui->stagedFilesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->stagedFilesTable->setColumnWidth(0, 20);
    ui->stagedFilesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->stagedFilesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);

    // Connect git process signals to DiffEditor
    connect(git, &Git::diffReady, this, [this](const QString &diffOutput) {
        GitParser parser;
        QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
        diffEditor->applyDiffData(hunks);

        // Обработка отложенной навигации
        if (m_pendingNavigation == PendingNavigation::GoNext) {
            diffEditor->navigateToFirstHunk();
            m_pendingNavigation = PendingNavigation::None;
        } else if (m_pendingNavigation == PendingNavigation::GoPrev) {
            diffEditor->navigateToLastHunk();
            m_pendingNavigation = PendingNavigation::None;
        }
        
        // Обновляем состояние кнопок навигации
        updateNavigationButtonsState();
    });

    // Connect hunk navigation buttons to DiffEditor
    connect(ui->actionPrevHunk, &QAction::triggered, this, &MainWindow::navigateToPrevHunk);
    connect(ui->actionNextHunk, &QAction::triggered, this, &MainWindow::navigateToNextHunk);
    
    // Set default actions for hunk navigation buttons
    ui->prevHunkButton->setDefaultAction(ui->actionPrevHunk);
    ui->nextHunkButton->setDefaultAction(ui->actionNextHunk);

    // Подключаем BranchesWidget к Git
    ui->branchesWidget->setGit(git);

    // Сохраняем имя ветки при попытке checkout
    connect(ui->branchesWidget, &BranchesWidget::checkoutAttempted, this, [this](const QString &branch) {
        m_lastCheckoutBranch = branch;
    });

    // Подключаем сигнал checkout для обновления интерфейса
    connect(git, &Git::checkoutReady, this, [this](bool success, const QString &message) {
        if (success) {
            // Обновляем панель веток, filesTable и diff
            ui->branchesWidget->refresh();
            refreshGitStatus();
        } else {
            // Проверяем, можно ли заstashить изменения
            if ((message.contains("would be overwritten by checkout") ||
                 message.contains("Please commit your changes or stash them")) && !m_lastCheckoutBranch.isEmpty()) {

                QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    tr("Несохраненные изменения"),
                    tr("У вас есть несохраненные изменения. Заstashить их и переключиться на ветку \"%1\"?").arg(m_lastCheckoutBranch),
                    QMessageBox::Yes | QMessageBox::No
                );

                if (reply == QMessageBox::Yes) {
                    git->checkoutBranch(m_lastCheckoutBranch, true);
                }
            } else {
                QMessageBox::warning(this, tr("Ошибка Git"), message);
            }
        }
    });

    // Подключаем сигнал создания stash для обновления интерфейса
    connect(git, &Git::stashCreated, this, [this](bool success, const QString &message) {
        if (success) {
            // Обновляем panel веток (чтобы обновить список stash) и filesTable
            ui->branchesWidget->refresh();
            refreshGitStatus();
        } else {
            QMessageBox::warning(this, tr("Ошибка Git"), message);
        }
    });
    
    // Если репозиторий загружен из настроек, обновляем панель веток
    if (!repositoryPath.isEmpty() && isGitRepository(repositoryPath)) {
        ui->branchesWidget->refresh();
    }
    
    // Connect the menu actions to their slots
    connect(ui->actionOpenRepository, &QAction::triggered, this, &MainWindow::openRepository);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(ui->actionCommit, &QAction::triggered, this, &MainWindow::commitChanges);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::refreshGitStatus);
    
    // Connect panel toggle actions
    connect(ui->actionToggleLeftPanel, &QAction::toggled, this, &MainWindow::toggleLeftPanel);
    connect(ui->actionToggleTopPanel, &QAction::toggled, this, &MainWindow::toggleTopPanel);
    
    // Connect amend checkbox
    connect(ui->amend_chk, &QCheckBox::stateChanged, this, &MainWindow::onAmendCheckBoxChanged);
    
    // Set default action for refresh button
    ui->refreshButton->setDefaultAction(ui->actionRefresh);
    
    // Connect commit button to the same action
    ui->commitButton->setDefaultAction(ui->actionCommit);
    
    // Connect table selection changes
    connect(ui->filesTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onFileTableSelectionChanged);
    connect(ui->stagedFilesTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onStagedFileTableSelectionChanged);
    
    // Connect double-click to open file
    connect(ui->filesTable, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        if (index.isValid() && index.column() >= 0) {
            FileModel *model = qobject_cast<FileModel*>(ui->filesTable->model());
            if (model) {
                openFile(model->getFileName(index.row()));
            }
        }
    });
    connect(ui->stagedFilesTable, &QTableView::doubleClicked, this, [this](const QModelIndex &index) {
        if (index.isValid() && index.column() >= 0) {
            FileModel *model = qobject_cast<FileModel*>(ui->stagedFilesTable->model());
            if (model) {
                openFile(model->getFileName(index.row()));
            }
        }
    });

    connect(ui->commitMessageTextEdit,
            &QTextEdit::textChanged,
            this,
            &MainWindow::updateCommitButtonState);

    // Connect git process signals
    connect(git, &Git::statusReady, this, &MainWindow::onGitStatusFinished);
    connect(git, &Git::diffReady, this, &MainWindow::onGitDiffReady);
    connect(git, &Git::commitHistoryReady, this, &MainWindow::onGitCommitHistoryReady);
    connect(git, &Git::commitReady, this, &MainWindow::onGitCommitFinished);
    connect(git, &Git::addFileReady, this, &MainWindow::refreshGitStatus);
    connect(git, &Git::unstageFileReady, this, &MainWindow::refreshGitStatus);
    connect(git, &Git::error, this, &MainWindow::onGitError);
    
    // Set up context menus
    ui->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->filesTable, &QTableView::customContextMenuRequested, this, &MainWindow::showFileContextMenu);
    
    ui->stagedFilesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->stagedFilesTable, &QTableView::customContextMenuRequested, this, &MainWindow::showStagedFileContextMenu);
    
    // Set initial state of commit action and button
    ui->actionCommit->setEnabled(false);
    ui->commitButton->setEnabled(false);
    
    // Set initial visibility of staged label and table
    ui->stagedLabel->setVisible(false);
    ui->stagedFilesTable->setVisible(false);
    
    // Connect action changes to update button state
    connect(ui->actionCommit, &QAction::changed, this, [this]() {
        ui->commitButton->setEnabled(ui->actionCommit->isEnabled());
    });
    
    // Set window title to show current repository
    setWindowTitle(QString("Commit Craft - %1").arg(repositoryPath));
    
    // Load initial git status and commit history
    refreshGitStatus();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;
    if (settingsDialog) {
        delete settingsDialog;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSplitterState();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSplitterState()
{
    settings->setValue("splitterSizes", ui->splitter->saveState());
    settings->setValue("leftSplitterSizes", ui->leftSplitter->saveState());    
    settings->setValue("topSplitterSizes", ui->topSplitter->saveState());
}

void MainWindow::restoreSplitterState()
{
    if (settings->contains("splitterSizes")) {
        ui->splitter->restoreState(settings->value("splitterSizes").toByteArray());
    }
    if (settings->contains("leftSplitterSizes")) {
        ui->leftSplitter->restoreState(settings->value("leftSplitterSizes").toByteArray());
    }
    if (settings->contains("topSplitterSizes")) {
        ui->topSplitter->restoreState(settings->value("topSplitterSizes").toByteArray());
    }
}

void MainWindow::openSettingsDialog()
{
    if (!settingsDialog) {
        settingsDialog = new SettingsDialog(this);
    }

    if (settingsDialog->exec() == QDialog::Accepted) {
        // Применяем новые настройки шрифта к DiffEditor
        diffEditor->applyFontSettings(settingsDialog->getFontFamily(), settingsDialog->getFontSize());
    }
}

void MainWindow::refreshGitStatus()
{
    git->getStatus();
}

void MainWindow::onGitStatusFinished(const QString &output)
{
    // Parse status and update file models
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QList<QPair<QString, QString>> unstagedFiles;
    QList<QPair<QString, QString>> stagedFiles;

    for (const QString &line : lines) {
        if (line.length() < 4) continue;
        auto indexedStatus = line.left(1);
        auto workStatus = line.mid(1, 1);
        QString file = line.mid(3);
        if (indexedStatus != " " && indexedStatus != "?") {
            stagedFiles.append(qMakePair(indexedStatus, file));
        }
        if (workStatus != " ") {
            unstagedFiles.append(qMakePair(workStatus, file));
        }
    }

    unstagedFilesModel->setFiles(unstagedFiles);
    stagedFilesModel->setFiles(stagedFiles);
    updateCommitButtonState();

    // Включаем watcher обратно после обновления
    m_fsWatcher->blockSignals(false);

    git->getCommitHistory();
}

void MainWindow::onGitDiffReady(const QString &output)
{
    // Diff-данные применяются через lambda в конструкторе
    Q_UNUSED(output);
}

void MainWindow::onGitCommitHistoryReady(const QList<QList<QString>> &commits)
{
    commitHistoryModel->setCommits(commits);
}

void MainWindow::onGitCommitFinished(bool success, const QString &message)
{
    if (success) {
        // Success - clear the commit message and refresh the git status
        ui->commitMessageTextEdit->clear();
        // Uncheck amend checkbox after successful commit
        ui->amend_chk->setChecked(false);
        refreshGitStatus();
    } else {
        // Error - show message
        QMessageBox::warning(this, tr("Ошибка Git"), message);
    }
}

void MainWindow::onGitError(const QString &error)
{
    QMessageBox::warning(this, "Ошибка Git", error);
}

void MainWindow::openRepository()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите Git репозиторий"),
                                                    repositoryPath,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        // Check if the selected directory is a Git repository
        if (isGitRepository(dir)) {
            repositoryPath = dir;
            // Save the repository path to settings
            settings->setValue("repositoryPath", repositoryPath);
            setWindowTitle(QString("Commit Craft - %1").arg(repositoryPath));
            
            // Обновляем watcher для нового репозитория
            m_fsWatcher->removePaths(m_fsWatcher->directories());
            m_fsWatcher->addPath(repositoryPath);

            refreshGitStatus();
            
            // Обновляем панель веток
            ui->branchesWidget->refresh();
        } else {
            QMessageBox::warning(this, tr("Ошибка"), 
                                tr("Выбранная директория не является Git репозиторием."));
        }
    }
}

bool MainWindow::isGitRepository(const QString &path)
{
    QDir dir(path);
    return dir.exists(".git");
}

void MainWindow::showFileContextMenu(const QPoint &pos)
{
    QModelIndexList selectedIndexes = ui->filesTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;

    FileModel *model = qobject_cast<FileModel*>(ui->filesTable->model());
    if (!model) return;

    QStringList selectedFiles;
    for (const QModelIndex &index : selectedIndexes) {
        selectedFiles << model->getFileName(index.row());
    }

    QMenu contextMenu(this);

    // Stage Action
    QString actionText = selectedFiles.size() > 1 ? tr("Добавить выделенные (%1)").arg(selectedFiles.size()) : tr("Добавить");
    QAction *addAction = new QAction(actionText, this);
    connect(addAction, &QAction::triggered, this, [this, selectedFiles]() { stageSelectedFiles(selectedFiles); });
    contextMenu.addAction(addAction);

    contextMenu.addSeparator();

    if (selectedFiles.size() == 1) {
        QString fileName = selectedFiles.first();
        QAction *copyPathAction = new QAction(tr("Копировать путь"), this);
        connect(copyPathAction, &QAction::triggered, this, [this, fileName]() { copyFilePath(fileName); });
        contextMenu.addAction(copyPathAction);

        QAction *openFileAction = new QAction(tr("Открыть файл"), this);
        connect(openFileAction, &QAction::triggered, this, [this, fileName]() { openFile(fileName); });
        contextMenu.addAction(openFileAction);

        QAction *openFolderAction = new QAction(tr("Открыть папку"), this);
        connect(openFolderAction, &QAction::triggered, this, [this, fileName]() { openFolder(fileName); });
        contextMenu.addAction(openFolderAction);

        QAction *deleteAction = new QAction(tr("Удалить"), this);
        connect(deleteAction, &QAction::triggered, this, [this, fileName]() { deleteFile(fileName); });
        contextMenu.addAction(deleteAction);

        QAction *blameAction = new QAction(tr("Blame"), this);
        blameAction->setEnabled(false);
        contextMenu.addAction(blameAction);

        contextMenu.addSeparator();

        QAction *stashAction = new QAction(tr("Спрятать в Stash"), this);
        connect(stashAction, &QAction::triggered, this, [this, fileName]() {
            bool ok;
            QString message = QInputDialog::getText(this, tr("Create Stash"),
                                                     tr("Stash message:"),
                                                     QLineEdit::Normal,
                                                     tr("WIP on %1").arg(fileName),
                                                     &ok);
            if (ok && !message.isEmpty()) {
                git->createStash({fileName}, message);
            }
        });
        contextMenu.addAction(stashAction);
    } else {
        // Multi-file actions
        QAction *deleteAction = new QAction(tr("Удалить выделенные"), this);
        connect(deleteAction, &QAction::triggered, this, [this, selectedFiles]() { deleteSelectedFiles(selectedFiles); });
        contextMenu.addAction(deleteAction);

        contextMenu.addSeparator();

        QAction *stashAction = new QAction(tr("Спрятать выделенные в Stash"), this);
        connect(stashAction, &QAction::triggered, this, [this, selectedFiles]() { stashSelectedFiles(selectedFiles); });
        contextMenu.addAction(stashAction);
    }

    contextMenu.exec(ui->filesTable->viewport()->mapToGlobal(pos));
}

void MainWindow::stageSelectedFiles(const QStringList &files)
{
    if (files.isEmpty()) return;

    m_fsWatcher->blockSignals(true);
    // Используем метод addFiles, который принимает список и выполняет одну команду git add
    git->addFiles(files);
}

void MainWindow::deleteSelectedFiles(const QStringList &files)
{
    if (files.isEmpty()) return;

    QString message = files.size() > 1 
        ? tr("Вы уверены, что хотите удалить %1 выделенных файлов?").arg(files.size())
        : tr("Вы уверены, что хотите удалить файл \"%1\"?").arg(files.first());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Удалить файлы"),
        message,
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        for (const QString &file : files) {
            QFile::remove(file);
        }
        refreshGitStatus();
    }
}

void MainWindow::stashSelectedFiles(const QStringList &files)
{
    if (files.isEmpty()) return;

    bool ok;
    QString message = QInputDialog::getText(this, tr("Create Stash"),
                                             tr("Stash message:"),
                                             QLineEdit::Normal,
                                             tr("WIP on selection"),
                                             &ok);
    if (ok && !message.isEmpty()) {
        git->createStash(files, message);
    }
}

void MainWindow::addSelectedFile()
{
    // Get the currently selected row
    QModelIndexList selectedIndexes = ui->filesTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;
    
    int row = selectedIndexes.first().row();
    
    // Get the file name from the model
    QString fileName = unstagedFilesModel->getFileName(row);
    if (fileName.isEmpty()) return;

    // Временно отключаем watcher чтобы избежать двойного обновления
    m_fsWatcher->blockSignals(true);
    git->addFile(fileName);
}

void MainWindow::showStagedFileContextMenu(const QPoint &pos)
{
    QModelIndexList selectedIndexes = ui->stagedFilesTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;

    FileModel *model = qobject_cast<FileModel*>(ui->stagedFilesTable->model());
    if (!model) return;

    QStringList selectedFiles;
    for (const QModelIndex &index : selectedIndexes) {
        selectedFiles << model->getFileName(index.row());
    }

    QMenu contextMenu(this);

    // Unstage Action
    QString actionText = selectedFiles.size() > 1 ? tr("Убрать выделенные (%1)").arg(selectedFiles.size()) : tr("Убрать из stage");
    QAction *unstageAction = new QAction(actionText, this);
    connect(unstageAction, &QAction::triggered, this, [this, selectedFiles]() { unstageSelectedFiles(selectedFiles); });
    contextMenu.addAction(unstageAction);

    contextMenu.addSeparator();

    if (selectedFiles.size() == 1) {
        QString fileName = selectedFiles.first();
        QAction *copyPathAction = new QAction(tr("Копировать путь"), this);
        connect(copyPathAction, &QAction::triggered, this, [this, fileName]() { copyFilePath(fileName); });
        contextMenu.addAction(copyPathAction);

        QAction *openFileAction = new QAction(tr("Открыть файл"), this);
        connect(openFileAction, &QAction::triggered, this, [this, fileName]() { openFile(fileName); });
        contextMenu.addAction(openFileAction);

        QAction *openFolderAction = new QAction(tr("Открыть папку"), this);
        connect(openFolderAction, &QAction::triggered, this, [this, fileName]() { openFolder(fileName); });
        contextMenu.addAction(openFolderAction);

        QAction *deleteAction = new QAction(tr("Удалить"), this);
        connect(deleteAction, &QAction::triggered, this, [this, fileName]() { deleteFile(fileName); });
        contextMenu.addAction(deleteAction);

        QAction *blameAction = new QAction(tr("Blame"), this);
        blameAction->setEnabled(false);
        contextMenu.addAction(blameAction);

        contextMenu.addSeparator();

        QAction *discardAction = new QAction(tr("Отменить изменения"), this);
        connect(discardAction, &QAction::triggered, this, [this, fileName]() { discardFileChanges(fileName); });
        contextMenu.addAction(discardAction);
    } else {
        // Multi-file actions
        QAction *deleteAction = new QAction(tr("Удалить выделенные"), this);
        connect(deleteAction, &QAction::triggered, this, [this, selectedFiles]() { deleteSelectedFiles(selectedFiles); });
        contextMenu.addAction(deleteAction);
    }

    contextMenu.exec(ui->stagedFilesTable->viewport()->mapToGlobal(pos));
}

void MainWindow::unstageSelectedFiles(const QStringList &files)
{
    if (files.isEmpty()) return;

    m_fsWatcher->blockSignals(true);
    // Используем метод unstageFiles, который принимает список и выполняет одну команду git reset
    git->unstageFiles(files);
}

void MainWindow::unstageSelectedFile()
{
    // Get the currently selected row
    QModelIndexList selectedIndexes = ui->stagedFilesTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;
    
    int row = selectedIndexes.first().row();
    
    // Get the file name from the model
    QString fileName = stagedFilesModel->getFileName(row);
    if (fileName.isEmpty()) return;

    // Временно отключаем watcher чтобы избежать двойного обновления
    m_fsWatcher->blockSignals(true);
    git->unstageFile(fileName);
}

void MainWindow::commitChanges()
{
    // Get the commit message
    QString commitMessage = ui->commitMessageTextEdit->toPlainText().trimmed();
    
    // Check if commit message is empty
    if (commitMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Введите сообщение коммита."));
        return;
    }
    
    // Check if amend is checked
    bool amend = ui->amend_chk->isChecked();

    // Временно отключаем watcher чтобы избежать двойного обновления
    m_fsWatcher->blockSignals(true);
    git->commit(commitMessage, amend);
}

void MainWindow::updateCommitButtonState()
{
    // Enable commit action only if there are staged files
    bool hasStagedFiles = stagedFilesModel->rowCount() > 0;

    if (hasStagedFiles) {
        auto msg = ui->commitMessageTextEdit->toPlainText().trimmed();
        ui->actionCommit->setEnabled(!msg.isEmpty());
    } else {
        ui->actionCommit->setEnabled(false);
    }

    // Hide/show staged label and table based on whether there are staged files
    ui->stagedLabel->setVisible(hasStagedFiles);
    ui->stagedFilesTable->setVisible(hasStagedFiles);
}

void MainWindow::onFileTableSelectionChanged()
{
    QModelIndexList selectedIndexes = ui->filesTable->selectionModel()->selectedRows();
    if (!selectedIndexes.isEmpty()) {
        int row = selectedIndexes.first().row();
        QString fileName = unstagedFilesModel->getFileName(row);
        if (!fileName.isEmpty()) {
            m_lastSelectedFileName = fileName;
            m_lastSelectionSource = SelectionSource::Unstaged;
            updateDiffPanel(fileName);
        }
    }
    
    // Обновляем состояние кнопок при смене файла
    updateNavigationButtonsState();
}

void MainWindow::onStagedFileTableSelectionChanged()
{
    QModelIndexList selectedIndexes = ui->stagedFilesTable->selectionModel()->selectedRows();
    if (!selectedIndexes.isEmpty()) {
        int row = selectedIndexes.first().row();
        QString fileName = stagedFilesModel->getFileName(row);
        if (!fileName.isEmpty()) {
            m_lastSelectedFileName = fileName;
            m_lastSelectionSource = SelectionSource::Staged;
            updateDiffPanel(fileName);
        }
    }
}

void MainWindow::updateDiffPanel(const QString &fileName)
{
    QString leftContent, rightContent;
    QString leftVersion, rightVersion;

    if (m_lastSelectionSource == SelectionSource::Staged) {
        leftContent = getFileContent(fileName, false);   // HEAD
        rightContent = getFileContent(fileName, true);   // staged
        leftVersion = "HEAD";
        rightVersion = "Staged";
        git->getDiffStaged(fileName);
    } else {
        bool isStaged = false;
        for (int i = 0; i < stagedFilesModel->rowCount(); ++i) {
            if (stagedFilesModel->getFileName(i) == fileName) {
                isStaged = true;
                break;
            }
        }

        if (isStaged) {
            leftContent = getFileContent(fileName, true);    // staged
            rightContent = getFileContent(fileName, false);  // current
            leftVersion = "Staged";
            rightVersion = "Current";
            git->getDiffWorkingTree(fileName);
        } else {
            leftContent = getFileContent(fileName, true);    // HEAD
            rightContent = getFileContent(fileName, false);  // current
            leftVersion = "HEAD";
            rightVersion = "Current";
            git->getDiff(fileName);
        }
    }

    // Обновляем информацию о файле и версиях
    QString infoText = QString("%1\t%2 vs %3").arg(fileName, leftVersion, rightVersion);
    ui->diffInfoLabel->setText(infoText);

    diffEditor->setContents(leftContent, rightContent, fileName);
}

void MainWindow::synchronizeZoom(int zoom)
{
    // Больше не используется — синхронизация зума внутри DiffEditor
    Q_UNUSED(zoom);
}

QString MainWindow::getFileContent(const QString &fileName, bool staged)
{
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }

    if (staged) {
        // Для staged версии получаем содержимое из индекса (git show :file)
        QProcess gitProcess;
        gitProcess.setProgram(gitPath);
        gitProcess.setArguments(QStringList() << "show" << (":" + fileName));
        gitProcess.setWorkingDirectory(repositoryPath);
        gitProcess.start();
        gitProcess.waitForFinished(5000);

        if (gitProcess.exitCode() == 0) {
            QString output = gitProcess.readAllStandardOutput();
            if (!output.isEmpty()) {
                return output;
            }
        }

        // Fallback: если файл не staged, берём из HEAD
        gitProcess.setArguments(QStringList() << "show" << ("HEAD:" + fileName));
        gitProcess.start();
        gitProcess.waitForFinished(5000);

        if (gitProcess.exitCode() == 0) {
            return gitProcess.readAllStandardOutput();
        }
        return "";
    } else {
        // Для current версии читаем из файловой системы
        QString absoluteFilePath = QDir(repositoryPath).absoluteFilePath(fileName);
        
        // Проверяем, существует ли файл
        if (!QFile::exists(absoluteFilePath)) {
            // Файл удалён из working tree
            return "";
        }
        
        QFile file(absoluteFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            return content;
        }
        return "";
    }
}

void MainWindow::navigateToNextHunk()
{
    // 1. Пробуем перейти к следующему ханку в текущем файле
    if (diffEditor->navigateToNextHunk())
        return;

    // 2. Если конец файла достигнут, переключаемся на следующий файл в списке unstaged
    if (m_lastSelectionSource == SelectionSource::Unstaged) {
        QModelIndex current = ui->filesTable->selectionModel()->currentIndex();
        QModelIndex next = current.sibling(current.row() + 1, 0);
        if (next.isValid()) {
            // Устанавливаем флаг отложенной навигации
            m_pendingNavigation = PendingNavigation::GoNext;
            
            // Выбираем следующий файл (это вызовет асинхронную загрузку diff)
            ui->filesTable->selectionModel()->select(next, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            ui->filesTable->scrollTo(next);
        }
    }
    
    // Обновляем состояние кнопок
    updateNavigationButtonsState();
}

void MainWindow::toggleLeftPanel(bool visible)
{
    ui->leftFrame->setVisible(visible);
}

void MainWindow::toggleTopPanel(bool visible)
{
    ui->topFrame->setVisible(visible);
}

void MainWindow::onAmendCheckBoxChanged(int state)
{
    // Check if the checkbox is checked and commit message is empty
    if (state == Qt::Checked && ui->commitMessageTextEdit->toPlainText().trimmed().isEmpty()) {
        // Get the last commit message
        QSettings gitSettings("CommitCraft", "Settings");
        QString gitPath = gitSettings.value("gitPath", "").toString();
        if (gitPath.isEmpty()) {
            gitPath = "git";
        }

        QProcess gitProcess;
        gitProcess.setProgram(gitPath);
        gitProcess.setArguments(QStringList() << "log" << "-1" << "--pretty=format:%s");
        gitProcess.setWorkingDirectory(repositoryPath);
        gitProcess.start();
        gitProcess.waitForFinished();

        if (gitProcess.exitCode() == 0) {
            QString lastCommitMessage = gitProcess.readAllStandardOutput();
            if (!lastCommitMessage.isEmpty()) {
                ui->commitMessageTextEdit->setPlainText(lastCommitMessage);
            }
        }
    }
}

void MainWindow::navigateToPrevHunk()
{
    // 1. Пробуем перейти к предыдущему ханку в текущем файле
    if (diffEditor->navigateToPrevHunk())
        return;

    // 2. Если начало файла достигнуто, переключаемся на предыдущий файл в списке unstaged
    if (m_lastSelectionSource == SelectionSource::Unstaged) {
        QModelIndex current = ui->filesTable->selectionModel()->currentIndex();
        QModelIndex prev = current.sibling(current.row() - 1, 0);
        if (prev.isValid()) {
            // Устанавливаем флаг отложенной навигации
            m_pendingNavigation = PendingNavigation::GoPrev;
            
            // Выбираем предыдущий файл (это вызовет асинхронную загрузку diff)
            ui->filesTable->selectionModel()->select(prev, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            ui->filesTable->scrollTo(prev);
        }
    }
    
    // Обновляем состояние кнопок
    updateNavigationButtonsState();
}

void MainWindow::saveFontSettings(const QString &fontFamily, int fontSize)
{
    QSettings settings("CommitCraft", "AppSettings");
    settings.setValue("fontFamily", fontFamily);
    settings.setValue("fontSize", fontSize);
}

QPair<QString, int> MainWindow::loadFontSettings()
{
    QSettings settings("CommitCraft", "AppSettings");
    QString fontFamily = settings.value("fontFamily", "Consolas").toString();
    int fontSize = settings.value("fontSize", 10).toInt();
    return qMakePair(fontFamily, fontSize);
}

void MainWindow::setupFileSystemWatcher()
{
    // Настройка debounce-таймера
    m_fsDebounceTimer->setSingleShot(true);
    m_fsDebounceTimer->setInterval(500); // 500мс задержка
    connect(m_fsDebounceTimer, &QTimer::timeout, this, [this]() {
        refreshGitStatus();
    });

    // Подключение сигналов watcher
    connect(m_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onFileSystemChanged);
    connect(m_fsWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileSystemChanged);

    // Добавляем корень репозитория в watcher
    if (!repositoryPath.isEmpty() && QDir(repositoryPath).exists()) {
        m_fsWatcher->addPath(repositoryPath);
    }
}

void MainWindow::onFileSystemChanged()
{
    // Перезапускаем debounce-таймер
    m_fsDebounceTimer->start();
}

void MainWindow::copyFilePath(const QString &fileName)
{
    QString absolutePath = QDir(repositoryPath).absoluteFilePath(fileName);
    QGuiApplication::clipboard()->setText(absolutePath);
    statusBar()->showMessage(tr("Путь скопирован: %1").arg(absolutePath), 3000);
}

void MainWindow::openFile(const QString &fileName)
{
    QString absolutePath = QDir(repositoryPath).absoluteFilePath(fileName);
    if (QFile::exists(absolutePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Файл не существует: %1").arg(fileName));
    }
}

void MainWindow::openFolder(const QString &fileName)
{
    QString absolutePath = QDir(repositoryPath).absoluteFilePath(fileName);
    QString folderPath = QFileInfo(absolutePath).absolutePath();
    if (QDir(folderPath).exists()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Директория не существует: %1").arg(folderPath));
    }
}

void MainWindow::deleteFile(const QString &fileName)
{
    QString absolutePath = QDir(repositoryPath).absoluteFilePath(fileName);
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Удалить файл"),
        tr("Вы уверены, что хотите удалить файл '%1'?").arg(fileName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // If file is staged, unstage it first
        bool isStaged = false;
        for (int i = 0; i < stagedFilesModel->rowCount(); ++i) {
            if (stagedFilesModel->getFileName(i) == fileName) {
                isStaged = true;
                break;
            }
        }
        
        if (isStaged) {
            m_fsWatcher->blockSignals(true);
            git->unstageFile(fileName);
        }
        
        // Delete file from filesystem
        if (QFile::remove(absolutePath)) {
            statusBar()->showMessage(tr("Файл удалён: %1").arg(fileName), 3000);
        } else {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось удалить файл: %1").arg(fileName));
        }
    }
}

void MainWindow::discardFileChanges(const QString &fileName)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Отменить изменения"),
        tr("Вы уверены, что хотите отменить изменения в '%1'? Все несохранённые изменения будут потеряны.").arg(fileName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Check if file is staged
        bool isStaged = false;
        for (int i = 0; i < stagedFilesModel->rowCount(); ++i) {
            if (stagedFilesModel->getFileName(i) == fileName) {
                isStaged = true;
                break;
            }
        }
        
        m_fsWatcher->blockSignals(true);
        
        if (isStaged) {
            // File is staged: restore from index (staged version)
            runGitCommand("checkout", QStringList() << "--" << fileName, repositoryPath);
        } else {
            // File is unstaged: restore from HEAD
            runGitCommand("checkout", QStringList() << "HEAD" << "--" << fileName, repositoryPath);
        }
    }
}

void MainWindow::showBlameStub()
{
    QMessageBox::information(this, tr("Blame"), tr("Функция ещё не реализована."));
}

void MainWindow::runGitCommand(const QString &command, const QStringList &args, const QString &workingDir)
{
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    QStringList allArgs = args;
    allArgs.prepend(command);
    
    QProcess gitProcess;
    gitProcess.setProgram(gitPath);
    gitProcess.setArguments(allArgs);
    gitProcess.setWorkingDirectory(workingDir);
    gitProcess.start();
    gitProcess.waitForFinished(5000);
    
    if (gitProcess.exitCode() == 0) {
        refreshGitStatus();
    } else {
        QString errorMsg = gitProcess.readAllStandardError();
        QMessageBox::warning(this, tr("Ошибка Git"), tr("Не удалось выполнить git %1: %2").arg(command, errorMsg));
    }
    
    m_fsWatcher->blockSignals(false);
}

void MainWindow::updateNavigationButtonsState()
{
    bool hasNext = false;
    bool hasPrev = false;

    // Получаем информацию о текущем файле
    QModelIndex current = ui->filesTable->selectionModel()->currentIndex();
    int totalFiles = ui->filesTable->model()->rowCount();

    if (totalFiles > 0 && current.isValid()) {
        bool isFirstFile = (current.row() == 0);
        bool isLastFile = (current.row() == totalFiles - 1);

        // Проверяем состояние ханков
        bool isHunkEmpty = (diffEditor->hunkCount() == 0);
        bool atHunkStart = diffEditor->isAtFirstHunk();
        bool atHunkEnd = diffEditor->isAtLastHunk();

        // Логика для кнопки Next:
        // Можно идти дальше, если мы не в конце списка ханков текущего файла
        // ИЛИ если мы в конце ханков, но есть еще файлы в списке
        hasNext = (!atHunkEnd && !isHunkEmpty) || !isLastFile;

        // Логика для кнопки Prev:
        // Можно идти назад, если мы не в начале списка ханков
        // ИЛИ если мы в начале ханков, но есть предыдущие файлы
        hasPrev = (!atHunkStart && !isHunkEmpty) || !isFirstFile;
    }

    ui->actionPrevHunk->setEnabled(hasPrev);
    ui->actionNextHunk->setEnabled(hasNext);
}
