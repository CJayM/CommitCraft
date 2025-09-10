#include "mainwindow.h"
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include "./ui_mainwindow.h"
#include "codeeditor.h"
#include "settingsdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settings(new QSettings("CommitCraft", "MainWindow"))
    , settingsDialog(nullptr)
    , gitProcess(new QProcess(this))
    , repositoryPath(QDir::currentPath())
    , stagedContentEditor(nullptr)
    , currentContentEditor(nullptr)
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
    
    // Replace QPlainTextEdit with CodeEditor for line numbers
    // First, remove the existing widgets
    delete ui->stagedContentTextEdit;
    delete ui->currentContentTextEdit;
    
    // Create new CodeEditor widgets
    stagedContentEditor = new CodeEditor(this);
    stagedContentEditor->setReadOnly(true);
    stagedContentEditor->setPlaceholderText(tr("Содержимое файла (staged)"));
    
    currentContentEditor = new CodeEditor(this);
    currentContentEditor->setReadOnly(true);
    currentContentEditor->setPlaceholderText(tr("Текущее содержимое файла"));
    
    // Add the CodeEditor widgets to the diffSplitter
    ui->diffSplitter->addWidget(stagedContentEditor);
    ui->diffSplitter->addWidget(currentContentEditor);
    
    // Connect zoom synchronization
    connect(stagedContentEditor, &CodeEditor::zoomChanged, this, &MainWindow::synchronizeZoom);
    connect(currentContentEditor, &CodeEditor::zoomChanged, this, &MainWindow::synchronizeZoom);
    
    // Connect the menu actions to their slots
    connect(ui->actionOpenRepository, &QAction::triggered, this, &MainWindow::openRepository);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    
    // Connect refresh button
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::refreshGitStatus);
    
    // Connect commit button
    connect(ui->commitButton, &QPushButton::clicked, this, &MainWindow::commitChanges);
    
    // Connect table selection changes
    connect(ui->filesTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onFileTableSelectionChanged);
    connect(ui->stagedFilesTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onStagedFileTableSelectionChanged);
    
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
    
    // Set initial state of commit button
    ui->commitButton->setEnabled(false);
    
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
    
    // Update commit button state
    updateCommitButtonState();
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

void MainWindow::commitChanges()
{
    // Get the commit message
    QString commitMessage = ui->commitMessageTextEdit->toPlainText().trimmed();
    
    // Check if commit message is empty
    if (commitMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Введите сообщение коммита."));
        return;
    }
    
    // Get git path from settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    
    // If git path is empty, try to find git in PATH
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    // Create a new process for git commit
    QProcess *commitProcess = new QProcess(this);
    
    // Set up the process
    commitProcess->setProgram(gitPath);
    commitProcess->setArguments(QStringList() << "commit" << "-m" << commitMessage);
    commitProcess->setWorkingDirectory(repositoryPath);
    
    // Connect to signals to handle completion
    connect(commitProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, commitProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    // Success - clear the commit message and refresh the git status
                    ui->commitMessageTextEdit->clear();
                    refreshGitStatus();
                    QMessageBox::information(this, tr("Успех"), tr("Коммит успешно выполнен."));
                } else {
                    // Error - show message
                    QString error = commitProcess->readAllStandardError();
                    QMessageBox::warning(this, tr("Ошибка Git"), 
                                        QString(tr("Не удалось выполнить git commit:\n%1")).arg(error));
                }
                // Clean up the process
                commitProcess->deleteLater();
            });
    
    // Start the process
    commitProcess->start();
}

void MainWindow::updateCommitButtonState()
{
    // Enable commit button only if there are staged files
    bool hasStagedFiles = ui->stagedFilesTable->rowCount() > 0;
    ui->commitButton->setEnabled(hasStagedFiles);
}

void MainWindow::onFileTableSelectionChanged()
{
    int row = ui->filesTable->currentRow();
    if (row >= 0) {
        QTableWidgetItem *fileItem = ui->filesTable->item(row, 1);
        if (fileItem) {
            updateDiffPanel(fileItem->text());
        }
    }
}

void MainWindow::onStagedFileTableSelectionChanged()
{
    int row = ui->stagedFilesTable->currentRow();
    if (row >= 0) {
        QTableWidgetItem *fileItem = ui->stagedFilesTable->item(row, 1);
        if (fileItem) {
            updateDiffPanel(fileItem->text());
        }
    }
}

void MainWindow::updateDiffPanel(const QString &fileName)
{
    // Get file contents
    QString stagedContent = getFileContent(fileName, true);
    QString currentContent = getFileContent(fileName, false);
    
    // Set content in text edits
    stagedContentEditor->setPlainText(stagedContent);
    currentContentEditor->setPlainText(currentContent);
    
    // Apply highlighting based on git diff
    applyDiffHighlighting(fileName);
}

