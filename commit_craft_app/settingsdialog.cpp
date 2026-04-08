#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFontDatabase>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    settings(new QSettings("CommitCraft", "AppSettings"))
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

QString SettingsDialog::getFontFamily() const
{
    return ui->fontComboBox->currentFont().family();
}

int SettingsDialog::getFontSize() const
{
    return ui->fontSizeSpinBox->value();
}

void SettingsDialog::setFontFamily(const QString &fontFamily)
{
    ui->fontComboBox->setCurrentFont(QFont(fontFamily));
}

void SettingsDialog::setFontSize(int fontSize)
{
    ui->fontSizeSpinBox->setValue(fontSize);
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
    // Git path
    QString gitPath = settings->value("gitPath", "").toString();
    setGitPath(gitPath);

    // Font settings
    QString fontFamily = settings->value("fontFamily", "Consolas").toString();
    int fontSize = settings->value("fontSize", 10).toInt();
    setFontFamily(fontFamily);
    setFontSize(fontSize);
}

void SettingsDialog::saveSettings()
{
    // Git path
    settings->setValue("gitPath", getGitPath());

    // Font settings
    settings->setValue("fontFamily", getFontFamily());
    settings->setValue("fontSize", getFontSize());
}
