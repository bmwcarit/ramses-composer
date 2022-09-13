#ifndef INT64EDITOR_H
#define INT64EDITOR_H

#include "qstackedwidget.h"
#include "qgridlayout.h"
#include "property_browser/controls/SpinBox.h"
#include "property_browser/controls/ScalarSlider.h"
#include <QWidget>

using namespace raco::property_browser;
using raco::property_browser::DoubleSlider;
namespace raco::common_editors {
class Int64Editor : public QWidget {
    Q_OBJECT
public:
    explicit Int64Editor(QWidget *parent = nullptr);

    void setRange(int min, int max);
    void setValue(int value);
    void setWidth(int width);
    void setHeight(int height);

Q_SIGNALS:
    void sigValueChanged(int value);
protected:
    QStackedWidget* stack_;
    Int64SpinBox* spinBox_;
    Int64Slider* slider_;
};
}

#endif // INT64EDITOR_H
