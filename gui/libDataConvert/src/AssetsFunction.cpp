#include "data_Convert/AssetsFunction.h"

namespace raco::dataConvert {

TIdentifier* AssetsFunction::Key(const std::string valueStr) {
	TIdentifier* key = new TIdentifier;
	key->set_valuestring(valueStr);
	return key;
}

TVariant* AssetsFunction::VariantNumeric(float Num) {
	TVariant* variant = new TVariant;
	TNumericValue* numeric = new TNumericValue;
	numeric->set_float_(Num);
	variant->set_allocated_numeric(numeric);
	return variant;
}

TVariant* AssetsFunction::VariantAsciiString(std::string str) {
	TVariant* variant = new TVariant;
	variant->set_asciistring(str);
	return variant;
}

TVariant* AssetsFunction::VariantIdenAndType(std::string str, TEIdentifierType Idtype) {
	TVariant* variant = new TVariant;
	TIdentifier* identifier = new TIdentifier;
	identifier->set_valuestring(str);
	variant->set_allocated_identifier(identifier);
	variant->set_identifiertype(Idtype);
	return variant;
}

TDataProvider* AssetsFunction::ProviderSrc(TEProviderSource value) {
	TDataProvider* provider = new TDataProvider;
	provider->set_source(value);
	return provider;
}

// curvereference
TDataProvider* AssetsFunction::ProviderCurve(std::string curveName) {
	TDataProvider* provide = new TDataProvider;
	TIdentifier* curveReference = new TIdentifier;
	curveReference->set_valuestring(curveName);
	provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	return provide;
}

TDataProvider* AssetsFunction::ProviderNumeric(float num) {
	TDataProvider* provide = new TDataProvider;
	provide->set_allocated_variant(VariantNumeric(num));
	return provide;
}

TDataProvider* AssetsFunction::ProviderAsciiString(std::string AsciiStr) {
	TDataProvider* provider = new TDataProvider;
	provider->set_allocated_variant(VariantAsciiString(AsciiStr));
	return provider;
}

void AssetsFunction::OperandCurve(TDataBinding& Operand, std::string curveName) {
	Operand.set_allocated_provider(ProviderCurve(curveName));
}

void AssetsFunction::OperandNumeric(TDataBinding& Operand, float num) {
	Operand.set_allocated_provider(ProviderNumeric(num));
}

void AssetsFunction::OperandKeySrc(TDataBinding& Operand, std::string keyStr, TEProviderSource src) {
	Operand.set_allocated_key(Key(keyStr));
	Operand.set_allocated_provider(ProviderSrc(src));
}

void AssetsFunction::OperandProVarIdentAndType(TDataBinding& Operand, std::string Identifier, TEIdentifierType Idtype) {
	TDataProvider* provider = new TDataProvider;
	provider->set_allocated_variant(VariantIdenAndType(Identifier, Idtype));
	Operand.set_allocated_provider(provider);
}

void AssetsFunction::DataBindingKeyProvider(TDataBinding* Operand, std::string keyStr, TEProviderSource src) {
	Operand->set_allocated_key(Key(keyStr));
	Operand->set_allocated_provider(ProviderSrc(src));
}

void AssetsFunction::NodeParamAddIdentifier(HmiWidget::TNodeParam* nodeParam, std::string nodeName) {
	TIdentifier* identifier = new TIdentifier;
	identifier->set_valuestring(nodeName);
	nodeParam->set_allocated_identifier(identifier);
}

void AssetsFunction::ResourceParamAddIdentifier(HmiWidget::TResourceParam* resParam, std::string Name)
{
	TIdentifier* identifier = new TIdentifier;
	identifier->set_valuestring(Name);
	resParam->set_allocated_identifier(identifier);
}

void AssetsFunction::ResourceParamAddResource(HmiWidget::TResourceParam* resParam, std::string resName) {
	TDataBinding* binding = new TDataBinding;
	binding->set_allocated_provider(ProviderAsciiString(resName));
	resParam->set_allocated_resource(binding);
}

void AssetsFunction::NodeParamAddNode(HmiWidget::TNodeParam* nodeParam, std::string nodeShape) {
	TDataBinding* node = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TVariant* variant = new TVariant;
	variant->set_asciistring(nodeShape);
	provider->set_allocated_variant(variant);
	node->set_allocated_provider(provider);
	nodeParam->set_allocated_node(node);
}

void AssetsFunction::TransformCreateScale(HmiWidget::TNodeTransform* transform, TDataBinding& operandX, TDataBinding& operandY, TDataBinding& operandZ) {
	TDataBinding* scale = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	operation->set_operator_(TEOperatorType_MuxVec3);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	// X
	auto operand = operation->add_operand();
	*operand = operandX;
	// Y
	operand = operation->add_operand();
	*operand = operandY;
	// Z
	operand = operation->add_operand();
	*operand = operandZ;

	provider->set_allocated_operation(operation);
	scale->set_allocated_provider(provider);
	transform->set_allocated_scale(scale);
}

// float(ValueStr) op float(num)
TDataBinding* AssetsFunction::BindingValueStrNumericOperatorType(std::string ValueStr, TEProviderSource src, float num, TEOperatorType op) {
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;

	operation->set_operator_(op);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);

