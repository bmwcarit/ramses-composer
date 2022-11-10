#ifndef ASSETS_FUNCTION_H
#define ASSETS_FUNCTION_H

#include <string>

#include "proto/Numeric.pb.h"
#include "proto/Common.pb.h"
#include "proto/Scenegraph.pb.h"
#include "proto/HmiWidget.pb.h"
#include "proto/HmiBase.pb.h"
#include <google/protobuf/text_format.h>


namespace raco::dataConvert {

struct Case {
	std::string Identifier;
	TEIdentifierType IdentifierType;
	std::string key;
	TEProviderSource src;
};

struct Condition {
	std::string key;
	TEProviderSource src;
};

struct PTWSwitchData {
	std::string outPutKey;
	TEDataType dataType;
	Condition condition;
	std::vector<Case> caseArr;
};

class AssetsFunction  {
public:
	// key  
	TIdentifier* Key(const std::string valueStr);
	// Variant Num
	TVariant* VariantNumeric(float Num);
	// Variant AsciiString
	TVariant* VariantAsciiString(std::string str);
	TVariant* VariantIdenAndType(std::string str, TEIdentifierType Idtype);
	// provider:src 
	TDataProvider* ProviderSrc(TEProviderSource value);
	// provider:numeric
	TDataProvider* ProviderNumeric(float num);
	// provider:asciiString
	TDataProvider* ProviderAsciiString(std::string AsciiStr);
	// transform operand:Curve
	TDataProvider* ProviderCurve(std::string curveName);

	void OperandCurve(TDataBinding& Operand, std::string curveName);
	void OperandNumeric(TDataBinding& Operand, float num);
	void OperandKeySrc(TDataBinding& Operand, std::string keyStr, TEProviderSource src);
	void OperandProVarIdentAndType(TDataBinding& Operand, std::string Identifier, TEIdentifierType Idtype);
	// nodeParamName
	void NodeParamAddIdentifier(HmiWidget::TNodeParam* node, std::string nodeName);
	// node Shape
	void NodeParamAddNode(HmiWidget::TNodeParam* node, std::string nodeShape);

	void DataBindingKeyProvider(TDataBinding* Operand, std::string keyStr, TEProviderSource src);

	// binding = float(ValueStr) op float(num)
	TDataBinding* BindingValueStrNumericOperatorType(std::string ValueStr, TEProviderSource src, float num, TEOperatorType op);
	// binding =  op (operand type1 -->  type2)
	TDataBinding* BindingTypeConvert(TDataBinding& Operand, TEDataType type1, TEDataType type2);

	void TransformCreateScale(HmiWidget::TNodeTransform* transform, TDataBinding& operandX, TDataBinding& operandY, TDataBinding& operandZ);

	void ResourceParamAddIdentifier(HmiWidget::TResourceParam* resParam, std::string nodeName);
	void ResourceParamAddResource(HmiWidget::TResourceParam* resParam, std::string resName);
	// HmiScenegraph::TUniform is different from HmiWidget TUniform
	void CreateHmiWidgetUniform(HmiWidget::TUniform* uniform, std::string name, std::string value, TEProviderSource src);

	void AddUniform2Appearance(HmiWidget::TAppearanceParam* appear,std::string name, std::string value, TEProviderSource src);

	void PTWSwitch(HmiWidget::TInternalModelParameter* internalModelValue, PTWSwitchData& switchData);
	void SwitchType2Operation(TOperation* operation, Condition condition);
	void SwitchCase2Operation(TOperation* operation, Case caseData, TEDataType type, bool isDefault = false);

	void ColorExternal(HmiWidget::TExternalModelParameter* external, std::string str);
	void ColorExternal(HmiWidget::TExternalModelParameter* external, std::string reStr, std::string ExStr);

private:

};

}
#endif // ASSETS_FUNCTION_H
