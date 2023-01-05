/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "property_browser/PropertySubtreeView.h"

#include "ErrorBox.h"
#include "core/CoreFormatter.h"
#include "property_browser/PropertyBrowserLayouts.h"
#include "property_browser/WidgetFactory.h"
#include "property_browser/controls/ExpandButton.h"
#include "property_browser/editors/LinkEditor.h"
#include "property_browser/editors/PropertyEditor.h"
#include "user_types/LuaScript.h"
#include "user_types/LuaScriptModule.h"

#include "MaterialData/materialManager.h"
#include "NodeData/nodeManager.h"
#include "material_logic/materalLogic.h"
#include "data_Convert/ProgramDefine.h"

#include "data_storage/Table.h"
#include "data_storage/Value.h"
#include "user_types/Material.h"
#include <QApplication>
#include <QDebug>
#include <QFormLayout>
#include <QFrame>
#include <QMessageBox>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <set>
namespace raco::property_browser {

void drawHighlight(QWidget* widget, float intensity) {
	if (intensity > 0) {
		QPainter painter{widget};
		QPen pen = painter.pen();
		pen.setStyle(Qt::PenStyle::NoPen);
		painter.setPen(pen);
		painter.setBrush(qApp->palette().highlight());
		painter.drawRect(widget->rect());
	}
}

EmbeddedPropertyBrowserView::EmbeddedPropertyBrowserView(PropertyBrowserItem* item, QWidget* parent)
	: QFrame{parent} {
}

static std::set<std::string> checkUniformName_;

PropertySubtreeView::PropertySubtreeView(PropertyBrowserModel* model, PropertyBrowserItem* item, QWidget* parent)
	: QWidget{parent}, item_{item}, model_{model}, layout_{this} {
	layout_.setAlignment(Qt::AlignTop);
	setContextMenuPolicy(Qt::CustomContextMenu);
	// .PropertySubtreeView--------------------------------------------------------------.
	// | expand button |  label + margin       | link control | property / value control |
	// .PropertySubtreeChildrenContainer-------------------------------------------------.
	// |               | PropertySubtreeView                                             |
	// | button margin | PropertySubtreeView                                             |
	// |               | PropertySubtreeView                                             |
	// .---------------------------------------------------------------------------------.

	labelContainer_ = new QWidget{this};
	auto* labelLayout = new PropertyBrowserHBoxLayout{labelContainer_};

	labelLayout->setAlignment(Qt::AlignLeft);

	if (!item->valueHandle().isObject()) {
		if (item->parentItem() && item->parentItem()->parentItem() && item->parentItem()->displayName() == "uniforms") {
			palette_ = labelContainer_->palette();
			isUniform_ = true;
		}

		label_ = WidgetFactory::createPropertyLabel(item, labelContainer_);
		label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

		if (item->expandable()) {
			decorationWidget_ = new ExpandButton(item, labelContainer_);
		} else {
			// Easier to make a dummy spacer widget than figuring out how to move everything else at the right place without it
			decorationWidget_ = new QWidget(this);
			decorationWidget_->setFixedHeight(0);
		}
		decorationWidget_->setFixedWidth(28);

		auto* linkControl = WidgetFactory::createLinkControl(item, labelContainer_);
		propertyControl_ = WidgetFactory::createPropertyEditor(item, labelContainer_);

		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
		labelLayout->addWidget(linkControl, 1);
		if (item->displayName() == "material") {
			//QObject::connect(item, &PropertyBrowserItem::valueChanged, this, &PropertySubtreeView::updateMaterial);
		}
        if (item->displayName() == "uniforms") {
            setUniformControls(item, labelLayout);
			checkUniformName_.clear();
			auto material = item->siblingItem("material");
			QObject::connect(material, &PropertyBrowserItem::valueChanged, this, &PropertySubtreeView::updateMaterial);
			updateUniformCombox();
		}
        QObject::connect(item, &PropertyBrowserItem::valueChanged, this, &PropertySubtreeView::updateMesh);

		auto isLuaScriptProperty = !item->valueHandle().isObject() && &item->valueHandle().rootObject()->getTypeDescription() == &raco::user_types::LuaScript::typeDescription && !item->valueHandle().parent().isObject();
		if (isLuaScriptProperty) {
			label_->setToolTip(QString::fromStdString(item->luaTypeName()));
		}

		linkControl->setControl(propertyControl_);
		label_->setEnabled(item->editable());
		QObject::connect(item, &PropertyBrowserItem::editableChanged, label_, &QWidget::setEnabled);
	} else {
		// Dummy label for the case that we are an object.
		decorationWidget_ = new QWidget{this};
		label_ = new QLabel{this};
		decorationWidget_->setFixedWidth(0);
		decorationWidget_->setFixedHeight(0);
		label_->setFixedWidth(0);
		label_->setFixedHeight(0);
		labelLayout->addWidget(decorationWidget_, 0);
		labelLayout->addWidget(label_, 0);
	}

	QObject::connect(item, &PropertyBrowserItem::errorChanged, this, &PropertySubtreeView::updateError);
	updateError();

	layout_.addWidget(labelContainer_, 1, 0);

	if (item->expandable()) {
		// Events which can cause a build or destroy of the childrenContainer_
		QObject::connect(item, &PropertyBrowserItem::childrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::showChildrenChanged, this, &PropertySubtreeView::updateChildrenContainer);
		QObject::connect(item, &PropertyBrowserItem::childrenChangedOrCollapsedChildChanged, this, &PropertySubtreeView::playStructureChangeAnimation);
		updateChildrenContainer();
	}

	insertKeyFrameAction_ = new QAction("insert KeyFrame", this);
	copyProperty_ = new QAction("copy Property", this);
	connect(insertKeyFrameAction_, &QAction::triggered, this, &PropertySubtreeView::slotInsertKeyFrame);
	connect(copyProperty_, &QAction::triggered, this, &PropertySubtreeView::slotCopyProperty);
	// Add right-click menu bar
	connect(this, &PropertySubtreeView::customContextMenuRequested, this, &PropertySubtreeView::slotTreeMenu);
}

void PropertySubtreeView::updateMaterial(raco::core::ValueHandle& v) {
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	pNode->uniformClear();
    updateUniformCombox();
}

void PropertySubtreeView::updateMesh(core::ValueHandle &v) {
    core::ValueHandle parent = v.parent();
    if (parent.isProperty()) {
        std::string parentProp = parent.getPropName();
        if (parentProp == "translation" || parentProp == "rotation" || parentProp == "scale") {
            Q_EMIT signalProxy::GetInstance().sigUpdateMeshModelMatrix();
        }
    }
}

std::vector<Uniform> PropertySubtreeView::Item2Uniform(PropertyBrowserItem* item) {
	raco::core::ValueHandle handle = item->valueHandle();
	std::vector<Uniform> uniforms;
	MaterialData materialData;
	raco::material_logic::MateralLogic materialLogic;
	materialLogic.setUniformsProperty(item->valueHandle(), materialData);
	uniforms.clear();
	for (auto& un : materialData.getUniforms()) {
		uniforms.push_back(un);
	}
	for (auto& it : materialData.getTextures()) {
		Uniform unifor;
		unifor.setName(it.getUniformName());
		unifor.setType(UniformType::String);
		unifor.setValue(it.getName());
		uniforms.push_back(unifor);
	}
	return uniforms;
}

void PropertySubtreeView::setUniformControls(PropertyBrowserItem* item, PropertyBrowserHBoxLayout* labelLayout) {
	std::vector<Uniform> uniforms = Item2Uniform(item);
	// Add all uniform properties to curUniform in MaterialManager
	raco::guiData::MaterialManager::GetInstance().curUniformClear();
	for (auto& un : uniforms) {
		raco::guiData::MaterialManager::GetInstance().addCurUniform(un);
	}
	uniformComBox_ = new QComboBox(labelContainer_);
	uniformDelButton_ = new QPushButton{QString("del"), labelContainer_};
	uniformDelButton_->setStyleSheet(
		"QPushButton{background-color:#141414; border-radius:7px;} \
		 QPushButton:hover{background-color:#5a5a5a; border-radius:7px;}");
	uniformComBox_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	uniformDelButton_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	labelLayout->addStretch(1);
	labelLayout->addWidget(uniformDelButton_, Qt::AlignRight);
	labelLayout->addWidget(uniformComBox_, Qt::AlignRight);
	QObject::connect(uniformComBox_, &QComboBox::textActivated, this, &PropertySubtreeView::slotUniformNameChanged);
	QObject::connect(uniformDelButton_, &QPushButton::clicked, this, &PropertySubtreeView::delUniformButtonClicked);

	for (auto& child : item->children()) {
		bool isShowed = false;
		NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
		for (auto& un : pNode->getUniforms()) {
			if (child->displayName() == un.getName()) {
				isShowed = true;
				break;
			}
		}
		if (!isShowed && child->displayName() != PTW_FRAG_SYS_OPACITY) {
			uniformComBox_->addItem(QString::fromStdString(child->displayName()));
		}
	}

	// Display content centered
	QLineEdit* lineEdit = new QLineEdit;
	lineEdit->setReadOnly(true);
	lineEdit->setAlignment(Qt::AlignCenter);

	lineEdit->setStyleSheet("QLineEdit{background-color:#14141400;}");
	uniformComBox_->setLineEdit(lineEdit);

	// Drop-down menu content centered
	QStandardItemModel* model = qobject_cast<QStandardItemModel*>(uniformComBox_->model());
	for (int i = 0; i < model->rowCount(); ++i) {
		QStandardItem* item = model->item(i);
		item->setTextAlignment(Qt::AlignCenter);
		item->setSizeHint({0, 25});
	}
	uniformComBox_->setCurrentText("add");
}

void PropertySubtreeView::updateError() {
	if (layout_.itemAtPosition(0, 0) && layout_.itemAtPosition(0, 0)->widget()) {
		auto* widget = layout_.itemAtPosition(0, 0)->widget();
		layout_.removeWidget(widget);
		widget->hide();
		widget->deleteLater();
	}
	if (item_->hasError()) {
		auto errorItem = item_->error();
		if (errorItem.category() == core::ErrorCategory::RAMSES_LOGIC_RUNTIME_ERROR || errorItem.category() == core::ErrorCategory::PARSE_ERROR || errorItem.category() == core::ErrorCategory::GENERAL || errorItem.category() == core::ErrorCategory::MIGRATION_ERROR) {
			layout_.addWidget(new ErrorBox(errorItem.message().c_str(), errorItem.level(), this), 0, 0);
			// It is unclear why this is needed - but without it, the error box does not appear immediately when an incompatible render buffer is assigned to a render target and the scene error view is in the background.
			// The error box does appear later, e. g. when the mouse cursor is moved over the preview or when the left mouse button is clicked in a different widget.
			update();
		}
	}
}

void PropertySubtreeView::slotTreeMenu(const QPoint& pos) {
	QMenu menu;
	if (item_->valueHandle().isProperty()) {
		std::string property = item_->valueHandle().getPropertyPath();
		QStringList strList = QString::fromStdString(property).split(".");
		// Determine whether it is a System/Custom Property and whether it is a Float type
		if (!isValidValueHandle(strList, item_->valueHandle())) {
			return;
		}
		menu.addAction(insertKeyFrameAction_);
		menu.addAction(copyProperty_);
		menu.exec(QCursor::pos());
	}
}

QString PropertySubtreeView::setCurveName(QString name) {
	QStringList list = name.split(".");
	QString resultName = list[0];
	if (list[list.size() - 1] == "x" || list[list.size() - 1] == "y" || list[list.size() - 1] == "z") {
		resultName += "." + list[list.size() - 2];
	}
	resultName += "." + list[list.size() - 1];
	return resultName;
}

QString PropertySubtreeView::setPropertyName(QString name) {
	QStringList list = name.split(".");
	QString resultName = "";
	// Set rotation, scaling, displacement attribute parameters
	if (list.size() > 1 && list.contains("translation") || list.contains("rotation") || list.contains("scale")) {
		resultName = list[list.size() - 2] + "." + list[list.size() - 1];
		return resultName;
	} else if (list.size() > 2 && list.contains("uniforms")) {
		int index = list.indexOf("uniforms");
		for (int i = index; i < list.size(); ++i) {
			resultName += list[i];
			resultName += ".";
		}
		resultName = resultName.left(resultName.length() - 1);
		return resultName;
	} else {
		QMessageBox warningBox(QMessageBox::Warning, "Warning", "This property is not available as an animated property.", QMessageBox::Yes);
	}
	return resultName;
}

void PropertySubtreeView::slotInsertKeyFrame() {
	raco::core::ValueHandle valueHandle = item_->valueHandle();
	bool isResource = valueHandle.rootObject().get()->getTypeDescription().isResource;
	if (isResource) {
		return;
	}
	if (valueHandle.isProperty()) {
		QString propertyPath = QString::fromStdString(valueHandle.getPropertyPath());
		if (valueHandle.parent() != NULL) {
			raco::core::ValueHandle parentHandle = valueHandle.parent();

			QString property;
			property = QString::fromStdString(parentHandle.getPropName()) + "." + QString::fromStdString(valueHandle.getPropName());
			if (QString::fromStdString(valueHandle.getPropertyPath()).contains("material")) {
				property = QString::fromStdString(valueHandle.getPropertyPath());
				property = property.section(".", 1);
			}
			property = setPropertyName(property);
			if (property == "") {
				return;
			}
			// is have active animation
			std::string sampleProperty = animationDataManager::GetInstance().GetActiveAnimation();
			if (sampleProperty == std::string()) {
				return;
			}

			propertyPath = setCurveName(propertyPath);
			QString curve = QString::fromStdString(sampleProperty) + "." + propertyPath;

			double value{0};
			if (valueHandle.type() == raco::core::PrimitiveType::Double) {
				value = valueHandle.asDouble();
			}
			std::map<std::string, std::string> bindingMap;
			NodeDataManager::GetInstance().getActiveNode()->NodeExtendRef().curveBindingRef().getPropCurve(sampleProperty, bindingMap);

			auto it = bindingMap.find(property.toStdString());
			if (it != bindingMap.end()) {
				Q_EMIT item_->model()->sigCreateCurve(property, QString::fromStdString(it->second), value);
				return;
			}
			Q_EMIT item_->model()->sigCreateCurveAndBinding(property, curve, value);
		}
	}
}

void PropertySubtreeView::slotCopyProperty() {
	raco::core::ValueHandle valueHandle = item_->valueHandle();
	bool isResource = valueHandle.rootObject().get()->getTypeDescription().isResource;
	if (isResource) {
		return;
	}
	if (valueHandle.isProperty()) {
		QString property = QString::fromStdString(valueHandle.getPropertyPath());
		if (property.contains("material")) {
			property = property.section(".", 1);
			QClipboard* clip = QApplication::clipboard();
			clip->setText(property);
			return;
		}
		std::string str = valueHandle.getPropName();
		if (valueHandle.parent() != NULL) {
			valueHandle = valueHandle.parent();
			QString property = QString::fromStdString(valueHandle.getPropName()) + "." + QString::fromStdString(str);
			QClipboard* clip = QApplication::clipboard();
			clip->setText(property);
		}
	}
}

void PropertySubtreeView::updateUniformCombox() {
	if (uniformComBox_!= nullptr) {
		uniformComBox_->clear();
		auto uV = item_->valueHandle();
		auto material = uV.parent().get("material").asRef();
		if (!material) {
			uniformComBox_->setCurrentText("add");
			return;
		}
		auto uniformTabe = material->get("uniforms")->asTable();
		for (int i = 0; i < uniformTabe.size(); i++) {
			NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
			bool noShowed = false;
			if (pNode->hasUniform(uniformTabe.name(i))) {
				noShowed = true;
			}
			if (!noShowed && uniformTabe.name(i) != PTW_FRAG_SYS_OPACITY) {
				uniformComBox_->addItem(QString::fromStdString(uniformTabe.name(i)));
			}
		}
		uniformComBox_->setCurrentText("add");
	}
}

void PropertySubtreeView::delUniformButtonClicked() {
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	for (auto checkUniformName : checkUniformName_) {
		if (pNode->hasUniform(checkUniformName)) {
			pNode->deleteUniformData(checkUniformName);
			updateUniformCombox();
			auto privateItem = item_->siblingItem("private");
			privateItem->set(false);
			privateItem->set(true);
		}
	}
}

void PropertySubtreeView::slotUniformNameChanged(QString s) {
	NodeData* pNode = NodeDataManager::GetInstance().getActiveNode();
	auto root = NodeDataManager::GetInstance().root();
	if (s == "add") {
		return;
	}
	if (!pNode->hasUniform(s.toStdString())) {
		auto materialOject = item_->siblingItem("material")->valueHandle().asRef();
		core::ValueHandle uniformsHandle = {materialOject, &user_types::Material::uniforms_};
		Uniform un;
		
		for (int i{0}; i < uniformsHandle.size(); i++) {
			if (s.toStdString() == uniformsHandle[i].getPropName()) {
				setUniformsProperty(uniformsHandle[i], un);
			}
		}
		pNode->insertUniformData(un);
	}
	updateUniformCombox();
	auto privateItem = item_->siblingItem("private");
	privateItem->set(false);
	privateItem->set(true);
}
void PropertySubtreeView::setUniformsProperty(core::ValueHandle valueHandle, Uniform& tempUniform) {
	using PrimitiveType = core::PrimitiveType;
	tempUniform.setName(valueHandle.getPropName());
	std::string property = valueHandle.getPropName();
	switch (valueHandle.type()) {
			case PrimitiveType::String: {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::String);
				tempUniform.setValue(valueHandle.asString());
				break;
			}
			case PrimitiveType::Bool: {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Bool);
				tempUniform.setValue(valueHandle.asBool());
				break;
			}
			case PrimitiveType::Int: {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Int);
				tempUniform.setValue(valueHandle.asInt());
				break;
			}
			case PrimitiveType::Double: {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Double);
				tempUniform.setValue(valueHandle.asDouble());
				break;
			}
			case PrimitiveType::Ref: {
				tempUniform.setName(property);
				tempUniform.setType(UniformType::Ref);
				if (valueHandle.asRef())
					tempUniform.setValue(valueHandle.asRef()->objectName());
				//TextureData textureData;
				//textureData.setUniformName(property);
				//setTexturePorperty(tempHandle.asRef(), materialData, textureData);

				//if (textureData.getName().empty()) {
				//	textureData.setName("empty");
				//	textureData.setBitmapRef("empty");
				//}
				//materialData.addTexture(textureData);
				break;
			}
			case PrimitiveType::Table:
			case PrimitiveType::Struct: {
				auto typeDesc = &valueHandle.constValueRef()->asStruct().getTypeDescription();
				if (typeDesc == &core::Vec2f::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec2f);
					Vec2 value;
					value.x = valueHandle[0].asDouble();
					value.y = valueHandle[1].asDouble();
					tempUniform.setValue(value);
				} else if (typeDesc == &core::Vec3f::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec3f);
					Vec3 value;
					value.x = valueHandle[0].asDouble();
					value.y = valueHandle[1].asDouble();
					value.z = valueHandle[2].asDouble();
					tempUniform.setValue(value);
				} else if (typeDesc == &core::Vec4f::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec4f);
					Vec4 value;
					value.x = valueHandle[0].asDouble();
					value.y = valueHandle[1].asDouble();
					value.z = valueHandle[2].asDouble();
					value.w = valueHandle[3].asDouble();
					tempUniform.setValue(value);
				} else if (typeDesc == &core::Vec2i::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec4i);
					Vec2int value;
					value.x = valueHandle[0].asInt();
					value.y = valueHandle[1].asInt();
					tempUniform.setValue(value);
				} else if (typeDesc == &core::Vec3i::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec3i);
					Vec3int value;
					value.x = valueHandle[0].asInt();
					value.y = valueHandle[1].asInt();
					value.z = valueHandle[2].asInt();
					tempUniform.setValue(value);
				} else if (typeDesc == &core::Vec4i::typeDescription) {
					tempUniform.setName(property);
					tempUniform.setType(UniformType::Vec4i);
					Vec4int value;
					value.x = valueHandle[0].asInt();
					value.y = valueHandle[1].asInt();
					value.z = valueHandle[2].asInt();
					value.w = valueHandle[3].asInt();
					tempUniform.setValue(value);
				}
				break;
			}
			default: {
				break;
			}
		};
}
bool PropertySubtreeView::isValidValueHandle(QStringList list, core::ValueHandle handle) {
	// lamada
	auto func = [&](QStringList tempList, raco::core::ValueHandle tempHandle) -> bool {
		if (tempHandle.isObject()) {
			for (int i{1}; i < list.size(); i++) {
				QString str = list[i];
				tempHandle = tempHandle.get(str.toStdString());
				if (!tempHandle.isProperty()) {
					return false;
				}
			}
		}
		return true;
	};

	if (list.contains("translation") || list.contains("rotation") || list.contains("scale")) {
		if (list.contains("x") || list.contains("y") || list.contains("z")) {
			return true;
		}
	} else if (list.contains("uniforms")) {
		if (list.contains("x") || list.contains("y") || list.contains("z")) {
			return true;
		} else {
			if (func(list, handle)) {
				if (!handle.hasProperty("x") && !handle.hasProperty("y") && !handle.hasProperty("z")) {
					return true;
				}
			}
		}
	}
	return false;
}

