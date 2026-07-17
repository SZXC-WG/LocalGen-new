// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QComboBox>
#include <QProxyStyle>

namespace ComboBoxPopupCompatibility {

class ManagedPopupStyle final : public QProxyStyle {
   public:
    using QProxyStyle::QProxyStyle;

    int styleHint(StyleHint hint, const QStyleOption* option,
                  const QWidget* widget,
                  QStyleHintReturn* returnData) const override {
        if (hint == QStyle::SH_ComboBox_Popup) return 0;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

inline void configureForManagedPopup(QComboBox* combo,
                                     int maxVisibleItems = 10) {
    combo->setMaxVisibleItems(maxVisibleItems);
#ifdef Q_OS_LINUX
    auto* popupStyle = new ManagedPopupStyle(
        combo->style() ? combo->style()->name() : QString());
    popupStyle->setParent(combo);
    combo->setStyle(popupStyle);
#endif
}

}  // namespace ComboBoxPopupCompatibility
