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
    , stagedContentEditor(nullptr)
    , currentContentEditor(nullptr)
    , unstagedFilesModel(new FileModel(this))
    , stagedFilesModel(new FileModel(this))
    , commitHistoryModel(new CommitHistoryModel(this))
    , commitItemDelegate(new CommitItemDelegate(this))
    , git(new Git(this))
    , currentHunkIndex(-1)
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
    
    // Set up the table views with models
    ui->filesTable->setModel(unstagedFilesModel);
    ui->stagedFilesTable->setModel(stagedFilesModel);
    ui->commitHistoryList->setModel(commitHistoryModel);
    ui->commitHistoryList->setItemDelegate(commitItemDelegate);
    
    // Connect zoom synchronization
    connect(stagedContentEditor, &CodeEditor::zoomChanged, this, &MainWindow::synchronizeZoom);
    connect(currentContentEditor, &CodeEditor::zoomChanged, this, &MainWindow::synchronizeZoom);
    
    // Connect scroll synchronization
    connect(stagedContentEditor->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &MainWindow::synchronizeScroll);
    connect(currentContentEditor->verticalScrollBar(), &QScrollBar::valueChanged, 
            this, &MainWindow::synchronizeScroll);
    
    // Connect hunk navigation buttons
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
    parseGitStatusOutput(output);
    git->getCommitHistory();
}

