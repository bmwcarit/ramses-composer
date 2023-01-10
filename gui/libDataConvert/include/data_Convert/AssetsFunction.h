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

struct PTWSwitch {
	std::string outPutKey;
	TEDataType dataType1;
	TEDataType dataType2;
	std::vector<TDataBinding> Operands; // n + 2
};

class AssetsFunction  {
public:
	// key  
	TIdentifier* Key(const std::string valueStr);
	// Variant Num Float
	TVariant* VariantNumeric(float Num);
	// Variant Num Int
	TVariant* VariantNumericInt(int Num);
	// Variant Num UInt
	TVariant* VariantNumericUInt(unsigned int Num);
	// Variant AsciiString
	TVariant* VariantAsciiString(std::string str);
	// Variant ResourceId
	TVariant* VariantResourceId(std::string str);
	TVariant* VariantIdenAndType(std::string str, TEIdentifierType Idtype);
	// provider:src 
	TDataProvider* ProviderSrc(TEProviderSource value);
	// provider:numeric  Float
	TDataProvider* ProviderNumeric(float num);
	// provider:numeric  Int
	TDataProvider* ProviderNumericInt(int num);
	// provider:numeric  UInt
	TDataProvider* ProviderNumericUInt(unsigned int num);
	// provider:asciiString
	TDataProvider* ProviderAsciiString(std::string AsciiStr);
	// transform operand:Curve
	TDataProvider* ProviderCurve(std::string curveName);
	// Operand{key , curveRef}
	void OperandKeyCurveRef(TDataBinding& Operand, std::string curveName);
	// Operand{curveRef}
	void OperandCurveRef(TDataBinding& Operand, std::string curveName);
	// Operand{Float}
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
	// convert <typ2-->type1> (Operand)
	TDataBinding* BindingTypeConvert(TDataBinding& Operand, TEDataType type1, TEDataType type2);
	// Mix (Operand1, Operand2, Operand3)
	TDataBinding* BindingTypeMix(TDataBinding& Operand1, TDataBinding& Operand2, TDataBinding& Operand3, TEDataType type1, TEDataType type2, TEDataType type3);

	void TransformCreateScale(HmiWidget::TNodeTransform* transform, TDataBinding& operandX, TDataBinding& operandY, TDataBinding& operandZ);

	void ResourceParamAddIdentifier(HmiWidget::TResourceParam* resParam, std::string nodeName);
	void ResourceParamAddResource(HmiWidget::TResourceParam* resParam, std::string resName);
	// HmiScenegraph::TUniform is different from HmiWidget TUniform
	void CreateHmiWidgetUniform(HmiWidget::TUniform* uniform, std::string name, std::string value, TEProviderSource src);

	void AddUniform2Appearance(HmiWidget::TAppearanceParam* appear,std::string name, std::string value, TEProviderSource src);

	void ColorIPAIconExternal(HmiWidget::TExternalModelParameter* external, std::string str);
	void ColorIPAIconInternal(HmiWidget::TInternalModelParameter* internal, std::string reStr, std::string ExStr);
	void ColorModeMixInternal(HmiWidget::TInternalModelParameter* internal, std::string colorMode, std::string IPAIconV4, std::string HUB_IPAIconV4, std::string HUB);

	void CompareOperation(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TDataBinding& Operand1, TEDataType type1, TDataBinding& Operand2, TEDataType type2, TEOperatorType op);
	void IfThenElse(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TDataBinding& Operand1, TEDataType type1, TDataBinding& Operand2, TEDataType type2, TDataBinding& Operand3, TEDataType type3);


	void operatorOperands(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TEDataType type, std::vector<TDataBinding>& Operands,TEOperatorType op);

	void SwitchAnimation(HmiWidget::TInternalModelParameter* internal, struct PTWSwitch& data);


	void addCompositeAnimation(HmiWidget::TWidget* widget, unsigned int time);

	void addCompositeAnimationDurationBinding(HmiWidget::TWidget* widget);

	void addTrigger(HmiWidget::TWidget* widget);
	void addPlayDomain(HmiWidget::TWidget* widget);


	void externalKeyVariant(HmiWidget::TExternalModelParameter* external, std::string keyStr, TVariant* var);

private:

};

}
#endif // ASSETS_FUNCTION_H
