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
#include <QMouseEvent>
#include "NodeData/nodeManager.h"
#include "AnimationData/animationData.h"
#include "signal/SignalProxy.h"
#include "VisualCurveNodeDelegate.h"
#include "FolderData/FolderDataManager.h"
#include "CurveData/CurveManager.h"

using namespace raco::guiData;
namespace raco::visualCurve {

class TreeModel : public QStandardItemModel {
    Q_OBJECT
public:
    TreeModel(QWidget *parent);
    void setFolderDataMgr(FolderDataManager *mgr);
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool move(QModelIndex source, int sourceRow, QModelIndex &dest, int destRow);
    void swapCurve(std::string oldCurve, std::string newCurve);
    void setFolderPath(Folder *folder, std::string path);
private:
    bool moveCurveToNode(Folder *srcFolder, STRUCT_CURVE_PROP *srcCurveProp, std::string srcCurvePath, std::string destCurvePath);
    bool moveCurveToDefaultNode(Folder *srcFolder, STRUCT_CURVE_PROP *srcCurveProp, std::string srcCurvePath);
    bool moveFolderToNode(Folder *srcFolder, std::string srcCurvePath, std::string destCurvePath);
    bool moveFolderToDefaultNode(Folder *srcFolder, std::string srcCurvePath);
signals:
    void moveRowFinished(std::string dest);
private:
    FolderDataManager *folderDataMgr_{nullptr};
};

class VisualCurveNodeTreeView : public QWidget {
    Q_OBJECT
public:
    explicit VisualCurveNodeTreeView(QWidget *parent,  raco::core::CommandInterface* commandInterface);
    ~VisualCurveNodeTreeView();
    void initCurves();
    void switchCurSelCurve(std::string curve);
    void cancelSelCurve();

public Q_SLOTS:
    void slotInsertCurve(QString property, QString curve, QVariant value);
    void slotRefrenceBindingCurve(std::string smapleProp, std::string prop, std::string curve);
    void slotShowContextMenu(const QPoint &p);
    void slotCreateFolder();
    void slotCreateCurve();
    void slotDelete();
    void slotItemChanged(QStandardItem *item);
    void slotCurrentRowChanged(const QModelIndex &index);
    void slotButtonDelegateClicked(const QModelIndex &index);
    void slotDeleteCurveFromVisualCurve(std::string curve);
    void slotModelMoved(std::string dest);
    void slotRefreshWidget();

signals:
    void sigRefreshVisualCurve();
    void sigSwitchVisualCurveInfoWidget();
private:
    void setFolderVisible(Folder *folder, bool visible);
    void searchCurve(NodeData *pNode, std::string &property, std::string curve, std::string sampleProp);
    bool itemFromPath(QStandardItem **item, QString node);
    QString curveFromItem(QStandardItem *item);
    void pushState2UndoStack(std::string description);
    void initItemFromFolder(QStandardItem *item, Folder* folder);
    void swapCurve(std::string oldCurve, std::string newCurve);
    void setFolderPath(Folder *folder, std::string path);
    void deleteFolder(QStandardItem *item);
    void deleteCurve(QStandardItem *item);
    bool sortIndex(const QModelIndex &index1, const QModelIndex &index2);
private:
    TreeModel *model_{nullptr};
    QTreeView *visualCurveTreeView_{nullptr};
    QMenu *menu_{nullptr};
    QAction *createFolder_{nullptr};
    QAction *delete_{nullptr};
    QAction *createCurve_{nullptr};
    FolderDataManager *folderDataMgr_{nullptr};
    std::string selNode_;
    ButtonDelegate *visibleButton_{nullptr};
    raco::core::CommandInterface* commandInterface_{nullptr};
};
}

#endif // VISUALCURVENODETREEVIEW_H
