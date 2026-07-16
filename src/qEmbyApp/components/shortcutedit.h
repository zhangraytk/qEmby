#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QWidget>

class QEvent;
class QLineEdit;
class QPushButton;

class ShortcutEdit : public QWidget
{
    Q_OBJECT

public:
    explicit ShortcutEdit(QWidget* parent = nullptr);

    QString value() const;
    QString defaultValue() const;
    bool isCapturing() const;
    QLineEdit* lineEdit() const;

    void setValue(const QString& value);
    void setDefaultValue(const QString& value);

Q_SIGNALS:
    void shortcutTextChanged(const QString& value);
    void validationChanged(bool valid);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void beginCapture();
    void endCapture();
    void commitManualText();
    void restoreDefault();
    void commitValue(const QString& value);
    bool validate(const QString& value) const;
    void setValidationState(bool valid);

    QLineEdit* m_lineEdit = nullptr;
    QPushButton* m_captureButton = nullptr;
    QPushButton* m_restoreButton = nullptr;
    QString m_defaultValue;
    QString m_committedValue;
    bool m_capturing = false;
    bool m_valid = true;
};

#endif
