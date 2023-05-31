/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "core/UserObjectFactoryInterface.h"
#include "core/ProxyTypes.h"

#include "core/Link.h"
#include "user_types/EngineTypeAnnotation.h"

namespace raco::serialization::proxy {

using namespace raco::data_storage;

using raco::core::DisplayNameAnnotation;
using raco::core::RangeAnnotation;

using raco::user_types::EngineTypeAnnotation;
using raco::core::LinkStartAnnotation;
using raco::core::LinkEndAnnotation;
using raco::core::URIAnnotation;
using raco::core::EnumerationAnnotation;
using raco::core::ArraySemanticAnnotation;
using raco::core::HiddenProperty;
using raco::core::TagContainerAnnotation;
using raco::core::UserTagContainerAnnotation;
using raco::core::ExpectEmptyReference;
using raco::core::RenderableTagContainerAnnotation;
using raco::core::FeatureLevel;
using raco::core::ReadOnlyAnnotation;

template <typename U, typename T>
struct tuple_has_type {};

template <typename U, typename... T>
struct tuple_has_type<U, std::tuple<T...>> : std::disjunction<std::is_same<U, T>...> {
};


class ProxyObjectFactory : public raco::core::UserObjectFactoryInterface {
public:
	// The ProxyObjectFactory creates all objects and properties that are deserialized.
	// These are then further used as input to the migration code.
	// After migration the proxy objects are converted to the real user types by the deserialization.
	// Those are created using the UserObjectFactory.
	//
	// Please note:
	// - The types used here are the proxy types from ProxyTypes.h and live in the namespace
	//   raco::serialization::proxy. They are therefore different types than the normally used types 
	//   even if the class names are identical.
	// - The object and property type maps initialized here must contain all types and properties
	//   appearing in old files to allow them to be deserialized before migration. This includes
	//   object and property types not used anymore.

	// Master list of all Property<T, Args...> types that can be deserialized.
	// Contains
	// - dynamic properties = all properties that can occur in Tables.
	// - static properties = properties declared as member variables EditorObject-derived classes
	// Value<T> does not appear here, even for pointer types derived from EditorObject.
	using PropertyTypeMapType = std::tuple<
		Property<std::string, URIAnnotation>,
		Property<double, DisplayNameAnnotation, RangeAnnotation<double>>,

		Property<int, DisplayNameAnnotation, EnumerationAnnotation>,
		Property<bool, DisplayNameAnnotation>,
		Property<Vec4f, DisplayNameAnnotation>,
		Property<BlendOptions, DisplayNameAnnotation>,

		Property<bool, EngineTypeAnnotation>,
		Property<int, EngineTypeAnnotation>,
		Property<int64_t, EngineTypeAnnotation>,
		Property<double, EngineTypeAnnotation>,
		Property<std::string, EngineTypeAnnotation>,
		Property<SEditorObject, EngineTypeAnnotation>,
		Property<Table, EngineTypeAnnotation>,
		Property<Vec2f, EngineTypeAnnotation>,
		Property<Vec3f, EngineTypeAnnotation>,
		Property<Vec4f, EngineTypeAnnotation>,
		Property<Vec2i, EngineTypeAnnotation>,
		Property<Vec3i, EngineTypeAnnotation>,
		Property<Vec4i, EngineTypeAnnotation>,
		Property<STexture, EngineTypeAnnotation>,
		Property<STextureExternal, EngineTypeAnnotation>,
		Property<STextureSampler2DBase, EngineTypeAnnotation>,
		Property<SRenderBufferMS, EngineTypeAnnotation>,
		Property<SCubeMap, EngineTypeAnnotation>,

		Property<bool, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<int, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<int64_t, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<double, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<std::string, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<SEditorObject, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Table, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec2f, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec3f, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec4f, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec2i, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec3i, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<Vec4i, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<STexture, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<STextureExternal, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<STextureSampler2DBase, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<SRenderBufferMS, EngineTypeAnnotation, LinkStartAnnotation>,
		Property<SCubeMap, EngineTypeAnnotation, LinkStartAnnotation>,

