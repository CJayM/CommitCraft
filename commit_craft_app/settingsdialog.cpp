#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    settings(new QSettings("CommitCraft", "Settings"))
{
    ui->setupUi(this);
    
    // Set window title
    setWindowTitle("Настройки");
    
    // Connect buttons
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->gitPathButton, &QPushButton::clicked, this, &SettingsDialog::onGitPathButtonClicked);
    
    // Load settings
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
    delete settings;
}

QString SettingsDialog::getGitPath() const
{
    return ui->gitPathLineEdit->text();
}

void SettingsDialog::setGitPath(const QString &gitPath)
{
    ui->gitPathLineEdit->setText(gitPath);
}

void SettingsDialog::onGitPathButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Выберите Git executable"), 
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), 
        tr("Executable Files (*.exe);;All Files (*)"));
    
    if (!fileName.isEmpty()) {
        ui->gitPathLineEdit->setText(fileName);
    }
}

void SettingsDialog::loadSettings()
{
    QString gitPath = settings->value("gitPath", "").toString();
    setGitPath(gitPath);
}

void SettingsDialog::saveSettings()
{
    settings->setValue("gitPath", getGitPath());
}