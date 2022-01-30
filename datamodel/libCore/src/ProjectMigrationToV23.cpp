/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/ProjectMigrationToV23.h"

#include "core/Serialization.h"
#include "core/SerializationKeys.h"

#include <QJsonObject>

using namespace raco::serialization;

namespace {

std::string structPropMap_V22 =
	R"___({
        "BlendOptions": {
            "blendColor": "Vec4f::DisplayNameAnnotation",
            "blendFactorDestAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorDestColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "cullmode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthFunction": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthwrite": "Bool::DisplayNameAnnotation"
        },
        "CameraViewport": {
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "offsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "offsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "DefaultResourceDirectories": {
            "imageSubdirectory": "String::DisplayNameAnnotation::URIAnnotation",
            "meshSubdirectory": "String::DisplayNameAnnotation::URIAnnotation",
            "scriptSubdirectory": "String::DisplayNameAnnotation::URIAnnotation",
            "shaderSubdirectory": "String::DisplayNameAnnotation::URIAnnotation"
        },
        "OrthographicFrustum": {
            "bottomPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "farPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "leftPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "nearPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "rightPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "topPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation"
        },
        "PerspectiveFrustum": {
            "aspectRatio": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "farPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "fieldOfView": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "nearPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation"
        }
    })___";

std::string userTypePropMap_V22 =
	R"___({
        "Animation": {
            "animationChannels": "Table::DisplayNameAnnotation",
            "animationOutputs": "Table::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "loop": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "play": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "rewindOnStop": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "AnimationChannel": {
            "animationIndex": "Int::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "samplerIndex": "Int::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaModules": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "LuaScriptModule": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec4f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "defaultResourceFolders": "DefaultResourceDirectories::DisplayNameAnnotation",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "RenderBuffer": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "format": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "RenderLayer": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "invertMaterialFilter": "Bool::DisplayNameAnnotation::EnumerationAnnotation",
            "materialFilterTags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "renderableTags": "Table::RenderableTagContainerAnnotation::DisplayNameAnnotation",
            "sortOrder": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation"
        },
        "RenderPass": {
            "camera": "BaseCamera::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "clearColor": "Vec4f::DisplayNameAnnotation",
            "enableClearColor": "Bool::DisplayNameAnnotation",
            "enableClearDepth": "Bool::DisplayNameAnnotation",
            "enableClearStencil": "Bool::DisplayNameAnnotation",
            "enabled": "Bool::DisplayNameAnnotation",
            "layer0": "RenderLayer::DisplayNameAnnotation",
            "layer1": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer2": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer3": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer4": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer5": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer6": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer7": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "order": "Int::DisplayNameAnnotation",
            "target": "RenderTarget::DisplayNameAnnotation::EmptyReferenceAllowable"
        },
        "RenderTarget": {
            "buffer0": "RenderBuffer::DisplayNameAnnotation",
            "buffer1": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer2": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer3": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer4": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer5": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer6": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer7": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "generateMipmaps": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";


std::string structPropMap_V21 =
	R"___({
        "BlendOptions": {
            "blendColor": "Vec4f::DisplayNameAnnotation",
            "blendFactorDestAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorDestColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "cullmode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthFunction": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthwrite": "Bool::DisplayNameAnnotation"
        },
        "CameraViewport": {
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "offsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "offsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicFrustum": {
            "bottomPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "farPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "leftPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "nearPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "rightPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "topPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation"
        },
        "PerspectiveFrustum": {
            "aspectRatio": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "farPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "fieldOfView": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "nearPlane": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation"
        }
    })___";

