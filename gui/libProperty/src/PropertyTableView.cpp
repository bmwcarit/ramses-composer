#include "property/PropertyTableView.h"

namespace raco::property {

PropertyModel::PropertyModel(TABLETYPE type, QObject *parent) :
    QAbstractTableModel(parent), type_(type) {

    roleMap.insert(static_cast<int>(PROPERTY_TYPE_FLOAT), "FLOAT");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_INT), "INT");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_BOOL), "BOOL");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_STRING), "STRING");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_SAMPLETEXTURE), "SAMPLETEXTURE");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC2i), "VEC2i");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC3i), "VEC3i");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC4i), "VEC4i");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC2f), "VEC2f");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC3f), "VEC3f");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_VEC4f), "VEC4f");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_MAT2), "MTX2");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_MAT3), "MTX3");
    roleMap.insert(static_cast<int>(PROPERTY_TYPE_MAT4), "MTX4");

    parent_ = dynamic_cast<PropertyTableView*>(parent);
    switch(type_) {
    case PROPERTY_SYSTEM: {

        break;
    }
    case PROPERTY_CUSTOM: {
        std::map<std::string, EPropertyType> customTypeMap  = PropertyDataManager::GetInstance().getCustomPropertyTypeMap();
        for (auto it : customTypeMap) {
            dataMap_.emplace(QString::fromStdString(it.first), it.second);
        }
        break;
    }
    case PROPERTY_ANIMATION: {

        break;
    }
    default:
        break;
    }

    for (const auto &it : dataMap_) {
        keyVec_.push_back(it.first);
        typeVec_.push_back(it.second);
    }
}

void PropertyModel::initModel() {
    switch(type_) {
    case PROPERTY_SYSTEM: {

        break;
    }
    case PROPERTY_CUSTOM: {
        std::map<std::string, EPropertyType> customTypeMap  = PropertyDataManager::GetInstance().getCustomPropertyTypeMap();
        for (auto it : customTypeMap) {
            dataMap_.emplace(QString::fromStdString(it.first), it.second);
			insertRow(rowCount(QModelIndex()));
        }
        break;
    }
    case PROPERTY_ANIMATION: {

        break;
    }
    default:
        break;
    }

    for (const auto &it : dataMap_) {
		keyVec_.push_back(it.first);
		typeVec_.push_back(it.second);
    }
}

QVariant PropertyModel::data(const QModelIndex &index, int role) const {
    switch (role) {
    case Qt::DisplayRole: {
        if (index.column() == 0 && index.row() < keyVec_.size()) {
            return keyVec_[index.row()];
        }
        else if (index.column() == 1 && index.row() < typeVec_.size()) {
			int type = static_cast<int>(typeVec_[index.row()]);
			return roleMap.value(type);
        }
    }
    }

    return QVariant();
}

bool PropertyModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    do {
        if (role == Qt::DisplayRole && index.column() == 1) {
            typeVec_[index.row()] = static_cast<EPropertyType>(roleMap.key(value.toString()));
            switch(type_) {
            case PROPERTY_SYSTEM: {

                break;
            }
            case PROPERTY_CUSTOM: {
                PropertyDataManager::GetInstance().modifyCustomProItem(keyVec_[index.row()].toStdString(), static_cast<EPropertyType>(roleMap.key(value.toString())));
                Q_EMIT signalProxy::GetInstance().sigUpdateCustomProperty_From_PropertyUI();
                break;
            }
            case PROPERTY_ANIMATION: {

                break;
            }
            default:
                break;
            }
        }

        if(role == Qt::DisplayRole && index.column() == 0) {
            if (std::find(keyVec_.begin(), keyVec_.end(), value.toString()) != keyVec_.end()) {
                return false;
            }
            QString oldName = keyVec_[index.row()];
            keyVec_[index.row()] = value.toString();

            switch(type_) {
            case PROPERTY_SYSTEM: {
                break;
            }
            case PROPERTY_CUSTOM: {
                PropertyDataManager::GetInstance().deleteCustomProItem(oldName.toStdString());
                PropertyDataManager::GetInstance().insertCustomProItem(value.toString().toStdString(), typeVec_[index.row()]);
                Q_EMIT signalProxy::GetInstance().sigUpdateCustomProperty_From_PropertyUI();
                break;
            }
            case PROPERTY_ANIMATION: {
                break;
            }
            default:
                break;
            }
        }

        return true;
    }while(false);

    return false;
}

int PropertyModel::rowCount(const QModelIndex &parent) const {
    return dataMap_.size();
}

int PropertyModel::columnCount(const QModelIndex &parent) const {
    return 2;
}

