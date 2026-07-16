#include "shortcutedit.h"

#include "../utils/shortcututils.h"
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>

ShortcutEdit::ShortcutEdit(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ShortcutEdit"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setObjectName(QStringLiteral("ShortcutEditLine"));
    m_lineEdit->setClearButtonEnabled(true);
    m_lineEdit->setPlaceholderText(
        tr("Type shortcuts separated by semicolons"));
    m_lineEdit->installEventFilter(this);

    m_captureButton = new QPushButton(tr("Capture"), this);
    m_captureButton->setObjectName(QStringLiteral("SettingsCardButton"));
    m_captureButton->setToolTip(tr("Capture the next key combination"));

    m_restoreButton = new QPushButton(tr("Restore Default"), this);
    m_restoreButton->setObjectName(QStringLiteral("SettingsCardButton"));
    m_restoreButton->setToolTip(tr("Restore the default shortcut"));

    layout->addWidget(m_lineEdit, 1);
    layout->addWidget(m_captureButton);
    layout->addWidget(m_restoreButton);
    setFocusProxy(m_lineEdit);

    connect(m_captureButton, &QPushButton::clicked, this, [this]() {
        if (m_capturing) {
            endCapture();
        } else {
            beginCapture();
        }
    });
    connect(m_restoreButton, &QPushButton::clicked,
            this, &ShortcutEdit::restoreDefault);
    connect(m_lineEdit, &QLineEdit::editingFinished,
            this, &ShortcutEdit::commitManualText);
    connect(m_lineEdit, &QLineEdit::textEdited, this, [this]() {
        if (!m_capturing) {
            setValidationState(validate(m_lineEdit->text()));
        }
    });
}

QString ShortcutEdit::value() const
{
    return m_lineEdit ? m_lineEdit->text().trimmed() : QString();
}

QString ShortcutEdit::defaultValue() const
{
    return m_defaultValue;
}

bool ShortcutEdit::isCapturing() const
{
    return m_capturing;
}

QLineEdit* ShortcutEdit::lineEdit() const
{
    return m_lineEdit;
}

void ShortcutEdit::setValue(const QString& value)
{
    if (!m_lineEdit) {
        return;
    }
    m_lineEdit->setText(value.trimmed());
    setValidationState(validate(m_lineEdit->text()));
    m_committedValue = m_lineEdit->text();
}

void ShortcutEdit::setDefaultValue(const QString& value)
{
    m_defaultValue = value.trimmed();
}

bool ShortcutEdit::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_lineEdit && m_capturing &&
        event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->isAutoRepeat()) {
            return true;
        }

        const QKeySequence sequence = ShortcutUtils::fromKeyEvent(keyEvent);
        if (sequence.isEmpty()) {
            return true;
        }

        const QString portable =
            sequence.toString(QKeySequence::PortableText).trimmed();
        if (portable.isEmpty()) {
            return true;
        }

        m_lineEdit->setText(portable);
        setValidationState(true);
        endCapture();
        commitValue(portable);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShortcutEdit::beginCapture()
{
    m_capturing = true;
    m_captureButton->setText(tr("Cancel"));
    m_captureButton->setProperty("capturing", true);
    m_captureButton->style()->unpolish(m_captureButton);
    m_captureButton->style()->polish(m_captureButton);
    m_lineEdit->setPlaceholderText(tr("Press a shortcut now"));
    m_lineEdit->setFocus(Qt::OtherFocusReason);
    m_lineEdit->selectAll();
}

void ShortcutEdit::endCapture()
{
    m_capturing = false;
    m_captureButton->setText(tr("Capture"));
    m_captureButton->setProperty("capturing", false);
    m_captureButton->style()->unpolish(m_captureButton);
    m_captureButton->style()->polish(m_captureButton);
    m_lineEdit->setPlaceholderText(
        tr("Type shortcuts separated by semicolons"));
}

void ShortcutEdit::commitManualText()
{
    if (m_capturing) {
        return;
    }
    const QString text = value();
    const bool valid = validate(text);
    setValidationState(valid);
    if (valid) {
        m_lineEdit->setText(text);
        commitValue(text);
    }
}

void ShortcutEdit::restoreDefault()
{
    endCapture();
    m_lineEdit->setText(m_defaultValue);
    setValidationState(true);
    commitValue(m_defaultValue);
}

void ShortcutEdit::commitValue(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed == m_committedValue) {
        return;
    }
    m_committedValue = trimmed;
    Q_EMIT shortcutTextChanged(trimmed);
}

bool ShortcutEdit::validate(const QString& value) const
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    const QStringList parts = ShortcutUtils::splitShortcutList(trimmed);
    if (parts.isEmpty()) {
        return false;
    }
    for (const QString& part : parts) {
        const QKeySequence sequence = ShortcutUtils::fromUserString(part);
        if (sequence.isEmpty() || sequence.count() != 1 ||
            sequence[0].key() == Qt::Key_unknown ||
            sequence[0].key() == Qt::Key(0)) {
            return false;
        }
    }
    return true;
}

void ShortcutEdit::setValidationState(bool valid)
{
    if (!m_lineEdit) {
        return;
    }
    m_valid = valid;
    m_lineEdit->setProperty("inputValid", valid);
    m_lineEdit->setToolTip(valid ? QString() :
        tr("Invalid shortcut. Separate alternatives with semicolons."));
    m_lineEdit->style()->unpolish(m_lineEdit);
    m_lineEdit->style()->polish(m_lineEdit);
    Q_EMIT validationChanged(valid);
}
