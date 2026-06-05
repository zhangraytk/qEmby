#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>

class QFocusEvent;
class QKeyEvent;

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit ShortcutEdit(QWidget* parent = nullptr);

Q_SIGNALS:
    void shortcutTextChanged(const QString& value);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

#endif