QVariant PropertyModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return "Name";
        } else if (section == 1) {
            return "DataType";
        }

    }
    return QVariant();
}

Qt::ItemFlags PropertyModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

bool PropertyModel::insertRows(int row, int count, const QModelIndex &parent) {
    if (count == 0) {
        return false;
    }

    if (!parent.isValid()) {
        beginInsertRows(QModelIndex(), row, row + count - 1);
    }
    else {
        beginInsertRows(parent, row, row + count - 1);
    }

    key_++;
    QString name = QString("property") + QString::number(key_);

    keyVec_.push_back(name);
    typeVec_.push_back(EPropertyType::PROPERTY_TYPE_FLOAT);

    switch(type_) {
    case PROPERTY_SYSTEM: {

        break;
    }
    case PROPERTY_CUSTOM: {
        dataMap_.emplace(name, EPropertyType::PROPERTY_TYPE_FLOAT);
        PropertyDataManager::GetInstance().insertCustomProItem(name.toStdString(), EPropertyType::PROPERTY_TYPE_FLOAT);
        Q_EMIT signalProxy::GetInstance().sigUpdateCustomProperty_From_PropertyUI();
        break;
    }
    case PROPERTY_ANIMATION: {
        break;
    }
    default:
        break;
    }

    endInsertRows();

    return true;
}

bool PropertyModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (count == 0) {
        return false;
    }

    if (!(std::find(keyVec_.begin(), keyVec_.end(), keyVec_[row]) != keyVec_.end()) ||
            !(std::find(typeVec_.begin(), typeVec_.end(), typeVec_[row]) != typeVec_.end())) {
        return false;
    }

    if (!parent.isValid()) {
        beginRemoveRows(QModelIndex(), row, row + count - 1);
    }
    else {
        beginRemoveRows(parent, row, row + count - 1);
    }

    std::vector<QString>::iterator itKey = std::find(keyVec_.begin(), keyVec_.end(), keyVec_[row]);
    QString name = keyVec_[row];
    keyVec_.erase(itKey);

    std::vector<EPropertyType>::iterator itType = std::find(typeVec_.begin(), typeVec_.end(), typeVec_[row]);
    typeVec_.erase(itType);

    switch(type_) {
    case PROPERTY_SYSTEM: {

        break;
    }
    case PROPERTY_CUSTOM: {
        auto it = dataMap_.find(name);
        if (it != dataMap_.end()) {
            dataMap_.erase(it);
        }
        PropertyDataManager::GetInstance().deleteCustomProItem(name.toStdString());
        Q_EMIT signalProxy::GetInstance().sigUpdateCustomProperty_From_PropertyUI();
        break;
    }
    case PROPERTY_ANIMATION: {

        break;
    }
    default:
        break;
    }

    endRemoveRows();

    return true;
}

PropertyTableView::PropertyTableView(TABLETYPE type, QWidget *parent) :
    QTableView(parent), type_(type) {
    this->verticalHeader()->setVisible(false);
    sysModel = new PropertyModel(type, this);
    this->setModel(sysModel);

    PropertyTextDelegate* textDelegate = new PropertyTextDelegate(this);
    setItemDelegateForColumn(0, textDelegate);

    PropertyTableDelegate *boxDelegate = new PropertyTableDelegate(this);
    setItemDelegateForColumn(1, boxDelegate);

    this->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);

    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &PropertyTableView::slotResetProperty);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitPropertyView, this, &PropertyTableView::slotInitPropertyView);
}

void PropertyTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList list = this->selectedIndexes();
    if (list.size() > 0) {
        Q_EMIT propertySelectedChanged(true);
    }
    else {
        Q_EMIT propertySelectedChanged(false);
    }
}

void PropertyTableView::addProperty() {
    sysModel->insertRow(this->model()->rowCount());
}

void PropertyTableView::delProperty() {
    QModelIndexList list = this->selectedIndexes();

    QMap<int, int> rowMap;
    foreach (QModelIndex index, list) {
        rowMap.insert(index.row(), 0);
    }

    QMapIterator<int, int> itor(rowMap);
    itor.toBack();
    while (itor.hasPrevious())
    {
        itor.previous();
        sysModel->removeRow(itor.key());
    }
}

void PropertyTableView::slotInitPropertyView() {
    sysModel->initModel();
}

void PropertyTableView::slotResetProperty() {
    int size = sysModel->rowCount(QModelIndex());
    sysModel->removeRows(0, size);
}

void PropertyTableView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QModelIndex index = this->indexAt(event->pos());
        if (!index.isValid()) {
            this->clearSelection();
            this->setCurrentIndex(QModelIndex());
        }
    }
    this->viewport()->repaint();
    QTableView::mousePressEvent(event);
}

}
