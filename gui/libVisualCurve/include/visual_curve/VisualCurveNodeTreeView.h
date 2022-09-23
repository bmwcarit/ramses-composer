#ifndef VISUALCURVENODETREEVIEW_H
#define VISUALCURVENODETREEVIEW_H

#include <QWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QHeaderView>
#include <QDrag>
#include <QDropEvent>
#include "NodeData/nodeManager.h"
#include "AnimationData/animationData.h"
#include "signal/SignalProxy.h"
#include "VisualCurveNodeDelegate.h"
#include "FolderData/FolderDataManager.h"

using namespace raco::guiData;
namespace raco::visualCurve {

class TreeModel : public QStandardItemModel {
    Q_OBJECT
public:
    TreeModel(QWidget *parent);
    void setFolderData(FolderDataManager *mgr);
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool move(QModelIndex source, int sourceRow, QModelIndex &dest, int destRow);
signals:
    void moveOneRow(const QModelIndex &source);
private:
    FolderDataManager *folderDataMgr_{nullptr};
};

class VisualCurveNodeTreeView : public QWidget {
    Q_OBJECT
public:
    explicit VisualCurveNodeTreeView(QWidget *parent = nullptr);
    ~VisualCurveNodeTreeView();
    void initCurves();
    void switchCurSelCurve(std::string curve);
    void cancleSelCurve();

public Q_SLOTS:
    void slotInsertCurve(QString property, QString curve, QVariant value);
    void slotRefrenceBindingCurve(std::string smapleProp, std::string prop, std::string curve);
    void slotShowContextMenu(const QPoint &p);
    void slotCreateFolder();
    void slotDeleteFolder();
    void slotCreateCurve();
    void slotDeleteCurve();
    void slotItemChanged(QStandardItem *item);
    void slotCurrentRowChanged(const QModelIndex &index);
    void slotButtonDelegateClicked(const QModelIndex &index);
    void slotDeleteCurveFromVisualCurve(std::string curve);

signals:
    void sigRefreshVisualCurve();
    void sigSwitchVisualCurveInfoWidget();
private:
    void searchCurve(NodeData *pNode, std::string &property, std::string curve, std::string sampleProp);
    bool itemFromPath(QStandardItem* item, QString node);
    QString curveFromItem(QStandardItem *item);
private:
    TreeModel *model_{nullptr};
    QTreeView *visualCurveTreeView_{nullptr};
    QMenu *menu_{nullptr};
    QAction *createFolder_{nullptr};
    QAction *deleteFolder_{nullptr};
    QAction *createCurve_{nullptr};
    QAction *deleteCurve_{nullptr};
    FolderDataManager *folderDataMgr_{nullptr};
    std::string selNode_;
    ButtonDelegate *visibleButton_{nullptr};
};
}

#endif // VISUALCURVENODETREEVIEW_H
