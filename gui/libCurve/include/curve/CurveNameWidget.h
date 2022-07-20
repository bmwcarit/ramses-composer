#ifndef CURVENAMEWIDGET_H
#define CURVENAMEWIDGET_H

#include <QLineEdit>
#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include "CurveData/CurveManager.h"
#include "NodeData/nodeManager.h"
#include "AnimationData/animationData.h"

using namespace raco::guiData;
class CurveNameWidget : public QDialog {
public:
    CurveNameWidget(QWidget* parent = nullptr);

    void setBindingData(QString property, QString curve);
    void getBindingData(QString& property, QString& curve);

public Q_SLOTS:
    void slotOkBtnClicked();
private:
    bool isValidCurveStr(std::string sampleProperty, std::string curve);
private:
    QLineEdit* curveEdit_{nullptr};
    QPushButton* okBtn_{nullptr};
    QString property_;
    QString curve_;
    int index_{0};
};

#endif // CURVENAMEWIDGET_H
