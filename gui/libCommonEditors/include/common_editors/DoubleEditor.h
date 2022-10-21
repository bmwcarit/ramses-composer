#ifndef DOUBLEEDITOR_H
#define DOUBLEEDITOR_H

#include "qstackedwidget.h"
#include "qgridlayout.h"
#include "property_browser/controls/SpinBox.h"
#include "property_browser/controls/ScalarSlider.h"
#include <QWidget>

using namespace raco::property_browser;
using raco::property_browser::DoubleSlider;
namespace raco::common_editors {

class DoubleEditor : public QWidget {
    Q_OBJECT
public:
    explicit DoubleEditor(QWidget *parent = nullptr);

    void setRange(double min, double max);
    void setValue(double value);
    void setWidth(int width);
    void setHeight(int height);
    double value();

Q_SIGNALS:
    void sigValueChanged(double value);
    void sigEditingFinished();
protected:
    QStackedWidget* stack_;
    DoubleSlider* slider_;
    DoubleSpinBox* spinBox_;
};
}

#endif // DOUBLEEDITOR_H
