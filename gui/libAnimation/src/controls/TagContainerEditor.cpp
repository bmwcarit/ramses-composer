#include "animation/controls/TagContainerEditor.h"

#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QMenu>
#include <QObject>
#include <QPalette>
#include <QPushButton>

#include "common_widgets/NoContentMarginsLayout.h"

namespace raco::animation {

TagContainerEditor::TagContainerEditor(QWidget* parent)
    : QWidget{parent} {
    this->setLayout(new raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>(this));

    label_ = new QLabel(this);
    layout()->addWidget(label_);

    setFixedHeight(15);
}

void TagContainerEditor::setText(const QString& t) {
    label_->setText(t);
}

}