std::string userTypePropMap_V21 =
	R"___({
        "Animation": {
            "animationChannels": "Table::DisplayNameAnnotation",
            "animationOutputs": "Table::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "loop": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "play": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "rewindOnStop": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "AnimationChannel": {
            "animationIndex": "Int::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "samplerIndex": "Int::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaModules": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "LuaScriptModule": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec4f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "RenderBuffer": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "format": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "RenderLayer": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "invertMaterialFilter": "Bool::DisplayNameAnnotation::EnumerationAnnotation",
            "materialFilterTags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "renderableTags": "Table::RenderableTagContainerAnnotation::DisplayNameAnnotation",
            "sortOrder": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation"
        },
        "RenderPass": {
            "camera": "BaseCamera::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "clearColor": "Vec4f::DisplayNameAnnotation",
            "enableClearColor": "Bool::DisplayNameAnnotation",
            "enableClearDepth": "Bool::DisplayNameAnnotation",
            "enableClearStencil": "Bool::DisplayNameAnnotation",
            "enabled": "Bool::DisplayNameAnnotation",
            "layer0": "RenderLayer::DisplayNameAnnotation",
            "layer1": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer2": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer3": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer4": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer5": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer6": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer7": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "order": "Int::DisplayNameAnnotation",
            "target": "RenderTarget::DisplayNameAnnotation::EmptyReferenceAllowable"
        },
        "RenderTarget": {
            "buffer0": "RenderBuffer::DisplayNameAnnotation",
            "buffer1": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer2": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer3": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer4": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer5": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer6": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer7": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "generateMipmaps": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";


std::string& structPropMap_V20 = structPropMap_V21;

std::string userTypePropMap_V20 =
	R"___({
        "Animation": {
            "animationChannels": "Table::DisplayNameAnnotation",
            "animationOutputs": "Table::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "loop": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "play": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "rewindOnStop": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "AnimationChannel": {
            "animationIndex": "Int::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "samplerIndex": "Int::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaModules": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "LuaScriptModule": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec4f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "RenderBuffer": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "format": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "RenderLayer": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "invertMaterialFilter": "Bool::DisplayNameAnnotation::EnumerationAnnotation",
            "materialFilterTags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "renderableTags": "Table::RenderableTagContainerAnnotation::DisplayNameAnnotation",
            "sortOrder": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation"
        },
        "RenderPass": {
            "camera": "BaseCamera::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "clearColor": "Vec4f::DisplayNameAnnotation",
            "enableClearColor": "Bool::DisplayNameAnnotation",
            "enableClearDepth": "Bool::DisplayNameAnnotation",
            "enableClearStencil": "Bool::DisplayNameAnnotation",
            "enabled": "Bool::DisplayNameAnnotation",
            "layer0": "RenderLayer::DisplayNameAnnotation",
            "layer1": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer2": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer3": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer4": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer5": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer6": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer7": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "order": "Int::DisplayNameAnnotation",
            "target": "RenderTarget::DisplayNameAnnotation::EmptyReferenceAllowable"
        },
        "RenderTarget": {
            "buffer0": "RenderBuffer::DisplayNameAnnotation",
            "buffer1": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer2": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer3": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer4": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer5": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer6": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer7": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";


std::string& structPropMap_V19 = structPropMap_V20;

std::string userTypePropMap_V19 =
	R"___({
        "Animation": {
            "animationChannels": "Table::DisplayNameAnnotation",
            "animationOutputs": "Table::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "loop": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "play": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "rewindOnStop": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "AnimationChannel": {
            "animationIndex": "Int::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "samplerIndex": "Int::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec4f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "RenderBuffer": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "format": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "RenderLayer": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "invertMaterialFilter": "Bool::DisplayNameAnnotation::EnumerationAnnotation",
            "materialFilterTags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "renderableTags": "Table::RenderableTagContainerAnnotation::DisplayNameAnnotation",
            "sortOrder": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation"
        },
        "RenderPass": {
            "camera": "BaseCamera::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "clearColor": "Vec4f::DisplayNameAnnotation",
            "enableClearColor": "Bool::DisplayNameAnnotation",
            "enableClearDepth": "Bool::DisplayNameAnnotation",
            "enableClearStencil": "Bool::DisplayNameAnnotation",
            "enabled": "Bool::DisplayNameAnnotation",
            "layer0": "RenderLayer::DisplayNameAnnotation",
            "layer1": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer2": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer3": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer4": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer5": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer6": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer7": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "order": "Int::DisplayNameAnnotation",
            "target": "RenderTarget::DisplayNameAnnotation::EmptyReferenceAllowable"
        },
        "RenderTarget": {
            "buffer0": "RenderBuffer::DisplayNameAnnotation",
            "buffer1": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer2": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer3": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer4": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer5": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer6": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer7": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
})___";

std::string& structPropMap_V18 = structPropMap_V19;

std::string userTypePropMap_V18 =
	R"___({
        "Animation": {
            "animationChannels": "Table::DisplayNameAnnotation",
            "animationOutputs": "Table::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "loop": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "play": "Bool::DisplayNameAnnotation::LinkEndAnnotation",
            "rewindOnStop": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "AnimationChannel": {
            "animationIndex": "Int::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "samplerIndex": "Int::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec3f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "RenderBuffer": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "format": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "height": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "width": "Int::RangeAnnotationInt::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "RenderLayer": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "invertMaterialFilter": "Bool::DisplayNameAnnotation::EnumerationAnnotation",
            "materialFilterTags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "renderableTags": "Table::RenderableTagContainerAnnotation::DisplayNameAnnotation",
            "sortOrder": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "tags": "Table::ArraySemanticAnnotation::TagContainerAnnotation::DisplayNameAnnotation"
        },
        "RenderPass": {
            "camera": "BaseCamera::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "clearColor": "Vec4f::DisplayNameAnnotation",
            "enableClearColor": "Bool::DisplayNameAnnotation",
            "enableClearDepth": "Bool::DisplayNameAnnotation",
            "enableClearStencil": "Bool::DisplayNameAnnotation",
            "enabled": "Bool::DisplayNameAnnotation",
            "layer0": "RenderLayer::DisplayNameAnnotation",
            "layer1": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer2": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer3": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer4": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer5": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer6": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "layer7": "RenderLayer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "order": "Int::DisplayNameAnnotation",
            "target": "RenderTarget::DisplayNameAnnotation::EmptyReferenceAllowable"
        },
        "RenderTarget": {
            "buffer0": "RenderBuffer::DisplayNameAnnotation",
            "buffer1": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer2": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer3": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer4": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer5": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer6": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "buffer7": "RenderBuffer::DisplayNameAnnotation::EmptyReferenceAllowable",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";


std::string& structPropMap_V14 = structPropMap_V18;

std::string userTypePropMap_V14 =
	R"___({
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec3f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "flipTexture": "Bool::DisplayNameAnnotation",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    }
)___";


std::string structPropMap_V13 = structPropMap_V14;

std::string userTypePropMap_V13 =
	R"___({
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "options": "BlendOptions::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "OrthographicFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "frustum": "PerspectiveFrustum::DisplayNameAnnotation::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "CameraViewport::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec3f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "origin": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    }
)___";


