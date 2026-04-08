#include "fontsettingsdialog.h"

#include <QFontComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFontDatabase>

FontSettingsDialog::FontSettingsDialog(const QString &currentFontFamily,
                                       int currentFontSize,
                                       QWidget *parent)
    : QDialog(parent)
    , m_fontComboBox(new QFontComboBox(this))
    , m_fontSizeSpinBox(new QSpinBox(this))
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
{
    setWindowTitle(tr("Настройки шрифта"));
    setMinimumWidth(350);

    // Фильтр только monospace-шрифты
    m_fontComboBox->setFontFilters(QFontComboBox::MonospacedFonts);
    m_fontComboBox->setCurrentFont(QFont(currentFontFamily));

    // Размер шрифта
    m_fontSizeSpinBox->setRange(8, 24);
    m_fontSizeSpinBox->setValue(currentFontSize);

    // Layout
    auto *formLayout = new QFormLayout();
    formLayout->addRow(tr("Шрифт:"), m_fontComboBox);
    formLayout->addRow(tr("Размер:"), m_fontSizeSpinBox);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(m_buttonBox);

    // Connect
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString FontSettingsDialog::getFontFamily() const
{
    return m_fontComboBox->currentFont().family();
}

int FontSettingsDialog::getFontSize() const
{
    return m_fontSizeSpinBox->value();
}
