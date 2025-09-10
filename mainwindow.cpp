#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "settingsdialog.h"
#include <QCloseEvent>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("CommitCraft", "MainWindow"))
    , settingsDialog(nullptr)
    , gitProcess(new QProcess(this))
    , repositoryPath(QDir::currentPath())
{
    ui->setupUi(this);
    restoreSplitterState();
    
    // Connect the menu actions to their slots
    connect(ui->actionOpenRepository, &QAction::triggered, this, &MainWindow::openRepository);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    
    // Connect refresh button
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshGitStatus);
    
    // Connect git process signals
    connect(gitProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onGitStatusFinished);
    connect(gitProcess, &QProcess::errorOccurred, this, &MainWindow::onGitStatusError);
    
    // Initialize tables
    ui->filesTable->setColumnCount(2);
    ui->filesTable->setHorizontalHeaderLabels(QStringList() << "Статус" << "Файл");
    ui->filesTable->horizontalHeader()->setStretchLastSection(true);
    ui->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->filesTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showFileContextMenu);
    
    ui->stagedFilesTable->setColumnCount(2);
    ui->stagedFilesTable->setHorizontalHeaderLabels(QStringList() << "Статус" << "Файл");
    ui->stagedFilesTable->horizontalHeader()->setStretchLastSection(true);
    ui->stagedFilesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->stagedFilesTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showStagedFileContextMenu);
    
    // Set window title to show current repository
    setWindowTitle(QString("Commit Craft - %1").arg(repositoryPath));
    
    // Load initial git status
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
    settings->setValue("rightSplitterSizes", ui->rightSplitter->saveState());
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
    if (settings->contains("rightSplitterSizes")) {
        ui->rightSplitter->restoreState(settings->value("rightSplitterSizes").toByteArray());
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
    executeGitStatus();
}

void MainWindow::executeGitStatus()
{
    // Clear the table
    ui->filesTable->setRowCount(0);
    
    // Get git path from settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    
    // If git path is empty, try to find git in PATH
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    // Set up the process
    gitProcess->setProgram(gitPath);
    gitProcess->setArguments(QStringList() << "status" << "--porcelain");
    
    // Set working directory to selected repository path
    gitProcess->setWorkingDirectory(repositoryPath);
    
    // Start the process
    gitProcess->start();
}

void MainWindow::onGitStatusFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QString output = gitProcess->readAllStandardOutput();
        parseGitStatusOutput(output);
    } else {
        QString error = gitProcess->readAllStandardError();
        QMessageBox::warning(this, "Ошибка Git", 
                            QString("Не удалось выполнить git status:\n%1").arg(error));
    }
}

void MainWindow::onGitStatusError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = "Не удалось запустить Git. Проверьте путь к Git в настройках.";
        break;
    case QProcess::Crashed:
        errorMessage = "Git процесс аварийно завершился.";
        break;
    case QProcess::Timedout:
        errorMessage = "Таймаут выполнения Git команды.";
        break;
    case QProcess::WriteError:
        errorMessage = "Ошибка записи в Git процесс.";
        break;
    case QProcess::ReadError:
        errorMessage = "Ошибка чтения из Git процесса.";
        break;
    default:
        errorMessage = "Неизвестная ошибка Git процесса.";
        break;
    }
    
    QMessageBox::warning(this, "Ошибка Git", errorMessage);
}

void MainWindow::parseGitStatusOutput(const QString &output)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Clear both tables
    ui->filesTable->setRowCount(0);
    ui->stagedFilesTable->setRowCount(0);
    
    // Separate staged and unstaged files
    QList<QPair<QString, QString>> unstagedFiles;
    QList<QPair<QString, QString>> stagedFiles;
    
    for (const QString &line : lines) {
        if (line.length() < 4) continue;
        
        QString status = line.left(2);
        QString file = line.mid(3);
        
        // Handle partially staged files (MM) - they should appear in both tables
        if (status == "MM") {
            // Add to both tables
            unstagedFiles.append(qMakePair(QString(" M"), file)); // Show as modified in unstaged
            stagedFiles.append(qMakePair(QString("M "), file));   // Show as staged in staged
        } else {
            // Handle other statuses normally
            if (isStaged(status)) {
                stagedFiles.append(qMakePair(status, file));
            }
            if (isUnstaged(status)) {
                unstagedFiles.append(qMakePair(status, file));
            }
        }
    }
    
    // Populate unstaged files table
    ui->filesTable->setRowCount(unstagedFiles.size());
    for (int i = 0; i < unstagedFiles.size(); ++i) {
        const auto &fileInfo = unstagedFiles.at(i);
        QTableWidgetItem *statusItem = new QTableWidgetItem(fileInfo.first);
        QTableWidgetItem *fileItem = new QTableWidgetItem(fileInfo.second);
        
        ui->filesTable->setItem(i, 0, statusItem);
        ui->filesTable->setItem(i, 1, fileItem);
    }
    
    // Populate staged files table
    ui->stagedFilesTable->setRowCount(stagedFiles.size());
    for (int i = 0; i < stagedFiles.size(); ++i) {
        const auto &fileInfo = stagedFiles.at(i);
        QTableWidgetItem *statusItem = new QTableWidgetItem(fileInfo.first);
        QTableWidgetItem *fileItem = new QTableWidgetItem(fileInfo.second);
        
        ui->stagedFilesTable->setItem(i, 0, statusItem);
        ui->stagedFilesTable->setItem(i, 1, fileItem);
    }
}

