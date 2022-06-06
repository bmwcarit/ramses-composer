#ifndef TIMEAXISMAINWINDOW_H
#define TIMEAXISMAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QToolBar>
#include <QMessageBox>
#include <QMenu>
#include "components/DataChangeDispatcher.h"
#include "core/CommandInterface.h"
#include "time_axis/TimeAxisWidget.h"
#include "time_axis/TimeAxisScrollArea.h"
#include "time_axis/AnimationEditorView.h"
#include "signal/SignalProxy.h"
#include <QItemSelectionModel>

using namespace raco::signal;
namespace Ui {
class TimeAxisMainWindow;
}

namespace raco::time_axis {
class AnimationButton : public QWidget {
    Q_OBJECT

public:
    AnimationButton(QWidget* parent = nullptr);

private:
    QPushButton *startBtn_;
    QPushButton *previousBtn_;

};

class TimeAxisMainWindow final : public QMainWindow {
Q_OBJECT
public:
    TimeAxisMainWindow(raco::components::SDataChangeDispatcher dispatcher,
                       raco::core::CommandInterface* commandInterface,
                       QWidget* parent = nullptr);

public Q_SLOTS:
    void slotInitAnimationMgr();
    //
    void slotCreateKeyFrame();
    void slotRefreshTimeAxis();
    void startOrStopAnimation();
    void slotUpdateAnimation();
    void slotUpdateAnimationKey(QString oldKey, QString newKey);
private Q_SLOTS:
    void slotTreeMenu(const QPoint &pos);
    void slotLoad();
    void slotCopy();
    void slotPaste();
    void slotDelete();
    void slotProperty();
    void slotCreateNew();
    void slotCurrentRowChanged(const QModelIndex &index);
    void slotItemChanged(QStandardItem *item);
private:
    bool initTitle(QWidget *parent);
    bool initTree(QWidget *parent);
	bool initAnimationMenu();
    void loadOperation();

private:
    TimeAxisWidget *timeAxisWidget_;
    TimeAxisScrollArea *timeAxisScrollArea_;
    QVBoxLayout *vBoxLayout_;
    QHBoxLayout *hBoxLayout;
    QHBoxLayout *hTitleLayout;
    raco::core::CommandInterface *commandInterface_;

    QWidget *titleWidget_;
    AnimationEditorView *editorView_;
    QPushButton *startBtn_;
    bool animationStarted_{false};
    //--------------------------------------------
	QMenu m_Menu;
    QAction *m_pLoad{nullptr};
    QAction *m_pCopy{nullptr};
    QAction *m_pDelete{nullptr};
    QAction *m_pProperty{nullptr};
    QAction *pasteAction_{nullptr};
private:
    int UUID_{1};
    QStandardItemModel* model_{nullptr};
    QString curItemName_;
    QString copyItemName_;
    QMap<QString, QStandardItem*> itemMap_;
    QLineEdit *lineBegin_{nullptr};
    QLineEdit *lineEnd_{nullptr};
};
}

#endif // TIMEAXISMAINWINDOW_H