void MainWindow::applyDiffHighlighting(const QString &fileName)
{
    // Get git path from settings
    QSettings gitSettings("CommitCraft", "Settings");
    QString gitPath = gitSettings.value("gitPath", "").toString();
    if (gitPath.isEmpty()) {
        gitPath = "git";
    }
    
    // Run git diff command
    QProcess gitProcess;
    gitProcess.setProgram(gitPath);
    gitProcess.setArguments(QStringList() << "diff" << fileName);
    gitProcess.setWorkingDirectory(repositoryPath);
    gitProcess.start();
    gitProcess.waitForFinished();
    
    if (gitProcess.exitCode() == 0) {
        QString diffOutput = gitProcess.readAllStandardOutput();
        // Parse and apply diff highlighting to the text edits
        parseAndApplyDiffHighlighting(diffOutput);
    } else {
        // If git diff fails, clear any existing highlighting
        stagedContentEditor->setPlainText(stagedContentEditor->toPlainText());
        currentContentEditor->setPlainText(currentContentEditor->toPlainText());
    }
}

void MainWindow::parseAndApplyDiffHighlighting(const QString &diffOutput)
{
    // Parse diff output to identify line differences
    QStringList lines = diffOutput.split('\n');
    
    // Maps to store line numbers and their types (added/deleted/modified)
    QMap<int, QString> stagedLineTypes;  // For staged content
    QMap<int, QString> currentLineTypes; // For current content
    
    int stagedLineNum = 1;
    int currentLineNum = 1;
    
    // Process diff lines
    for (const QString &line : lines) {
        if (line.startsWith("@@")) {
            // Hunk header - parse line numbers
            // Format: @@ -start,count +start,count @@
            QRegularExpression re("@@ -(\\d+),(\\d+) \\+(\\d+),(\\d+) @@");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                stagedLineNum = match.captured(1).toInt();
                currentLineNum = match.captured(3).toInt();
            }
        } else if (line.startsWith("+") && !line.startsWith("+++")) {
            // Added line (in current version)
            currentLineTypes[currentLineNum] = "added";
            currentLineNum++;
        } else if (line.startsWith("-") && !line.startsWith("---")) {
            // Deleted line (in staged version)
            stagedLineTypes[stagedLineNum] = "deleted";
            stagedLineNum++;
        } else if (!line.startsWith("diff") && !line.startsWith("index") && 
                   !line.startsWith("---") && !line.startsWith("+++")) {
            // Unchanged line
            if (!line.startsWith(" ")) {
                // This is an unchanged line
                stagedLineNum++;
                currentLineNum++;
            } else {
                // This is an unchanged line (starts with space)
                stagedLineNum++;
                currentLineNum++;
            }
        }
    }
    
    // Apply highlighting to staged content
    QTextCursor stagedCursor(stagedContentEditor->document());
    for (auto it = stagedLineTypes.begin(); it != stagedLineTypes.end(); ++it) {
        int lineNum = it.key();
        QString type = it.value();
        
        // Move cursor to the line
        stagedCursor.movePosition(QTextCursor::Start);
        for (int i = 1; i < lineNum && !stagedCursor.atEnd(); i++) {
            stagedCursor.movePosition(QTextCursor::Down);
        }
        
        // Select the entire line
        stagedCursor.movePosition(QTextCursor::StartOfLine);
        stagedCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        
        // Apply formatting
        QTextCharFormat format;
        if (type == "deleted") {
            format.setBackground(QColor(255, 198, 198)); // Light red
        }
        stagedCursor.setCharFormat(format);
    }
    
    // Apply highlighting to current content
    QTextCursor currentCursor(currentContentEditor->document());
    for (auto it = currentLineTypes.begin(); it != currentLineTypes.end(); ++it) {
        int lineNum = it.key();
        QString type = it.value();
        
        // Move cursor to the line
        currentCursor.movePosition(QTextCursor::Start);
        for (int i = 1; i < lineNum && !currentCursor.atEnd(); i++) {
            currentCursor.movePosition(QTextCursor::Down);
        }
        
        // Select the entire line
        currentCursor.movePosition(QTextCursor::StartOfLine);
        currentCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        
        // Apply formatting
        QTextCharFormat format;
        if (type == "added") {
            format.setBackground(QColor(198, 255, 198)); // Light green
        }
        currentCursor.setCharFormat(format);
    }
}

QString MainWindow::getFileContent(const QString &fileName, bool staged)
{
    QString absoluteFilePath = QDir(repositoryPath).absoluteFilePath(fileName);
    
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
            // If file doesn't exist in HEAD, return empty string
            return "";
        }
    } else {
        // For current content, read from file system
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

void MainWindow::synchronizeZoom(int zoom)
{
    // Prevent infinite recursion by temporarily blocking signals
    bool stagedBlocked = stagedContentEditor->blockSignals(true);
    bool currentBlocked = currentContentEditor->blockSignals(true);
    
    // Set the same zoom level for both editors
    if (sender() == stagedContentEditor) {
        currentContentEditor->setZoom(zoom);
    } else if (sender() == currentContentEditor) {
        stagedContentEditor->setZoom(zoom);
    }
    
    // Restore signals
    stagedContentEditor->blockSignals(stagedBlocked);
    currentContentEditor->blockSignals(currentBlocked);
}