void MainWindow::onGitDiffReady(const QString &output)
{
    // Extract hunk positions for navigation
    extractHunkPositions(output);
    
    // Demonstrate advanced usage of GitParser
    // Uncomment the following line to see hunk details in the debug output
    // displayHunkDetails(output);
    
    // Parse and apply diff highlighting to the text edits
    parseAndApplyDiffHighlighting(output);
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

void MainWindow::parseGitStatusOutput(const QString &output)
{
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
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

    // Update models with new data
    unstagedFilesModel->setFiles(unstagedFiles);
    stagedFilesModel->setFiles(stagedFiles);
    
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
    // Reset hunk navigation
    hunkPositions.clear();
    currentHunkIndex = -1;
    ui->actionPrevHunk->setEnabled(false);
    ui->actionNextHunk->setEnabled(false);
    
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
    git->getDiff(fileName);
    
    // For demonstration purposes, you could also parse and display hunk details here
    // This would typically be done in the onGitDiffReady slot after receiving the diff output
}

void MainWindow::extractHunkPositions(const QString &diffOutput)
{
    // Clear existing hunk positions
    hunkPositions.clear();
    currentHunkIndex = -1;
    
    // Use GitParser to parse the diff output
    GitParser parser;
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
    
    // Process each hunk to extract positions
    for (const Hunk &hunk : hunks) {
        // For simplicity, we're just storing the start positions
        // In a real application, you might want to store more information
        hunkPositions.append(qMakePair(hunk.leftStart, hunk.rightStart));
    }
    
    // Update button states
    bool hasHunks = !hunkPositions.isEmpty();
    ui->actionPrevHunk->setEnabled(hasHunks);
    ui->actionNextHunk->setEnabled(hasHunks);
}

void MainWindow::displayHunkDetails(const QString &diffOutput)
{
    GitParser parser;
    QList<Hunk> hunks = parser.parseDiffOutput(diffOutput);
    
    qDebug() << "Found" << hunks.size() << "hunks:";
    
    for (int i = 0; i < hunks.size(); ++i) {
        const Hunk &hunk = hunks[i];
        qDebug() << "Hunk" << i << ": @@ -" << hunk.leftStart << "," << hunk.leftSize 
                 << " +" << hunk.rightStart << "," << hunk.rightSize << " @@";
        
        qDebug() << "  Lines:";
        for (int j = 0; j < hunk.lines.size(); ++j) {
            const HunkLine &line = hunk.lines[j];
            QString typeStr;
            switch (line.type) {
            case HunkLine::Unchanged:
                typeStr = " ";
                break;
            case HunkLine::Removed:
                typeStr = "-";
                break;
            case HunkLine::Added:
                typeStr = "+";
                break;
            }
            qDebug() << "    " << typeStr << line.content;
        }
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
    if (staged) {
        // For staged content, we need to get it from git
        // We'll use a blocking call here for simplicity, but in a real application
        // you might want to make this asynchronous
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

void MainWindow::navigateToNextHunk()
{
    if (hunkPositions.isEmpty()) return;
    
    // Move to next hunk
    currentHunkIndex++;
    if (currentHunkIndex >= hunkPositions.size()) {
        currentHunkIndex = 0; // Wrap around to first hunk
    }
    
    // Get the hunk position
    const auto &hunk = hunkPositions[currentHunkIndex];
    int stagedLine = hunk.first;
    int currentLine = hunk.second;
    
    // Scroll to the hunk in both editors using scroll bars for better control
    // For staged content editor
    QTextCursor stagedCursor(stagedContentEditor->document());
    stagedCursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < stagedLine && !stagedCursor.atEnd(); i++) {
        stagedCursor.movePosition(QTextCursor::Down);
    }

    stagedContentEditor->setTextCursor(stagedCursor);
    // Calculate the position and scroll to it (center the hunk)
    QRect cursorRect = stagedContentEditor->cursorRect(stagedCursor);
    int scrollValue = cursorRect.y() - (stagedContentEditor->viewport()->height() / 2);
    scrollValue = qBound(0, scrollValue, stagedContentEditor->verticalScrollBar()->maximum());
    // stagedContentEditor->verticalScrollBar()->setValue(scrollValue);

    // For current content editor
    QTextCursor currentCursor(currentContentEditor->document());
    currentCursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < currentLine && !currentCursor.atEnd(); i++) {
        currentCursor.movePosition(QTextCursor::Down);
    }
    
    // Calculate the position and scroll to it (center the hunk)
    cursorRect = currentContentEditor->cursorRect(currentCursor);
    currentContentEditor->setTextCursor(currentCursor);
    scrollValue = cursorRect.y() - (currentContentEditor->viewport()->height() / 2);
    scrollValue = qBound(0, scrollValue, currentContentEditor->verticalScrollBar()->maximum());
    // currentContentEditor->verticalScrollBar()->setValue(scrollValue);
    qDebug() << "Scroll to " << scrollValue << stagedLine << currentLine;
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
    if (hunkPositions.isEmpty())
        return;

    // Move to previous hunk
    currentHunkIndex--;
    if (currentHunkIndex < 0) {
        currentHunkIndex = hunkPositions.size() - 1; // Wrap around to last hunk
    }

    // Get the hunk position
    const auto &hunk = hunkPositions[currentHunkIndex];
    int stagedLine = hunk.first;
    int currentLine = hunk.second;

    // Scroll to the hunk in both editors using scroll bars for better control
    // For staged content editor
    QTextCursor stagedCursor(stagedContentEditor->document());
    stagedCursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < stagedLine && !stagedCursor.atEnd(); i++) {
        stagedCursor.movePosition(QTextCursor::Down);
    }

    // Calculate the position and scroll to it (center the hunk)
    QRect cursorRect = stagedContentEditor->cursorRect(stagedCursor);
    int scrollValue = cursorRect.y() - (stagedContentEditor->viewport()->height() / 2);
    scrollValue = qBound(0, scrollValue, stagedContentEditor->verticalScrollBar()->maximum());
    stagedContentEditor->verticalScrollBar()->setValue(scrollValue);

    // For current content editor
    QTextCursor currentCursor(currentContentEditor->document());
    currentCursor.movePosition(QTextCursor::Start);
    for (int i = 1; i < currentLine && !currentCursor.atEnd(); i++) {
        currentCursor.movePosition(QTextCursor::Down);
    }

    // Calculate the position and scroll to it (center the hunk)
    cursorRect = currentContentEditor->cursorRect(currentCursor);
    scrollValue = cursorRect.y() - (currentContentEditor->viewport()->height() / 2);
    scrollValue = qBound(0, scrollValue, currentContentEditor->verticalScrollBar()->maximum());
    currentContentEditor->verticalScrollBar()->setValue(scrollValue);
}

void MainWindow::synchronizeScroll()
{
    return;
    // Prevent infinite recursion by temporarily blocking signals
    QScrollBar *stagedScrollBar = stagedContentEditor->verticalScrollBar();
    QScrollBar *currentScrollBar = currentContentEditor->verticalScrollBar();

    bool stagedBlocked = stagedScrollBar->blockSignals(true);
    bool currentBlocked = currentScrollBar->blockSignals(true);

    // Get the scroll bar ranges
    int stagedRange = stagedScrollBar->maximum() - stagedScrollBar->minimum();
    int currentRange = currentScrollBar->maximum() - currentScrollBar->minimum();

    // Only synchronize if both editors have content
    if (stagedRange > 0 && currentRange > 0) {
        // Calculate the scroll position as a percentage
        if (sender() == stagedScrollBar) {
            // Calculate percentage of staged scroll position
            double percentage = static_cast<double>(stagedScrollBar->value()
                                                    - stagedScrollBar->minimum())
                                / stagedRange;
            // Apply the same percentage to current scroll position
            int newValue = currentScrollBar->minimum()
                           + static_cast<int>(percentage * currentRange);
            currentScrollBar->blockSignals(currentBlocked);
            currentScrollBar->setValue(newValue);
        } else if (sender() == currentScrollBar) {
            // Calculate percentage of current scroll position
            double percentage = static_cast<double>(currentScrollBar->value()
                                                    - currentScrollBar->minimum())
                                / currentRange;
            // Apply the same percentage to staged scroll position
            int newValue = stagedScrollBar->minimum() + static_cast<int>(percentage * stagedRange);
            stagedScrollBar->blockSignals(stagedBlocked);
            stagedScrollBar->setValue(newValue);
        }
    }

    // Restore signals
    stagedScrollBar->blockSignals(stagedBlocked);
    currentScrollBar->blockSignals(currentBlocked);

    // stagedContentEditor->redraw();
    // currentContentEditor->redraw();
}
