#include "time_axis/AnimationEditorView.h"

namespace raco::time_axis{

AnimationEditorView::AnimationEditorView(QWidget *parent) : QTreeView(parent) {
    setHeaderHidden(true);
}

AnimationEditorModel::AnimationEditorModel(QWidget *parent) : QStandardItemModel(parent) {
	
}

Qt::ItemFlags AnimationEditorModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
            return Qt::NoItemFlags;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags |= Qt::ItemIsEditable;

    return flags;
}

bool AnimationEditorModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if (index.isValid() && role == Qt::EditRole)
    {
        QStandardItem *item = itemFromIndex(index);
        Q_EMIT sigItemDataChanged(value, item);
        item->setData(value);

        emit dataChanged(index, index);
        return true;
    }
    return false;
}
}
