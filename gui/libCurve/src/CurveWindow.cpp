#include "curve/CurveWindow.h"

namespace raco::curve {
CurveWindow::CurveWindow(raco::components::SDataChangeDispatcher dispatcher,
                         raco::core::CommandInterface *commandInterface, CurveLogic *curveLogic,
	                     NodeLogic *nodeLogic, QWidget *parent) :
                         QWidget{parent},
                         commandInterface_ {commandInterface},
						 curveLogic_{curveLogic},
						 nodeLogic_{nodeLogic} {
    QHBoxLayout* hLayout = new QHBoxLayout(this);
	curveTree_ = new CurveTree(this, curveLogic_, nodeLogic_);
    curveTree_->setHeaderHidden(true);
    hLayout->addWidget(curveTree_, 3);
    this->setLayout(hLayout);
	slotRefreshCurveView();
    connect(curveTree_, &CurveTree::sigRefreshCurveView, this, &CurveWindow::slotRefreshCurveView);
    connect(curveTree_, &CurveTree::sigUpdateCurve, this, &CurveWindow::slotUpdateCurve);
    connect(curveLogic, &CurveLogic::sigRefreshCurveView, this, &CurveWindow::slotRefreshCurveView);
}

void CurveWindow::slotRefreshCurveView() {
    std::list<Curve*> curveList = curveLogic_->getCurveList();

    if (model_) {
        delete model_;
        model_ = nullptr;
    }

    model_ = new CurveTreeModel(curveTree_, curveLogic_);
    curveTree_->setModel(model_);
    curveTree_->slotDelPointPropertyView();
    model_->initModel(curveList);
    curveTree_->setItemExpandStatus();

    Q_EMIT signalProxy::GetInstance().sigRepaintTimeAixs_From_CurveUI();
    Q_EMIT signalProxy::GetInstance().sigCheckCurveBindingValid_From_CurveUI();
}

void CurveWindow::slotUpdateCurve() {
    Q_EMIT signalProxy::GetInstance().sigRepaintTimeAixs_From_CurveUI();
}
}
