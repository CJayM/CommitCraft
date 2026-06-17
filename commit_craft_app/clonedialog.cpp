#include "clonedialog.h"
#include "ui_clonedialog.h"
#include <QDir>
#include <QFileDialog>
#include <QRegularExpression>
#include <QStandardPaths>

CloneDialog::CloneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CloneDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Клонировать репозиторий"));
    setModal(true);

    connect(ui->selectDestinationButton, &QPushButton::clicked,
            this, &CloneDialog::onSelectDestinationClicked);
    connect(ui->repositoryUrlEdit, &QLineEdit::textChanged,
            this, &CloneDialog::onUrlChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &CloneDialog::onAccept);

    validateInputs();
    ui->repositoryUrlEdit->setFocus();
}

CloneDialog::~CloneDialog()
{
    delete ui;
}

QString CloneDialog::getRepositoryUrl() const
{
    return ui->repositoryUrlEdit->text().trimmed();
}

QString CloneDialog::getDestinationPath() const
{
    return ui->destinationPathEdit->text().trimmed();
}

void CloneDialog::onSelectDestinationClicked()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Выберите директорию для клонирования"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    if (!dir.isEmpty()) {
        ui->destinationPathEdit->setText(dir);
        validateInputs();
    }
}

void CloneDialog::onUrlChanged(const QString &url)
{
    QString trimmed = url.trimmed();
    if (!trimmed.isEmpty()) {
        QString basename = trimmed;

        // Убираем trailing .git
        if (basename.endsWith(".git", Qt::CaseInsensitive)) {
            basename.chop(4);
        }

        // Извлекаем последний компонент пути
        int slash = basename.lastIndexOf('/');
        if (slash >= 0) {
            basename = basename.mid(slash + 1);
        }

        // Убираем special characters из имени
        basename.replace(QRegularExpression("[^a-zA-Z0-9._-]"), "");

        if (!basename.isEmpty()) {
            QString dest = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                           + QDir::separator() + basename;
            if (ui->destinationPathEdit->text().isEmpty()) {
                ui->destinationPathEdit->setText(dest);
            }
        }
    }
    validateInputs();
}

void CloneDialog::onAccept()
{
    if (!getRepositoryUrl().isEmpty() && !getDestinationPath().isEmpty()) {
        accept();
    }
}

void CloneDialog::validateInputs()
{
    bool valid = !ui->repositoryUrlEdit->text().trimmed().isEmpty()
                 && !ui->destinationPathEdit->text().trimmed().isEmpty();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}