std::string structPropMap_V12 = "{}";

std::string userTypePropMap_V12 =
	R"___({
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "blendColor": "Vec4f::DisplayNameAnnotation",
            "blendFactorDestAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorDestColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "cullmode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthFunction": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthwrite": "Bool::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "bottom": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "left": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "right": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "top": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortHeight": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortWidth": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "aspect": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "fov": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortHeight": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortWidth": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "backgroundColor": "Vec3f::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "origin": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";

std::string structPropMap_V11 = "{}";

std::string& userTypePropMap_V11 = userTypePropMap_V12;


std::string structPropMap_V10 = "{}";

std::string userTypePropMap_V10 =
	R"___({
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "blendColor": "Vec4f::DisplayNameAnnotation",
            "blendFactorDestAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorDestColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "cullmode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthFunction": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthwrite": "Bool::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "bottom": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "left": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "right": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "top": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortHeight": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortWidth": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "aspect": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "fov": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble::LinkEndAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortHeight": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetX": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortOffsetY": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "viewPortWidth": "Int::RangeAnnotationInt::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "origin": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";

            
std::string structPropMap_V9 = "{}";

std::string userTypePropMap_V9 =
	R"___({
        "CubeMap": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uriBack": "String::URIAnnotation::DisplayNameAnnotation",
            "uriBottom": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFront": "String::URIAnnotation::DisplayNameAnnotation",
            "uriLeft": "String::URIAnnotation::DisplayNameAnnotation",
            "uriRight": "String::URIAnnotation::DisplayNameAnnotation",
            "uriTop": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        },
        "LuaScript": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "luaInputs": "Table::DisplayNameAnnotation",
            "luaOutputs": "Table::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Material": {
            "blendColor": "Vec4f::DisplayNameAnnotation",
            "blendFactorDestAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorDestColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendFactorSrcColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationAlpha": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "blendOperationColor": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "cullmode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthFunction": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "depthwrite": "Bool::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uniforms": "Table::DisplayNameAnnotation",
            "uriDefines": "String::URIAnnotation::DisplayNameAnnotation",
            "uriFragment": "String::URIAnnotation::DisplayNameAnnotation",
            "uriGeometry": "String::URIAnnotation::DisplayNameAnnotation",
            "uriVertex": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "Mesh": {
            "bakeMeshes": "Bool::DisplayNameAnnotation",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "materialNames": "Table::ArraySemanticAnnotation::HiddenProperty",
            "meshIndex": "Int::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation"
        },
        "MeshNode": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "instanceCount": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "materials": "Table::DisplayNameAnnotation",
            "mesh": "Mesh::DisplayNameAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Node": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "OrthographicCamera": {
            "bottom": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "left": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "right": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "top": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "Vec4i::DisplayNameAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "PerspectiveCamera": {
            "aspect": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "far": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "fov": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "near": "Double::DisplayNameAnnotation::RangeAnnotationDouble",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "viewport": "Vec4i::DisplayNameAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "Prefab": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation"
        },
        "PrefabInstance": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "mapToInstance": "Table::ArraySemanticAnnotation::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "rotation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "scale": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "template": "Prefab::DisplayNameAnnotation",
            "translation": "Vec3f::DisplayNameAnnotation::LinkEndAnnotation",
            "visible": "Bool::DisplayNameAnnotation::LinkEndAnnotation"
        },
        "ProjectSettings": {
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "enableTimerFlag": "Bool::HiddenProperty",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "runTimer": "Bool::HiddenProperty",
            "sceneId": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "viewport": "Vec2i::DisplayNameAnnotation"
        },
        "Texture": {
            "anisotropy": "Int::DisplayNameAnnotation::RangeAnnotationInt",
            "children": "Table::ArraySemanticAnnotation::HiddenProperty",
            "magSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "minSamplingMethod": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "objectID": "String::HiddenProperty",
            "objectName": "String::DisplayNameAnnotation",
            "origin": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "uri": "String::URIAnnotation::DisplayNameAnnotation",
            "wrapUMode": "Int::DisplayNameAnnotation::EnumerationAnnotation",
            "wrapVMode": "Int::DisplayNameAnnotation::EnumerationAnnotation"
        }
    })___";


}  // namespace

