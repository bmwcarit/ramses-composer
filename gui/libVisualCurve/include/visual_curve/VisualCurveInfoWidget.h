#ifndef VISUALCURVEINFOWIDGET_H
#define VISUALCURVEINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLayout>
#include <QStackedWidget>
#include <QSpacerItem>
#include <QFormLayout>
#include "common_editors/DoubleEditor.h"
#include "common_editors/Int64Editor.h"
#include "time_axis/TimeAxisCommon.h"

using namespace raco::time_axis;
using namespace raco::common_editors;
namespace raco::visualCurve {
class VisualCurveInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VisualCurveInfoWidget(QWidget *parent = nullptr);

    void initVisualCurveKeyWidget();
    void initVisualCurveCursorWidget();
    void setKeyWidgetVisible();
    void setCursorWidgetVisible();

public Q_SLOTS:
    void slotSwitchInterpolationMode(int index);
    void slotKeyFrameChanged(int value);
    void slotKeyValueChanged(double value);
    void slotLeftKeyFrameChanged(int value);
    void slotLeftKeyValueChanged(double value);
    void slotRightKeyFrameChanged(int value);
    void slotRightKeyValueChanged(double value);
    void slotLeftTangentChanged(double value);
    void slotRightTangentChanged(double value);

    void slotCursorShow(bool checked);
    void slotCursorXChanged(int value);
    void slotCursorYChanged(double value);

    void slotUpdateSelKey();
    void slotUpdateCursorX();

signals:
    void sigRefreshCursorX();
    void sigRefreshVisualCurve();
    void sigSwitchCurveType(int type);
private:
    void switchCurveType(int type);
    void updateSelKey();
    void updateCursorX();
    void recaculateWorkerLeftPoint(QPair<QPointF, QPointF> &workerPoint, SKeyPoint keyPoint);
    void recaculateWorkerRightPoint(QPair<QPointF, QPointF> &workerPoint, SKeyPoint keyPoint);

private:
    QWidget *visualCurveKeyWidget_{nullptr};
    QWidget *visualCurveCursorWidget_{nullptr};
    QStackedWidget *centreWidget_{nullptr};

    QComboBox *interpolationComboBox_{nullptr};
    Int64Editor *keyFrameSpinBox_{nullptr};
    DoubleEditor *keyValueSpinBox_{nullptr};

    QLabel *leftValueLabel_{nullptr};
    QLabel *rightValueLabel_{nullptr};
    QLabel *leftFrameLabel_{nullptr};
    QLabel *rightFrameLabel_{nullptr};
    QLabel *leftTangentLabel_{nullptr};
    QLabel *rightTangentLabel_{nullptr};
    Int64Editor *leftFrameSpinBox_{nullptr};
    DoubleEditor *leftTangentSpinBox_{nullptr};
    DoubleEditor *leftValueSpinBox_{nullptr};
    Int64Editor *rightFrameSpinBox_{nullptr};
    DoubleEditor *rightTangentSpinBox_{nullptr};
    DoubleEditor *rightValueSpinBox_{nullptr};

    QGridLayout *cursorLayout_{nullptr};
    QCheckBox *showCursorCheckBox_{nullptr};
    Int64Editor *cursorXSpinBox_{nullptr};
    DoubleEditor *cursorYSpinBox_{nullptr};
};
}

#endif // VISUALCURVEINFOWIDGET_H
