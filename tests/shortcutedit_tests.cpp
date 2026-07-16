#include "shortcutedit.h"

#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

class ShortcutEditTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void captureManualEntryAndRestoreDefault()
    {
        ShortcutEdit edit;
        edit.setDefaultValue(QStringLiteral("Alt+Left; Back"));
        edit.setValue(QStringLiteral("Ctrl+H"));
        edit.show();
        QApplication::processEvents();

        const auto buttons = edit.findChildren<QPushButton*>();
        QCOMPARE(buttons.size(), 2);
        QPushButton* capture = buttons.at(0);
        QPushButton* restore = buttons.at(1);

        QSignalSpy changes(&edit, &ShortcutEdit::shortcutTextChanged);
        QTest::mouseClick(capture, Qt::LeftButton);
        QVERIFY(edit.isCapturing());
        QTest::keyClick(edit.lineEdit(), Qt::Key_K, Qt::ControlModifier);
        QVERIFY(!edit.isCapturing());
        QCOMPARE(edit.value(), QStringLiteral("Ctrl+K"));
        QCOMPARE(changes.count(), 1);

        edit.lineEdit()->setFocus();
        edit.lineEdit()->selectAll();
        QTest::keyClicks(edit.lineEdit(), QStringLiteral("Alt+Right; Forward"));
        QMetaObject::invokeMethod(edit.lineEdit(), "editingFinished",
                                  Qt::DirectConnection);
        QCOMPARE(edit.value(), QStringLiteral("Alt+Right; Forward"));
        QCOMPARE(changes.count(), 2);

        QTest::mouseClick(restore, Qt::LeftButton);
        QCOMPARE(edit.value(), QStringLiteral("Alt+Left; Back"));
        QCOMPARE(changes.count(), 3);
    }

    void invalidManualEntryIsNotCommitted()
    {
        ShortcutEdit edit;
        edit.setValue(QStringLiteral("Ctrl+H"));
        QSignalSpy changes(&edit, &ShortcutEdit::shortcutTextChanged);

        edit.lineEdit()->setText(QStringLiteral("Ctrl+DefinitelyNotAKey"));
        QMetaObject::invokeMethod(edit.lineEdit(), "editingFinished",
                                  Qt::DirectConnection);

        QCOMPARE(changes.count(), 0);
        QCOMPARE(edit.lineEdit()->property("inputValid").toBool(), false);
    }
};

QTEST_MAIN(ShortcutEditTests)
#include "shortcutedit_tests.moc"