void PropertySubtreeView::collectTabWidgets(QObject* item, QWidgetList& tabWidgets) {
	if (auto itemWidget = qobject_cast<QWidget*>(item)) {
		if (itemWidget->focusPolicy() & Qt::TabFocus) {
			tabWidgets.push_back(itemWidget);
		}
	}
	for (auto child : item->children()) {
		collectTabWidgets(child, tabWidgets);
	}
}

void PropertySubtreeView::recalculateTabOrder() {
	QWidgetList tabWidgets;
	collectTabWidgets(this, tabWidgets);

	auto lastWidget = previousInFocusChain();
	for (auto widget : tabWidgets) {
		setTabOrder(lastWidget, widget);
		lastWidget = widget;
	}
}

void PropertySubtreeView::updateChildrenContainer() {
	if (item_->showChildren() && !childrenContainer_) {
		if (!item_->valueHandle().isObject() && item_->type() == core::PrimitiveType::Ref) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			childrenContainer_->addWidget(new EmbeddedPropertyBrowserView{item_, this});
			layout_.addWidget(childrenContainer_, 2, 0);
		} else if (item_->valueHandle().isObject() || hasTypeSubstructure(item_->type())) {
			childrenContainer_ = new PropertySubtreeChildrenContainer{item_, this};
			// match is in nodeData by uniform
			for (const auto& child : item_->children()) {
				auto* subtree = new PropertySubtreeView{model_, child, childrenContainer_};
				childrenContainer_->addWidget(subtree);
			}

			QObject::connect(item_, &PropertyBrowserItem::childrenChanged, childrenContainer_, [this](const QList<PropertyBrowserItem*> items) {
				Q_EMIT model_->beforeStructuralChange(this);
				for (auto& childWidget : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
					Q_EMIT model_->beforeRemoveWidget(childWidget);
					childrenContainer_->removeWidget(childWidget);
					childWidget->deleteLater();
				}
				for (auto& child : items) {
					auto* subtree = new PropertySubtreeView{model_, child, childrenContainer_};
					childrenContainer_->addWidget(subtree);
				}
				recalculateTabOrder();
			});
			recalculateLabelWidth();
			layout_.addWidget(childrenContainer_, 2, 0);
		}
	} else if (!item_->showChildren() && childrenContainer_) {
		delete childrenContainer_;
		childrenContainer_ = nullptr;
	}

	recalculateTabOrder();
}

