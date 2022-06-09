#ifndef CURVEWINDOW_H
#define CURVEWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QHBoxLayout>

#include "components/DataChangeDispatcher.h"
#include "core/CommandInterface.h"
#include "CurveTree.h"

namespace raco::curve {
class CurveWindow : public QWidget {
    Q_OBJECT
public:
    explicit CurveWindow(raco::components::SDataChangeDispatcher dispatcher,
                         raco::core::CommandInterface* commandInterface,
                         CurveLogic* curveLogic,
                         QWidget* parent = nullptr);

public Q_SLOTS:
    void slotRefreshCurveView();
private:
    raco::core::CommandInterface *commandInterface_;
    CurveTree* curveTree_{nullptr};
    CurveTreeModel* model_{nullptr};
    CurveLogic* curveLogic_{nullptr};
};
}

#endif // CURVEWINDOW_H
