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

TVariant* AssetsFunction::VariantNumericInt(int Num) {
	TVariant* variant = new TVariant;
	TNumericValue* numeric = new TNumericValue;
	numeric->set_int_(Num);
	variant->set_allocated_numeric(numeric);
	return variant;
}

TVariant* AssetsFunction::VariantNumericUInt(unsigned int Num) {
	TVariant* variant = new TVariant;
	TNumericValue* numeric = new TNumericValue;
	numeric->set_uint(Num);
	variant->set_allocated_numeric(numeric);
	return variant;
}

TVariant* AssetsFunction::VariantAsciiString(std::string str) {
	TVariant* variant = new TVariant;
	variant->set_asciistring(str);
	return variant;
}

TVariant* AssetsFunction::VariantResourceId(std::string str) {
	TVariant* variant = new TVariant;
	variant->set_resourceid(str);
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

TDataProvider* AssetsFunction::ProviderNumericInt(int num) {
	TDataProvider* provide = new TDataProvider;
	provide->set_allocated_variant(VariantNumericInt(num));
	return provide;
}

TDataProvider* AssetsFunction::ProviderNumericUInt(unsigned int num) {
	TDataProvider* provide = new TDataProvider;
	provide->set_allocated_variant(VariantNumericUInt(num));
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

TDataBinding* AssetsFunction::BindingTypeMix(TDataBinding& Operand1, TDataBinding& Operand2, TDataBinding& Operand3, TEDataType type1, TEDataType type2, TEDataType type3) {
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;

	operation->set_operator_(TEOperatorType_Mix);
	operation->add_datatype(type1);
	operation->add_datatype(type2);
	operation->add_datatype(type3);
	auto it = operation->add_operand();
	*it = Operand1;
	it = operation->add_operand();
	*it = Operand2;
	it = operation->add_operand();
	*it = Operand3;

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

void AssetsFunction::ColorIPAIconExternal(HmiWidget::TExternalModelParameter* external, std::string str) {
	TDataBinding* binding = new TDataBinding;
	DataBindingKeyProvider(binding, str, TEProviderSource_ColorRegistry);
	external->set_allocated_key(Key(str));
	external->set_allocated_binding(binding);
}

void AssetsFunction::ColorIPAIconInternal(HmiWidget::TInternalModelParameter* internal, std::string reStr, std::string ExStr) {
	internal->set_allocated_key(Key(reStr));
	TDataBinding Operand;
	OperandKeySrc(Operand, ExStr, TEProviderSource_ExtModelValue);
	BindingTypeConvert(Operand, TEDataType_Vec4, TEDataType_Color);
	internal->set_allocated_binding(BindingTypeConvert(Operand, TEDataType_Vec4, TEDataType_Color));
}
void AssetsFunction::ColorModeMixInternal(HmiWidget::TInternalModelParameter* internal , std::string colorMode, std::string IPAIconV4, std::string HUD_IPAIconV4, std::string HUD) {
	internal->set_allocated_key(Key(colorMode));
	TDataBinding Operand1;
	OperandKeySrc(Operand1, IPAIconV4, TEProviderSource_IntModelValue);
	TDataBinding Operand2;
	OperandKeySrc(Operand2, HUD_IPAIconV4, TEProviderSource_IntModelValue);
	TDataBinding Operand3;
	OperandKeySrc(Operand3, HUD, TEProviderSource_ExtModelValue);
	internal->set_allocated_binding(BindingTypeMix(Operand1, Operand2, Operand3, TEDataType_Vec4, TEDataType_Vec4, TEDataType_Float));
}

// comparison operation
// resultKey(bool) = Operand1 op Operand2   (op >  <  ==)
void AssetsFunction::CompareOperation(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TDataBinding& Operand1, TEDataType type1, TDataBinding& Operand2, TEDataType type2, TEOperatorType op) {
	internal->set_allocated_key(Key(resultKey));
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;

	operation->set_operator_(op);
	operation->add_datatype(type1);
	operation->add_datatype(type2);

	auto operandIt1 = operation->add_operand();
	*operandIt1 = Operand1;

	auto operandIt2 = operation->add_operand();
	*operandIt2 = Operand2;
	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	internal->set_allocated_binding(binding);
}

// IfThenElse
void AssetsFunction::IfThenElse(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TDataBinding& Operand1, TEDataType type1, TDataBinding& Operand2, TEDataType type2, TDataBinding& Operand3, TEDataType type3) {
	internal->set_allocated_key(Key(resultKey));

	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;

	operation->set_operator_(TEOperatorType_IfThenElse);
	operation->add_datatype(type1);
	operation->add_datatype(type2);
	operation->add_datatype(type3);

	auto operandIt1 = operation->add_operand();
	*operandIt1 = Operand1;
	auto operandIt2 = operation->add_operand();
	*operandIt2 = Operand2;
	auto operandIt3 = operation->add_operand();
	*operandIt3 = Operand3;

	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	internal->set_allocated_binding(binding);
}

// op Operands
void AssetsFunction::operatorOperands(HmiWidget::TInternalModelParameter* internal, std::string resultKey, TEDataType type, std::vector<TDataBinding>& Operands, TEOperatorType op) {
	internal->set_allocated_key(Key(resultKey));
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	operation->set_operator_(op);

	if (Operands.size() <= 4) {
		for (int i = 0; i < Operands.size() && i < 4; ++i) {
			operation->add_datatype(type);
			auto operandIt = operation->add_operand();
			*operandIt = Operands[i];
		}
	} else {
		int i = 0;
		int n = Operands.size() / 4;
		for (int a = 0; a < n; a++) {
			for (i = 0; i < 3 ; ++i) {
				operation->add_datatype(type);
				auto operandIt = operation->add_operand();
				*operandIt = Operands[i];
			}
			operation->add_datatype(type);
			TDataBinding* OperandNew = new TDataBinding;
			TDataProvider* providerNew = new TDataProvider;
			TOperation* operationNew = new TOperation;
			operationNew->set_operator_(TEOperatorType_Add);
			int last = Operands.size() - (a + 1) * 3;
			if (last <= 4) {
				for (int j = 0; j < last; ++j, i++) {
					operationNew->add_datatype(type);
					auto operandItNew = operationNew->add_operand();
					*operandItNew = Operands[i];
				}
				providerNew->set_allocated_operation(operationNew);
				OperandNew->set_allocated_provider(providerNew);
			}
			auto operandIt = operation->add_operand();
			*operandIt = *OperandNew;
		}
	}

	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	internal->set_allocated_binding(binding);
}

// switch animation
void AssetsFunction::SwitchAnimation(HmiWidget::TInternalModelParameter* internal, struct PTWSwitch& data) {
	internal->set_allocated_key(Key(data.outPutKey));
	TDataBinding* binding = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	// condition
	operation->set_operator_(TEOperatorType_Switch);
	operation->add_datatype(data.dataType1);
	auto operandIt = operation->add_operand();
	*operandIt = data.Operands[0];
	// case
	for (int i = 1; i < data.Operands.size() - 1; ++i) {
		operation->add_datatype(data.dataType1);
		auto operandIt = operation->add_operand();
		*operandIt = data.Operands[i];
		i++;
		operation->add_datatype(data.dataType2);
		operandIt = operation->add_operand();
		*operandIt = data.Operands[i];
	}
	// default
	operation->add_datatype(data.dataType2);
	operandIt = operation->add_operand();
	*operandIt = data.Operands[data.Operands.size() - 1];

	provider->set_allocated_operation(operation);
	binding->set_allocated_provider(provider);
	internal->set_allocated_binding(binding);
}

// trigger
void AssetsFunction::addCompositeAnimation(HmiWidget::TWidget* widget) {
	// compositeAnimation
	auto compositeAnimation = widget->add_compositeanimation();
	TIdentifier* compositeidentifier = new TIdentifier;
	compositeidentifier->set_valuestring("compositeAnimation");
	compositeAnimation->set_allocated_compositeidentifier(compositeidentifier);

	// returnValue
	// returnValue.key
	auto returnValue = compositeAnimation->add_returnvalue();
	TIdentifier* key = new TIdentifier;
	key->set_valuestring("compositeAnimation.output");
	returnValue->set_allocated_key(key);

	// returnValue.animation
	auto animation = returnValue->add_animation();
	// returnValue.animation.Identifier
	auto anIdentifier = new TIdentifier;
	anIdentifier->set_valuestring("animation");
	animation->set_allocated_identifier(anIdentifier);

	// returnValue.animation.WidgetAnimation
	auto widgetAnimation = new HmiWidget::TWidgetAnimation;
	auto startValue = new TNumericValue;
	startValue->set_float_(0.0);
	widgetAnimation->set_allocated_startvalue(startValue);
	auto endValue = new TNumericValue;
	endValue->set_float_(1.0);
	widgetAnimation->set_allocated_endvalue(endValue);
	// durationbinding
	TDataBinding* durationBinding = new TDataBinding;
	durationBinding->set_allocated_key(Key("DurationValue"));
	durationBinding->set_allocated_provider(ProviderSrc(TEProviderSource_IntModelValue));
	widgetAnimation->set_allocated_durationbinding(durationBinding);
	widgetAnimation->set_interpolator(TEAnimationInterpolator::TEAnimationInterpolator_Linear);
	widgetAnimation->set_returntype(TEDataType::TEDataType_Float);
	widgetAnimation->set_loopcount(0);
	widgetAnimation->set_updateinterval(33);
	animation->set_allocated_widgetanimation(widgetAnimation);

	// returnValue.animation.trigger
	auto triggerIter = animation->add_trigger();
	triggerIter->set_action(HmiWidget::TEAnimationSlot::TEAnimationSlot_SlotAnimationStart);
	returnValue->set_returntype(TEDataType::TEDataType_Float);
}

void AssetsFunction::addTrigger(HmiWidget::TWidget* widget) {
	auto trigger = widget->add_trigger();
	TIdentifier* triggeridentifier = new TIdentifier;
	triggeridentifier->set_valuestring("StartStopCompositeAnimation");
	trigger->set_allocated_identifier(triggeridentifier);

	auto conditionalTrigger = new HmiWidget::TConditionalTrigger;
	auto condition = new TDataBinding;
	auto key = new TIdentifier;
	key->set_valuestring("Play");
	condition->set_allocated_key(key);

	TDataProvider* provider = new TDataProvider;
	provider->set_source(TEProviderSource_ExtModelValue);
	condition->set_allocated_provider(provider);

	conditionalTrigger->set_allocated_condition(condition);

	auto commond = new HmiWidget::TCommand;
	auto antrigger = new HmiWidget::TAnimationTrigger;
	auto animation = new TIdentifier;
	animation->set_valuestring("compositeAnimation");
	antrigger->set_allocated_animation(animation);
	antrigger->set_action(HmiWidget::TEAnimationAction::TEAnimationAction_Start);
	commond->set_allocated_animationtrigger(antrigger);
	conditionalTrigger->set_allocated_command(commond);

	auto elseCommond = new HmiWidget::TCommand;
	auto antriggerElse = new HmiWidget::TAnimationTrigger;
	auto animationElse = new TIdentifier;
	animationElse->set_valuestring("compositeAnimation");
	antriggerElse->set_allocated_animation(animationElse);
	antriggerElse->set_action(HmiWidget::TEAnimationAction::TEAnimationAction_Stop);
	elseCommond->set_allocated_animationtrigger(antriggerElse);
	conditionalTrigger->set_allocated_elsecommand(elseCommond);

	trigger->set_allocated_conditionaltrigger(conditionalTrigger);
}

void AssetsFunction::addPlayDomain(HmiWidget::TWidget* widget) {
	auto play = widget->add_externalmodelvalue();
	TIdentifier* playKey = new TIdentifier;
	playKey->set_valuestring("Play");
	play->set_allocated_key(playKey);
	TVariant* variant = new TVariant;
	variant->set_bool_(true);
	play->set_allocated_variant(variant);

	auto domain = widget->add_internalmodelvalue();
	TIdentifier* domainKey = new TIdentifier;
	domainKey->set_valuestring("domain");
	domain->set_allocated_key(domainKey);

	auto binding = new TDataBinding;
	auto bindKey = new TIdentifier;
	bindKey->set_valuestring("compositeAnimation.output");
	binding->set_allocated_key(bindKey);
	TDataProvider* provider = new TDataProvider;
	provider->MutableExtension(HmiWidget::animation)->set_valuestring("compositeAnimation");
	binding->set_allocated_provider(provider);
	domain->set_allocated_binding(binding);
}

// externalModelValue{key , variant}
void AssetsFunction::externalKeyVariant(HmiWidget::TExternalModelParameter* external, std::string keyStr, TVariant* var) {
	external->set_allocated_key(Key(keyStr));
	external->set_allocated_variant(var);
}

}  // namespace raco::dataConvert