void PropertySubtreeView::mousePressEvent(QMouseEvent* event) {
	if (!isUniform_)
		return;
	if (event->button() == Qt::LeftButton) {
		if (!isChecked_) {
			QPalette pal;
			pal.setColor(QPalette::Window, QColor(206, 143, 26, 200));
			labelContainer_->setAutoFillBackground(true);
			labelContainer_->setPalette(pal);
		} else {
			labelContainer_->setAutoFillBackground(true);
			labelContainer_->setPalette(palette_);
		}
		isChecked_ = !isChecked_;
	}
	if (isChecked_) {
		QLabel* label = dynamic_cast<QLabel*>(labelContainer_->layout()->itemAt(1)->widget());
		checkUniformName_.insert(label->text().toStdString());
	} else {
		QLabel* label = dynamic_cast<QLabel*>(labelContainer_->layout()->itemAt(1)->widget());
		checkUniformName_.erase(label->text().toStdString());
	}
}

void PropertySubtreeView::setLabelAreaWidth(int width) {
	if (labelWidth_ != width) {
		labelWidth_ = width;
		recalculateLabelWidth();
	}
}

void PropertySubtreeView::recalculateLabelWidth() {
	if (childrenContainer_ != nullptr) {
		childrenContainer_->setOffset(decorationWidget_->width());
	}
	const int labelWidthHint = std::max(labelWidth_, getLabelAreaWidthHint() - decorationWidget_->width());
	if (label_->width() != labelWidthHint) {
		label_->setFixedWidth(labelWidthHint);
	}

	for (const auto& child : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
		child->setLabelAreaWidth(labelWidthHint - decorationWidget_->width());
	}
}

