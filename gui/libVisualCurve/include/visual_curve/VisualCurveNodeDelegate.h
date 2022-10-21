#ifndef VISUALCURVENODEDELEGATE_H
#define VISUALCURVENODEDELEGATE_H

#include "FolderData/FolderDataManager.h"
#include <QStandardItem>
#include <QStandardItemModel>
#include <QObject>
#include <QStyledItemDelegate>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QTreeView>
#include <QDebug>

namespace raco::visualCurve {
using namespace raco::guiData;

class ButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ButtonDelegate(QObject *parent = nullptr)
        :QStyledItemDelegate(parent){ }

    void setModel(QStandardItemModel *model) {
        model_ = model;
    }
    void setFolderManager(FolderDataManager *folderMgr) {
        folderMgr_ = folderMgr;
    }

signals:
    void clicked(const QModelIndex& index);

public:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
private slots:
    void clearWidget();
    void updateWidget(QModelIndex begin, QModelIndex end);
    void collapsed(const QModelIndex& index);
    void btnClicked();
private:
    void itemCollapsed(QStandardItem *item);
private:
    mutable QMap<QPersistentModelIndex,QWidget *> m_iWidgets;
private:
    QString text_;
    QIcon icon_;
    FolderDataManager *folderMgr_{nullptr};
    QStandardItemModel *model_{nullptr};
    bool isVisible{true};
};
}

#endif // VISUALCURVENODEDELEGATE_H
