#include "animation/controls/ExpandButton.h"

#include <QStyle>
#include <QWidget>

#include "style/Icons.h"

namespace raco::animation {

using namespace ::raco::style;

ExpandButton::ExpandButton(QWidget* parent)
    : QPushButton{parent} {
    // auto icon = Icons::icon(Pixmap::collapsed, this);
    auto icon = Icons::instance().collapsed;
    setIcon(icon);
    setContentsMargins(0, 0, 0, 0);
    setFlat(true);

    QObject::connect(this, &ExpandButton::clicked, this, &ExpandButton::updateIcon);
}

void ExpandButton::updateIcon() {
    bIsExpanded = !bIsExpanded;
    if (bIsExpanded) {
        // setIcon(Icons::icon(Pixmap::expanded, this));
        setIcon(Icons::instance().expanded);
    } else {
        // setIcon(Icons::icon(Pixmap::collapsed, this));
        setIcon(Icons::instance().collapsed);
    }
    Q_EMIT expandBtnClicked(bIsExpanded);
}


}