int PropertySubtreeView::getLabelAreaWidthHint() const {
	int labelWidthHint = label_->fontMetrics().boundingRect(label_->text()).width();
	if (childrenContainer_ != nullptr && childrenContainer_->size().height() > 0) {
		for (const auto& child : childrenContainer_->findChildren<PropertySubtreeView*>(QString{}, Qt::FindDirectChildrenOnly)) {
			const auto childLabelWidthHint = child->getLabelAreaWidthHint();
			if (labelWidthHint < childLabelWidthHint) {
				labelWidthHint = childLabelWidthHint;
			}
		}
	}
	return labelWidthHint + decorationWidget_->width();
}

void PropertySubtreeView::paintEvent(QPaintEvent* event) {
	drawHighlight(this, highlight_);
	recalculateLabelWidth();
	QWidget::paintEvent(event);
}

void PropertySubtreeView::playStructureChangeAnimation() {
	if (!item_->hasCollapsedParent()) {
		if (visibleRegion().isEmpty()) {
			Q_EMIT model_->addNotVisible(this);
		} else {
			auto* animation = new QPropertyAnimation(this, "highlight");
			animation->setDuration(250);
			animation->setStartValue(0.0);
			animation->setEndValue(1.0);
			QObject::connect(animation, &QPropertyAnimation::destroyed, this, [this]() {
				highlight_ = 0.0;
				update();
			});
			animation->start(QPropertyAnimation::DeleteWhenStopped);
		}
	} else {
		Q_EMIT model_->addNotVisible(this);
	}
}

}  // namespace raco::property_browser