	auto operand1 = operation->add_operand();
	operand1->set_allocated_key(Key(ValueStr));
	operand1->set_allocated_provider(ProviderSrc(src));

	auto operand2 = operation->add_operand();
	operand2->set_allocated_provider(ProviderNumeric(num));
	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	return binding;
}
// convert <typ2-->type1> (Operand)
TDataBinding* AssetsFunction::BindingTypeConvert(TDataBinding& Operand, TEDataType type1, TEDataType type2) {
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;

	operation->set_operator_(TEOperatorType_Convert);
	operation->add_datatype(type1);
	operation->add_datatype(type2);
	auto it = operation->add_operand();
	*it = Operand;

	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	return binding;
}

void AssetsFunction::CreateHmiWidgetUniform(HmiWidget::TUniform* uniform, std::string name, std::string value, TEProviderSource src) {
	TDataBinding* nameBinding = new TDataBinding;
	nameBinding->set_allocated_provider(ProviderAsciiString(name));
	uniform->set_allocated_name(nameBinding);
	TDataBinding* valueBinding = new TDataBinding;
	DataBindingKeyProvider(valueBinding, value, src);
	uniform->set_allocated_value(valueBinding);
}

void AssetsFunction::AddUniform2Appearance(HmiWidget::TAppearanceParam* appear, std::string name, std::string value, TEProviderSource src) {
	HmiWidget::TUniform uniform;
	// add OPACITY uniform
	CreateHmiWidgetUniform(&uniform, name, value, src);
	*(appear->add_uniform()) = uniform;
}

///////////////   switch case  start    ///////////
void AssetsFunction::SwitchType2Operation(TOperation* operation, Condition condition) {
	operation->set_operator_(TEOperatorType_Switch);
	operation->add_datatype(TEDataType_Identifier);
	auto operand = operation->add_operand();
	operand->set_allocated_key(Key(condition.key));
	operand->set_allocated_provider(ProviderSrc(condition.src));
}

void AssetsFunction::SwitchCase2Operation(TOperation* operation, Case caseData, TEDataType type,bool isDefault) {
	if (!isDefault) {
		operation->add_datatype(TEDataType_Identifier);
		operation->add_datatype(type);

		TDataBinding operand2;
		auto it = operation->add_operand();
		OperandProVarIdentAndType(operand2, caseData.Identifier, caseData.IdentifierType);
		*it = operand2;

		TDataBinding operand1;
		 it = operation->add_operand();
		OperandKeySrc(operand1, caseData.key, caseData.src);
		*it = operand1;
	} else {
		operation->add_datatype(type);

		TDataBinding operand1;
		auto it = operation->add_operand();
		OperandKeySrc(operand1, caseData.key, caseData.src);
		*it = operand1;
	}
}

void AssetsFunction::PTWSwitch(HmiWidget::TInternalModelParameter* internalModelValue, PTWSwitchData& switchData) {
	// add result key
	internalModelValue->set_allocated_key(Key(switchData.outPutKey));

	// add binding
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	// add switch condition to operation
	SwitchType2Operation(operation, switchData.condition);

	// add switch case to operation
	for (auto caseData : switchData.caseArr) {
		SwitchCase2Operation(operation, caseData, switchData.dataType);
	}
	SwitchCase2Operation(operation, switchData.caseArr.at(0), switchData.dataType, true);
	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	internalModelValue->set_allocated_binding(binding);
}
///////////////   switch case end   ///////////

void AssetsFunction::ColorExternal(HmiWidget::TExternalModelParameter* external, std::string str) {
	TDataBinding* binding = new TDataBinding;
	DataBindingKeyProvider(binding, str, TEProviderSource_ColorRegistry);
	external->set_allocated_key(Key(str));
	external->set_allocated_binding(binding);
}

void AssetsFunction::ColorExternal(HmiWidget::TExternalModelParameter* external, std::string reStr, std::string ExStr) {
	external->set_allocated_key(Key(reStr));
	TDataBinding Operand;
	OperandKeySrc(Operand, ExStr, TEProviderSource_ExtModelValue);
	BindingTypeConvert(Operand, TEDataType_Vec4, TEDataType_Color);
	external->set_allocated_binding(BindingTypeConvert(Operand, TEDataType_Vec4, TEDataType_Color));
}

}  // namespace raco::dataConvert
