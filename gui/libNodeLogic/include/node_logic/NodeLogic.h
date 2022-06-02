#ifndef NODEDATAPRO_H
#define NODEDATAPRO_H

#include <QObject>
#include <set>

#include "core/CommandInterface.h"
#include "property_browser/PropertyBrowserItem.h"
#include "NodeData/nodeManager.h"
#include "CurveData/CurveManager.h"
#include "signal/SignalProxy.h"
#include <QDebug>

using namespace raco::signal;
using namespace raco::guiData;

namespace raco::node_logic {
class PropertyBrowserItem;

class NodeLogic : public QObject {
	Q_OBJECT
public:
    explicit NodeLogic(raco::core::CommandInterface* commandInterface,
                       QObject *parent = nullptr);

    void setCommandInterface(raco::core::CommandInterface* commandInterface);
	void AnalyzeHandle();

	void initBasicProperty(raco::core::ValueHandle valueHandle, NodeData *pNode);
	void Analyzing(NodeData *pNode);

	void setUniformProperty(raco::core::ValueHandle valueHandle, NodeData *PropertyTree, bool bVec = false);

	void setMaterial(raco::core::ValueHandle valueHandle, NodeData *material);
    bool getValueHanlde(std::string property, core::ValueHandle &valueHandle);

	void setProperty(core::ValueHandle handle, std::string property, float value) {
        if (getValueHanlde(property, handle) && commandInterface_) {
			commandInterface_->set(handle, value);
		}
	}

	std::map<std::string, core::ValueHandle > &getNodeNameHandleReMap() {
		return nodeObjectIDHandleReMap_;
	}

	void setNodeNameHandleReMap(std::map<std::string, core::ValueHandle> nodeNameHandleReMap) {
		nodeObjectIDHandleReMap_ = std::move(nodeNameHandleReMap);
    }

public Q_SLOTS:
    void slotUpdateActiveAnimation(QString animation);
    void slotUpdateKeyFrame(int keyFrame);

signals:
	void sig_getHandles_from_NodePro(std::set<core::ValueHandle>& handles);
private:

	std::map<std::string, core::ValueHandle> nodeObjectIDHandleReMap_;
	raco::core::CommandInterface *commandInterface_;
	QString curAnimation_;
};
}

#endif // NODEDATAPRO_H