		Property<bool, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<int, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<int64_t, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<double, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<std::string, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<SEditorObject, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Table, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec2f, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec3f, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec4f, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec2i, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec3i, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<Vec4i, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<STexture, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<STextureExternal, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<STextureSampler2DBase, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<SRenderBufferMS, EngineTypeAnnotation, LinkEndAnnotation>,
		Property<SCubeMap, EngineTypeAnnotation, LinkEndAnnotation>,

		Property<bool, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<int, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<int64_t, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<double, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<std::string, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Table, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec2f, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec3f, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec4f, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec2i, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec3i, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Vec4i, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<STextureSampler2DBase, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<STextureExternal, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<SRenderBufferMS, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<SCubeMap, EngineTypeAnnotation, LinkStartAnnotation, LinkEndAnnotation>,

		// EditorObject
		Property<Table, ArraySemanticAnnotation, HiddenProperty>,
		Property<Table, ArraySemanticAnnotation, HiddenProperty, UserTagContainerAnnotation, DisplayNameAnnotation>,
		Property<std::string, HiddenProperty>,
		Property<std::string, DisplayNameAnnotation>,

		// Node
		Property<Table, ArraySemanticAnnotation, TagContainerAnnotation, DisplayNameAnnotation>,
		Property<Table, ArraySemanticAnnotation, HiddenProperty, TagContainerAnnotation, DisplayNameAnnotation>,
		Property<bool, DisplayNameAnnotation, LinkEndAnnotation>,
		Property<bool, DisplayNameAnnotation, LinkEndAnnotation, FeatureLevel>,
		Property<Vec3f, DisplayNameAnnotation, LinkEndAnnotation>,

		// MeshNode
		Property<SMesh, DisplayNameAnnotation>,
		Property<Table, DisplayNameAnnotation>,
		Property<int, DisplayNameAnnotation, RangeAnnotation<int>>,

		// ProjectSettings
		Property<Vec2i, DisplayNameAnnotation>,
		Property<Vec4i, DisplayNameAnnotation>,
		Property<Vec3f, DisplayNameAnnotation>,
		Property<bool, HiddenProperty>,
		Property<DefaultResourceDirectories, DisplayNameAnnotation>,
		Property<std::string, DisplayNameAnnotation, URIAnnotation>,
		Property<int, DisplayNameAnnotation, ReadOnlyAnnotation>,

		// LuaScript
		Property<Table, DisplayNameAnnotation, LinkEndAnnotation>,
		Property<LuaStandardModuleSelection, DisplayNameAnnotation>,

		// LuaInterface
		Property<Table, DisplayNameAnnotation, LinkStartAnnotation, LinkEndAnnotation>,
		Property<Table, DisplayNameAnnotation, FeatureLevel>,
		Property<LuaStandardModuleSelection, DisplayNameAnnotation, FeatureLevel>,

		// BaseCamera
		Property<CameraViewport, DisplayNameAnnotation, LinkEndAnnotation>,

		// CameraViewport
		Property<int, RangeAnnotation<int>, DisplayNameAnnotation, LinkEndAnnotation>,

		// PerspectiveCamera
		Property<PerspectiveFrustum, DisplayNameAnnotation, LinkEndAnnotation>,
		Property<int, DisplayNameAnnotation, EnumerationAnnotation, FeatureLevel>,

		// PerspectiveFrustum
		Property<double, DisplayNameAnnotation, RangeAnnotation<double>, LinkEndAnnotation>,

		// OrthographicCamera
		Property<OrthographicFrustum, DisplayNameAnnotation, LinkEndAnnotation>,

		// RenderBuffer
		Property<int, RangeAnnotation<int>, DisplayNameAnnotation>,

