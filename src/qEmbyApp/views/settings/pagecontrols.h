#ifndef PAGE_CONTROLS_H
#define PAGE_CONTROLS_H

#include "settingspagebase.h"

class PageControls : public SettingsPageBase {
    Q_OBJECT
public:
    explicit PageControls(QEmbyCore* core, QWidget* parent = nullptr);
};

#endif