namespace raco::serializationToV23 {

QJsonDocument migrateProjectToV23(const QJsonDocument& document) {
	int const documentVersion = deserializeFileVersion(document);

	QJsonObject documentObject{document.object()};

	// File Version 9: External references
	if (documentVersion < 9) {
		documentObject.insert(keys::EXTERNAL_PROJECTS, QJsonDocument::fromJson("{}").object());
    }
	
	if (documentVersion < 23) {
		// V7 and V8 same as V9
		if (documentVersion < 10) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V9.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V9.c_str()).object());
		} else if (documentVersion < 11) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V10.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V10.c_str()).object());
		} else if (documentVersion < 12) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V11.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V11.c_str()).object());
		} else if (documentVersion < 13) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V12.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V12.c_str()).object());
		} else if (documentVersion < 14) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V13.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V13.c_str()).object());
		} else if (documentVersion < 15) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V14.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V14.c_str()).object());
		} else if (documentVersion < 19) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V18.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V18.c_str()).object());
		} else if (documentVersion < 20) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V19.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V19.c_str()).object());
		} else if (documentVersion < 21) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V20.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V20.c_str()).object());
		} else if (documentVersion < 22) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V21.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V21.c_str()).object());
		} else if (documentVersion < 23) {
			documentObject.insert(keys::USER_TYPE_PROP_MAP, QJsonDocument::fromJson(userTypePropMap_V22.c_str()).object());
			documentObject.insert(keys::STRUCT_PROP_MAP, QJsonDocument::fromJson(structPropMap_V22.c_str()).object());
		}
	}

	QJsonDocument newDocument{documentObject};
	// for debugging:
	//auto migratedJSON = QString(newDocument.toJson()).toStdString();
	return newDocument;
}

}  // namespace raco::core