		// RenderPass
		Property<SRenderTarget, DisplayNameAnnotation, ExpectEmptyReference>,
		Property<SBaseCamera, DisplayNameAnnotation>,
		Property<SRenderLayer, DisplayNameAnnotation>,
		Property<SRenderLayer, DisplayNameAnnotation, ExpectEmptyReference>,
		Property<int, DisplayNameAnnotation>,
		Property<int, DisplayNameAnnotation, LinkEndAnnotation>,
		Property<Vec4f, DisplayNameAnnotation, LinkEndAnnotation>,

		// RenderLayer
		Property<Table, RenderableTagContainerAnnotation, DisplayNameAnnotation>,
		Property<Table, RenderableTagContainerAnnotation, HiddenProperty, DisplayNameAnnotation>,
		Property<bool, DisplayNameAnnotation, EnumerationAnnotation>,
		Property<int, LinkEndAnnotation>,

		// RenderTarget
		Property<SRenderBuffer, DisplayNameAnnotation>,
		Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference>,
		Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference>,

		// Texture
		Property<std::string, URIAnnotation, DisplayNameAnnotation>,

		// PrefabInstance
		Property<SPrefab, DisplayNameAnnotation>,

		// Timer
		Property<TimerInput, DisplayNameAnnotation>,
		Property<TimerOutput, DisplayNameAnnotation>,
		Property<int64_t, DisplayNameAnnotation, LinkEndAnnotation>,
		Property<int64_t, DisplayNameAnnotation, LinkStartAnnotation>,

		// AnchorPoint
		Property<SNode, DisplayNameAnnotation>,
		Property<AnchorPointOutputs, DisplayNameAnnotation>,
		Property<double, DisplayNameAnnotation, LinkStartAnnotation>,
		Property<Vec2f, DisplayNameAnnotation, LinkStartAnnotation>,

		// Skin
		Property<SMeshNode, DisplayNameAnnotation>,

		// BlendOptions
		Property<ColorWriteMask, DisplayNameAnnotation>,
		Property<CameraViewport, DisplayNameAnnotation>,
		Property<ScissorOptions, DisplayNameAnnotation>,
		Property<StencilOptions, DisplayNameAnnotation>
	>;

	static ProxyObjectFactory& getInstance();

	virtual SEditorObject createObject(const std::string& type, const std::string& name = std::string(), const std::string& id = std::string()) const override;
	virtual data_storage::ValueBase* createValue(const std::string& type) const override;

	const std::map<std::string, TypeDescriptor>& getTypes() const override;
	bool isUserCreatable(const std::string& type, int featureLevel) const override;

	const std::map<std::string, ValueCreationFunction>& getProperties() const;


	std::shared_ptr<AnnotationBase> createAnnotation(const std::string& type) const override;

	template <typename T, class... Args>
	static ValueBase* staticCreateProperty(T defaultT, Args... params) {
		static_assert(tuple_has_type<Property<T, Args...>, PropertyTypeMapType>::value == true);
		return new Property<T, Args...>(defaultT, params...);
	}

private:
	ProxyObjectFactory();

	using AnnotationCreationFunction = std::function<std::shared_ptr<AnnotationBase>()>;

	struct AnnotationDescriptor {
		ReflectionInterface::TypeDescriptor description;
		AnnotationCreationFunction createFunc;
	};

	template <class T>
	static SEditorObject createObjectInternal(const std::string& name, const std::string& id);
	template <class T>
	static std::shared_ptr<AnnotationBase> createAnnotationInternal();
	template <class T>
	static data_storage::ValueBase* createValueInternal();

	template<class... Args>
	std::map<std::string, ProxyObjectFactory::TypeDescriptor> makeTypeMap();
	template <class... Args>
	std::map<std::string, ValueCreationFunction> makePropertyMapTuple(std::tuple<Args...>* dummy);
	template <class... Args>
	std::map<std::string, ProxyObjectFactory::AnnotationDescriptor> makeAnnotationMap();

	std::map<std::string, TypeDescriptor> types_;
	// Annotations that can be dynmically added to / removed from ClassWithReflectedMembers
	std::map<std::string, AnnotationDescriptor> annotations_;
	std::map<std::string, ValueCreationFunction> properties_;
};

}  // namespace raco::serialization::proxy