bool MainWindow::isStaged(const QString &status)
{
    // Check if the file is staged (first character is not space)
    return status.at(0) != ' ' && status.at(0) != '?';
}

bool MainWindow::isUnstaged(const QString &status)
{
    // Check if the file has unstaged changes (second character is not space)
    return status.length() > 1 && status.at(1) != ' ' && status.at(1) != '?';
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
    // Get the item at the position
    QTableWidgetItem *item = ui->filesTable->itemAt(pos);
    if (!item) return;
    
    // Get the row of the item
    int row = item->row();
    
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
    int row = ui->filesTable->currentRow();
    if (row < 0) return;
    
    // Get the file name from the second column
    QTableWidgetItem *fileItem = ui->filesTable->item(row, 1);
    if (!fileItem) return;
    
    QString fileName = fileItem->text();
    
    // Make the file path absolute by prepending the repository path
    QString absoluteFilePath = QDir(repositoryPath).absoluteFilePath(fileName);
    
    // Get git path from settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    
    // If git path is empty, try to find git in PATH
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    // Create a new process for git add
    QProcess *addProcess = new QProcess(this);
    
    // Set up the process
    addProcess->setProgram(gitPath);
    addProcess->setArguments(QStringList() << "add" << absoluteFilePath);
    addProcess->setWorkingDirectory(repositoryPath);
    
    // Connect to signals to handle completion
    connect(addProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, addProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    // Success - refresh the git status
                    refreshGitStatus();
                } else {
                    // Error - show message
                    QString error = addProcess->readAllStandardError();
                    QMessageBox::warning(this, tr("Ошибка Git"), 
                                        QString(tr("Не удалось выполнить git add:\n%1")).arg(error));
                }
                // Clean up the process
                addProcess->deleteLater();
            });
    
    // Start the process
    addProcess->start();
}

void MainWindow::showStagedFileContextMenu(const QPoint &pos)
{
    // Get the item at the position
    QTableWidgetItem *item = ui->stagedFilesTable->itemAt(pos);
    if (!item) return;
    
    // Get the row of the item
    int row = item->row();
    
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
    int row = ui->stagedFilesTable->currentRow();
    if (row < 0) return;
    
    // Get the file name from the second column
    QTableWidgetItem *fileItem = ui->stagedFilesTable->item(row, 1);
    if (!fileItem) return;
    
    QString fileName = fileItem->text();
    
    // Make the file path absolute by prepending the repository path
    QString absoluteFilePath = QDir(repositoryPath).absoluteFilePath(fileName);
    
    // Get git path from settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    
    // If git path is empty, try to find git in PATH
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    // Create a new process for git reset
    QProcess *resetProcess = new QProcess(this);
    
    // Set up the process
    resetProcess->setProgram(gitPath);
    resetProcess->setArguments(QStringList() << "reset" << "HEAD" << absoluteFilePath);
    resetProcess->setWorkingDirectory(repositoryPath);
    
    // Connect to signals to handle completion
    connect(resetProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, resetProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    // Success - refresh the git status
                    refreshGitStatus();
                } else {
                    // Error - show message
                    QString error = resetProcess->readAllStandardError();
                    QMessageBox::warning(this, tr("Ошибка Git"), 
                                        QString(tr("Не удалось выполнить git reset:\n%1")).arg(error));
                }
                // Clean up the process
                resetProcess->deleteLater();
            });
    
    // Start the process
    resetProcess->start();
}
