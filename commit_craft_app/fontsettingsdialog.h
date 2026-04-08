#ifndef FONTSETTINGSDIALOG_H
#define FONTSETTINGSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QFontComboBox;
class QSpinBox;
class QDialogButtonBox;
QT_END_NAMESPACE

/// Диалог настройки шрифта DiffEditor
class FontSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FontSettingsDialog(const QString &currentFontFamily,
                                int currentFontSize,
                                QWidget *parent = nullptr);

    QString getFontFamily() const;
    int getFontSize() const;

private:
    QFontComboBox *m_fontComboBox;
    QSpinBox *m_fontSizeSpinBox;
    QDialogButtonBox *m_buttonBox;
};

#endif // FONTSETTINGSDIALOG_H
