#include "property_browser/PropertyBrowserMeshWidget.h"
#include "NodeData/nodeManager.h"


namespace raco::property_browser {
PropertyBrowserMeshWidget::PropertyBrowserMeshWidget(QWidget* parent)
	: QWidget{parent} {
    QGridLayout *layout = new QGridLayout{this};
	nameLabel_ = new QLabel{QString("Name"), this};
	materialLabel_ = new QLabel{QString("Material"), this};
    nameEditor_ = new QLineEdit(this);
    materialEditor_ = new QLineEdit(this);
    layout->setContentsMargins(35, 0, 0, 0);
    layout->addWidget(nameLabel_, 0, 0);
    layout->addWidget(nameEditor_, 0, 1);
    layout->addWidget(materialLabel_, 1, 0);
    layout->addWidget(materialEditor_, 1, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 2);
	setLayout(layout);

    QObject::connect(nameEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserMeshWidget::slotUpdateMeshName);
	QObject::connect(materialEditor_, &QLineEdit::editingFinished, this, &PropertyBrowserMeshWidget::slotUpdateMaterial);
}

void PropertyBrowserMeshWidget::initPropertyBrowserMeshWidget() {
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();

	if (pNode) {
		std::string name = pNode->NodeExtendRef().meshPropRef().modifyName();
		nameEditor_->setText(QString::fromStdString(name));
		std::string material = pNode->NodeExtendRef().meshPropRef().modifyMaterial();
		materialEditor_->setText(QString::fromStdString(material));
	}
}

void PropertyBrowserMeshWidget::clearMesh() {
    nameEditor_->clear();
    materialEditor_->clear();
}

void PropertyBrowserMeshWidget::slotUpdateMeshName() {
    QString name = nameEditor_->text();
    qDebug() << "Mesh name changed :" << name;
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	if (pNode)
		pNode->NodeExtendRef().meshPropRef().modifyName(name.toStdString());
}

void PropertyBrowserMeshWidget::slotUpdateMaterial() {
    QString material = materialEditor_->text();
    qDebug() << "Mesh material changed :" << material;
    NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	if (pNode)
		pNode->NodeExtendRef().meshPropRef().modifyMaterial(material.toStdString());
}
}  // namespace raco::property_browser
