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
    
    // Initialize table
    ui->filesTable->setColumnCount(2);
    ui->filesTable->setHorizontalHeaderLabels(QStringList() << "Статус" << "Файл");
    ui->filesTable->horizontalHeader()->setStretchLastSection(true);
    
    // Enable context menu
    ui->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->filesTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showFileContextMenu);
    
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
    
    ui->filesTable->setRowCount(lines.size());
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.length() < 4) continue;
        
        QString status = line.left(2);
        QString file = line.mid(3);
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        QTableWidgetItem *fileItem = new QTableWidgetItem(file);
        
        ui->filesTable->setItem(i, 0, statusItem);
        ui->filesTable->setItem(i, 1, fileItem);
    }
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
