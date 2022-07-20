#include "property_browser/controls/ResultTree.h"
#include "style/Colors.h"
#include "style/Icons.h"

using namespace raco::style;
namespace raco::property_browser {
ResultTree::ResultTree(QWidget *parent)
    : QDialog{parent} {
    setWindowFlags(Qt::Popup);
    setSizeGripEnabled(true);

    acceptButton_.setFlat(true);
    acceptButton_.setIcon(Icons::instance().done);
    closeButton_.setFlat(true);
    closeButton_.setIcon(Icons::instance().close);

    layout_.setContentsMargins(0, 5, 0, 0);
    layout_.addWidget(&treeWidget_, 0, 0, 1, 3);
    layout_.addWidget(&acceptButton_, 1, 0);
    layout_.addWidget(&closeButton_, 1, 2);
    layout_.setColumnStretch(1, 1);

    treeWidget_.setFixedWidth(400);

//    QObject::connect(dynamic_cast<QApplication*>(QApplication::instance()), &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) { if (now != this && !this->isAncestorOf(now)) accept(); });
    QObject::connect(&closeButton_, &QPushButton::clicked, this, &QWidget::close);
    QObject::connect(&acceptButton_, &QPushButton::clicked, this, [this]() { accept(); });
    QObject::connect(&treeWidget_, &QListWidget::doubleClicked, this, [this](const QModelIndex &index) { selectedCurve_ = index.data().toString(); accept(); });
    QObject::connect(&treeWidget_, &QListWidget::clicked, this, [this](const QModelIndex &index) { selectedCurve_ = index.data().toString(); });
}

QString ResultTree::getSelectedCurve() {
    return selectedCurve_;
}

void ResultTree::addCurve(QString curve) {
    treeWidget_.addItem(curve);
}

void ResultTree::setGeoMetry(QWidget *widget) {
    updateGeometry();
    QPoint parentLocation = widget->mapToGlobal(widget->geometry().topLeft());
    QSize size = sizeHint();
    QRect screen = QApplication::desktop()->screenGeometry(widget);
    int x = std::min( parentLocation.x() - size.width() / 2, screen.x() + screen.width() - size.width());
    int y = std::min( parentLocation.y(), screen.height()-size.height());
    move( x, y );
}

void ResultTree::clear() {
    treeWidget_.clear();
}

}
