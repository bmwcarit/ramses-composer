#include "data_Convert/InputAssets.h"
#include "data_Convert/ProgramDefine.h"
#include "PropertyData/PropertyType.h"
#include "style/Icons.h"

#include <QMessageBox>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <qdir.h>


namespace raco::dataConvert {
using namespace raco::style;


bool InputPtx::parseOneNode(const HmiScenegraph::TNode& tNode, NodeData& nodeData) {
	nodeData.setName(tNode.name());

	TVector3f tScale = tNode.scale();
	Vec3 scaleData{tScale.x(), tScale.y(), tScale.z()};
	nodeData.insertSystemData("scale", scaleData);

	TVector3f tRota = tNode.rotation();
	Vec3 rotaData{tRota.x(), tRota.y(), tRota.z()};
	nodeData.insertSystemData("rotation", rotaData);

	TVector3f tTran = tNode.translation();
	Vec3 tranData{tTran.x(), tTran.y(), tTran.z()};
	nodeData.insertSystemData("translation", tranData);

	
	return true;
}

bool InputPtx::readProgram2Ptx(std::string filePathStr) {
	std::string ptxPath = filePathStr + "/scene.ptx";
	QFile file(QString::fromStdString(ptxPath));
	if (!file.open(QIODevice::ReadOnly)) {
		return false;
	}
	QByteArray allArray = file.readAll();
	QString allStr = QString(allArray);
	std::string input = allStr.toStdString();
	HmiScenegraph::TScene scene;
	bool success = google::protobuf::TextFormat::ParseFromString(input, &scene);
	const HmiScenegraph::TNode& tRoot = scene.root();
	qDebug() << QString::fromStdString(tRoot.name());
	for (HmiScenegraph::TNode child : tRoot.child()) {
		qDebug() << QString::fromStdString(child.name());
		NodeData data;
		parseOneNode(child, data);
	}


	



	


	// tRoot
	// HmiScenegraph::TNode* tRoot = new HmiScenegraph::TNode();
	// scene.set_allocated_root(tRoot);

	// materiallibrary
	//HmiScenegraph::TMaterialLib* materialLibrary = new HmiScenegraph::TMaterialLib();
	//scene.set_allocated_materiallibrary(materialLibrary);




	return success;


	qDebug() << QString::fromStdString(filePathStr);
	return true;
}

}  // namespace raco::dataConvert
