#include "mainwindow.h"
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QPair>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
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

    // DiffEditor создан через promoted widget в mainwindow.ui
    // Получаем ссылатель на него
    diffEditor = ui->diffEditor;

    // Set up the table views with models
    ui->filesTable->setModel(unstagedFilesModel);
    ui->stagedFilesTable->setModel(stagedFilesModel);
    ui->commitHistoryList->setModel(commitHistoryModel);
    ui->commitHistoryList->setItemDelegate(commitItemDelegate);

    // Connect git process signals to DiffEditor
    connect(git, &Git::diffReady, this, [this](const QString &diffOutput) {
        GitParser parser;
        QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
        diffEditor->applyDiffData(hunks);
    });

    // Connect hunk navigation buttons to DiffEditor
    connect(ui->actionPrevHunk, &QAction::triggered, this, &MainWindow::navigateToPrevHunk);
    connect(ui->actionNextHunk, &QAction::triggered, this, &MainWindow::navigateToNextHunk);
    
    // Set default actions for hunk navigation buttons
    ui->prevHunkButton->setDefaultAction(ui->actionPrevHunk);
    ui->nextHunkButton->setDefaultAction(ui->actionNextHunk);
    
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

    connect(ui->commitMessageTextEdit,
            &QTextEdit::textChanged,
            this,
            &MainWindow::updateCommitButtonState);

    // Connect git process signals
    connect(git, &Git::statusReady, this, &MainWindow::onGitStatusFinished);
    connect(git, &Git::diffReady, this, &MainWindow::onGitDiffReady);
    connect(git, &Git::commitHistoryReady, this, &MainWindow::onGitCommitHistoryReady);
    connect(git, &Git::commitReady, this, &MainWindow::onGitCommitFinished);
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
    
    settingsDialog->exec();
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
        QMessageBox::information(this, tr("Успех"), tr("Коммит успешно выполнен."));
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
            refreshGitStatus();
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
    // Get the index at the position
    QModelIndex index = ui->filesTable->indexAt(pos);
    if (!index.isValid()) return;
    
    // Create the context menu
    QMenu contextMenu(tr("Файл"), this);
    
    // Create the "Добавить" action
    QAction *addAction = new QAction(tr("Добавить"), this);
    connect(addAction, &QAction::triggered, this, &MainWindow::addSelectedFile);
    contextMenu.addAction(addAction);
    
    // Show the context menu
    contextMenu.exec(ui->filesTable->viewport()->mapToGlobal(pos));
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
    
    git->addFile(fileName);
}

void MainWindow::showStagedFileContextMenu(const QPoint &pos)
{
    // Get the index at the position
    QModelIndex index = ui->stagedFilesTable->indexAt(pos);
    if (!index.isValid()) return;
    
    // Create the context menu
    QMenu contextMenu(tr("Файл"), this);
    
    // Create the "Убрать из stage" action
    QAction *unstageAction = new QAction(tr("Убрать из stage"), this);
    connect(unstageAction, &QAction::triggered, this, &MainWindow::unstageSelectedFile);
    contextMenu.addAction(unstageAction);
    
    // Show the context menu
    contextMenu.exec(ui->stagedFilesTable->viewport()->mapToGlobal(pos));
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
            updateDiffPanel(fileName);
        }
    }
}

void MainWindow::onStagedFileTableSelectionChanged()
{
    QModelIndexList selectedIndexes = ui->stagedFilesTable->selectionModel()->selectedRows();
    if (!selectedIndexes.isEmpty()) {
        int row = selectedIndexes.first().row();
        QString fileName = stagedFilesModel->getFileName(row);
        if (!fileName.isEmpty()) {
            updateDiffPanel(fileName);
        }
    }
}

void MainWindow::updateDiffPanel(const QString &fileName)
{
    // Get file contents
    QString stagedContent = getFileContent(fileName, true);
    QString currentContent = getFileContent(fileName, false);

    // Set content in DiffEditor (diff data will come asynchronously via applyDiffData)
    diffEditor->setContents(stagedContent, currentContent, fileName);
}

void MainWindow::synchronizeZoom(int zoom)
{
    // Больше не используется — синхронизация зума внутри DiffEditor
    Q_UNUSED(zoom);
}

QString MainWindow::getFileContent(const QString &fileName, bool staged)
{
    if (staged) {
        // For staged content, we need to get it from git
        QSettings gitSettings("CommitCraft", "Settings");
        QString gitPath = gitSettings.value("gitPath", "").toString();
        if (gitPath.isEmpty()) {
            gitPath = "git";
        }

        QProcess gitProcess;
        gitProcess.setProgram(gitPath);
        gitProcess.setArguments(QStringList() << "show" << QString("HEAD:%1").arg(fileName));
        gitProcess.setWorkingDirectory(repositoryPath);
        gitProcess.start();
        gitProcess.waitForFinished();

        if (gitProcess.exitCode() == 0) {
            return gitProcess.readAllStandardOutput();
        } else {
            return "";
        }
    } else {
        // For current content, read from file system
        QString absoluteFilePath = QDir(repositoryPath).absoluteFilePath(fileName);
        QFile file(absoluteFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            return content;
        } else {
            return "";
        }
    }
}

void MainWindow::navigateToNextHunk()
{
    diffEditor->navigateToNextHunk();
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
    diffEditor->navigateToPrevHunk();
}
