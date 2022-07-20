#ifndef ANIMATIONEDITORVIEW_H
#define ANIMATIONEDITORVIEW_H

#include <QTreeView>
#include <QStandardItemModel>
namespace raco::time_axis{

class AnimationEditorModel : public QStandardItemModel {
    Q_OBJECT
public:
    AnimationEditorModel(QWidget *parent);
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;


Q_SIGNALS:
    void sigItemDataChanged(QVariant var, QStandardItem* item);
private:
    QVector<QString> valueVec_;
};

class AnimationEditorView : public QTreeView {
    Q_OBJECT
public:
    AnimationEditorView(QWidget *parent);
};
}

#endif // ANIMATIONEDITORVIEW_H
