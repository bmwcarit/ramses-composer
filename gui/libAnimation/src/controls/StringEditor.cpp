#include "animation/controls/StringEditor.h"

#include <QBoxLayout>
#include <QColor>
#include <QDebug>
#include <QDesktopServices>
#include <QFocusEvent>
#include <QMenu>
#include <QObject>
#include <QPalette>
#include <QPushButton>

#include "style/Icons.h"

#include "common_widgets/NoContentMarginsLayout.h"
#include "core/Queries.h"
#include "style/Colors.h"

namespace raco::animation {

StringEditor::StringEditor(QWidget* parent)
    : QWidget{parent} {
    this->setLayout(new raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>(this));
    lineEdit_ = new QLineEdit(this);
    layout()->addWidget(lineEdit_);

    // simple reset of outdate color on editingFinished
    QObject::connect(lineEdit_, &QLineEdit::editingFinished, this, [this]() {
        updatedInBackground_ = false;
    });

    setStyleSheet("QWidget{background-color:black;}");
}

void StringEditor::setText(const QString& t) {
    if (lineEdit_->hasFocus() && editingStartedByUser() && t != lineEdit_->text()) {
        updatedInBackground_ = true;
    } else {
        lineEdit_->setText(t);
        Q_EMIT textChanged(t);
    }
}

bool StringEditor::editingStartedByUser() {
    return lineEdit_->isModified() || lineEdit_->cursorPosition() != lineEdit_->text().size();
}

bool StringEditor::updatedInBackground() const {
    return updatedInBackground_;
}

int StringEditor::errorLevel() const noexcept {
    return static_cast<int>(errorLevel_);
}

}
