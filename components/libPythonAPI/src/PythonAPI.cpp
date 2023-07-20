/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined(WIN32) && defined(_DEBUG) && defined(PYTHON_DEBUG_LIBRARY_AVAILABLE)
// Needed to avoid pybind11/embed.h to cause linking to the non-debug DLL if the debug DLL is available.
// See https://github.com/pybind/pybind11/issues/3403#issuecomment-962878324
#define Py_DEBUG
#endif

#include <pybind11/iostream.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include <raco_pybind11_embed.h>

#include "python_api/PythonAPI.h"

#include "application/RaCoApplication.h"

#include "core/Handles.h"
#include "core/PathManager.h"
#include "core/Queries.h"

#include "core/ExternalReferenceAnnotation.h"
#include "core/PrefabOperations.h"

#include "user_types/Enumerations.h"
#include "user_types/Mesh.h"
#include "user_types/Prefab.h"
#include "user_types/PrefabInstance.h"
#include "user_types/RenderLayer.h"

#include <spdlog/fmt/fmt.h>

#include <stdlib.h>

namespace py = pybind11;

namespace {
raco::application::RaCoApplication* app;
raco::python_api::PythonRunStatus currentRunStatus;

py::object python_get_scalar_value(raco::core::ValueHandle handle) {
	using namespace raco::user_types;

	switch (handle.type()) {
		case raco::data_storage::PrimitiveType::Bool:
			return py::cast(handle.asBool());
			break;
		case raco::data_storage::PrimitiveType::Int:
			if (auto anno = handle.query<raco::core::EnumerationAnnotation>()) {
				switch (static_cast<raco::core::EUserTypeEnumerations>(anno->type_.asInt())) {
					case raco::core::EUserTypeEnumerations::CullMode:
						return py::cast(static_cast<ECullMode>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::BlendOperation:
						return py::cast(static_cast<EBlendOperation>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::BlendFactor:
						return py::cast(static_cast<EBlendFactor>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::DepthFunction:
						return py::cast(static_cast<EDepthFunc>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::TextureAddressMode:
						return py::cast(static_cast<ETextureAddressMode>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::TextureMinSamplingMethod:
					case raco::core::EUserTypeEnumerations::TextureMagSamplingMethod:
						return py::cast(static_cast<ETextureSamplingMethod>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::TextureFormat:
						return py::cast(static_cast<ETextureFormat>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::RenderBufferFormat:
						return py::cast(static_cast<ERenderBufferFormat>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::RenderLayerOrder:
						return py::cast(static_cast<ERenderLayerOrder>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::RenderLayerMaterialFilterMode:
						return py::cast(static_cast<ERenderLayerMaterialFilterMode>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::FrustumType:
						return py::cast(static_cast<EFrustumType>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::StencilFunction:
						return py::cast(static_cast<EStencilFunc>(handle.asInt()));

					case raco::core::EUserTypeEnumerations::StencilOperation:
						return py::cast(static_cast<EStencilOperation>(handle.asInt()));

					default:
						assert(false);
						return py::none();
				}
			} else {
				return py::cast(handle.asInt());
			}
			break;
		case raco::data_storage::PrimitiveType::Int64:
			return py::cast(handle.asInt64());
			break;
		case raco::data_storage::PrimitiveType::Double:
			return py::cast(handle.asDouble());
			break;
		case raco::data_storage::PrimitiveType::String:
			return py::cast(handle.asString());
			break;
		case raco::data_storage::PrimitiveType::Ref:
			return py::cast(handle.asRef());
			break;
		default:
			assert(false);
			return py::none();
	}
}

void checkObject(raco::core::SEditorObject object) {
	if (!app->activeRaCoProject().project()->isInstance(object)) {
		throw std::runtime_error(fmt::format("Object '{}' not in project", object->objectName()));
	}
}

template <typename UserType>
void checkTypedObject(raco::core::SEditorObject object) {
	checkObject(object);
	if (!object->isType<UserType>()) {
		throw std::runtime_error(fmt::format("Object '{}' is not of expected type '{}'", object->objectName(), UserType::typeDescription.typeName));
	}
}

void checkProperty(const raco::core::PropertyDescriptor& desc) {
	checkObject(desc.object());
	raco::core::ValueHandle handle{desc};
	if (!handle || handle.constValueRef()->query<raco::core::HiddenProperty>()) {
		throw std::runtime_error(fmt::format("Property '{}' doesn't exist in object '{}'", desc.getPropertyPath(), desc.object()->objectName()));
	}
	if (auto anno = handle.query<raco::core::FeatureLevel>(); anno && *anno->featureLevel_ > app->activeRaCoProject().project()->featureLevel()) {
		throw std::runtime_error(fmt::format("Property {} inaccessible at feature level {}", handle.getPropertyPath(), app->activeRaCoProject().project()->featureLevel()));
	}
}

void checkHiddenProperty(const raco::core::PropertyDescriptor& desc) {
	checkObject(desc.object());
	raco::core::ValueHandle handle{desc};
	if (!handle) {
		throw std::runtime_error(fmt::format("Property '{}' doesn't exist in object '{}'", desc.getPropertyPath(), desc.object()->objectName()));
	}
	if (auto anno = handle.query<raco::core::FeatureLevel>(); anno && *anno->featureLevel_ > app->activeRaCoProject().project()->featureLevel()) {
		throw std::runtime_error(fmt::format("Property {} inaccessible at feature level {}", handle.getPropertyPath(), app->activeRaCoProject().project()->featureLevel()));
	}
}

template <typename T>
void set_scalar_value_typed(const raco::core::ValueHandle& handle, py::object value, std::string_view typeName) {
	try {
		app->activeRaCoProject().commandInterface()->set(handle, value.cast<T>());
		app->doOneLoop();
	} catch (py::cast_error& e) {
		throw std::runtime_error(fmt::format("Set property '{}': can't convert value of type '{}' to property type '{}'.",
			handle.getPropertyPath(), py::cast<std::string>(repr(py::type::of(value))), typeName));
	}
}

template<typename T, int N>
std::array<T, N> to_std_array(const py::list& pyList) {
	auto result = std::array<T, N>{};
	std::transform(pyList.begin(), pyList.end(), result.begin(), [](const auto& pyObject) { return pyObject.template cast<T>(); });
	return result;
}

void set_as_vec(raco::core::ValueHandle const& handle, const py::list& pyList) {
	auto* commandInterface = app->activeRaCoProject().commandInterface();
	const auto typeDesc = &handle.constValueRef()->asStruct().getTypeDescription();
	if (typeDesc == &raco::core::Vec2f::typeDescription) {
		commandInterface->set(handle, to_std_array<double, 2>(pyList));
	} else if (typeDesc == &raco::core::Vec3f::typeDescription) {
		commandInterface->set(handle, to_std_array<double, 3>(pyList));
	} else if (typeDesc == &raco::core::Vec4f::typeDescription) {
		commandInterface->set(handle, to_std_array<double, 4>(pyList));
	} else if (typeDesc == &raco::core::Vec2i::typeDescription) {
		commandInterface->set(handle, to_std_array<int, 2>(pyList));
	} else if (typeDesc == &raco::core::Vec3i::typeDescription) {
		commandInterface->set(handle, to_std_array<int, 3>(pyList));
	} else if (typeDesc == &raco::core::Vec4i::typeDescription) {
		commandInterface->set(handle, to_std_array<int, 4>(pyList));
	}	else {
		throw std::runtime_error(fmt::format("Set property '{}': should be of vector type.", handle.getPropertyPath()));
	}
}

void python_set_value(const raco::core::PropertyDescriptor& desc, py::object value) {
	checkProperty(desc);
	raco::core::ValueHandle handle{desc};
	if (!handle.hasSubstructure()) {
		switch (handle.type()) {
			case raco::data_storage::PrimitiveType::Bool:
				set_scalar_value_typed<bool>(handle, value, "bool");
				break;
			case raco::data_storage::PrimitiveType::Int:
				set_scalar_value_typed<int>(handle, value, "int");
				break;
			case raco::data_storage::PrimitiveType::Int64:
				set_scalar_value_typed<int64_t>(handle, value, "int64");
				break;
			case raco::data_storage::PrimitiveType::Double:
				set_scalar_value_typed<double>(handle, value, "double");
				break;
			case raco::data_storage::PrimitiveType::String:
				set_scalar_value_typed<std::string>(handle, value, "string");
				break;
			case raco::data_storage::PrimitiveType::Ref:
				set_scalar_value_typed<raco::core::SEditorObject>(handle, value, "reference");
				break;
		}
	} else if (
		handle.isVec2f() || handle.isVec3f() || handle.isVec4f() ||
		handle.isVec2i() || handle.isVec3i() || handle.isVec4i()) {
		const auto pyList = py::cast<py::list>(value);
		if (handle.size() == pyList.size()) {
			set_as_vec(handle, pyList);
		} else {
			throw std::runtime_error(fmt::format("Set property '{}': iterable length ({}) doesn't match vector dimensions ({}).",
				handle.getPropertyPath(),
				pyList.size(),
				handle.size()));
		}
	} else {
		throw std::runtime_error(fmt::format("Set property '{}': can't set complex properties directly.", handle.getPropertyPath()));
	}
}

void python_load_project(std::string& path, int featureLevel) {
	if (app->isRunningInUI()) {
		throw std::runtime_error(fmt::format("Can not load project: project-switching Python functions currently not allowed in UI."));
	}

	if (!path.empty()) {
		try {
			app->switchActiveRaCoProject(QString::fromStdString(path), {}, false, featureLevel);
		} catch (std::exception& e) {
			app->switchActiveRaCoProject(QString(), {}, false, featureLevel);
			throw e;
		}
	} else {
		app->switchActiveRaCoProject(QString(), {}, false, featureLevel);
		throw std::runtime_error("Load project: file name is empty.");
	}
}

void python_import_gltf(const std::string path, const raco::core::SEditorObject parent) {
	auto fileLocation = raco::utils::u8path(path);
	fileLocation = fileLocation.normalizedAbsolutePath(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Mesh, app->activeProjectFolder()));

	auto absPath = fileLocation.string();

	auto* commandInterface = app->activeRaCoProject().commandInterface();
	// create dummy cache entry to prevent "cache corpses" if the mesh file is otherwise not accessed by any Mesh
	auto dummyCacheEntry = commandInterface->meshCache()->registerFileChangedHandler(absPath, {nullptr, nullptr});
	if (auto* sceneGraph = commandInterface->meshCache()->getMeshScenegraph(absPath)) {
		commandInterface->insertAssetScenegraph(*sceneGraph, absPath, parent);
	} else {
		throw std::invalid_argument("Unable to import GLTF mesh at " + fileLocation.string());
	}

	app->doOneLoop();
}

raco::core::Project* python_add_external_project(const std::string& path) {
	auto projectFileLocation = raco::utils::u8path(path);
	projectFileLocation = projectFileLocation.normalizedAbsolutePath(app->activeProjectFolder());

	raco::core::LoadContext loadContext;
	loadContext.featureLevel = app->activeRaCoProject().project()->featureLevel();
	loadContext.pathStack.emplace_back(app->activeRaCoProject().project()->currentPath());
	auto* project = app->externalProjects()->addExternalProject(projectFileLocation.string(), loadContext);
	if (project == nullptr) {
		throw std::invalid_argument("Unable to add external project from " + projectFileLocation.string());
	}

	return project;
}

py::object python_add_external_references(const std::string& path, const std::vector<std::string>& types) {
	auto* project = python_add_external_project(path);
	if (project == nullptr) {
		return py::cast(std::vector<raco::core::SEditorObject>());
	}
	std::vector<raco::core::SEditorObject> editorObjects;
	for (const auto& type : types) {
		for (const auto& item : project->instances()) {
			if (item->getTypeDescription().typeName == type) {
				editorObjects.push_back(item);
			}
		}
	}

	auto* projectInterface = app->externalProjects()->getExternalProjectCommandInterface(project->currentPath());
	auto value = projectInterface->copyObjects(editorObjects, false);
	auto result = app->activeRaCoProject().commandInterface()->pasteObjects(value, nullptr, true);

	return py::cast(result);
}

py::object getPropertyChildProperties(const raco::core::PropertyDescriptor& desc) {
	checkProperty(desc);
	auto handle = raco::core::ValueHandle(desc);
	std::vector<std::string> propNames;
	if (handle.hasSubstructure()) {
		for (size_t index = 0; index < handle.size(); index++) {
			propNames.emplace_back(handle[index].getPropName());
		}
	}
	return py::cast(propNames);
}

py::object getObjectChildProperties(raco::core::SEditorObject obj) {
	checkObject(obj);
	std::vector<std::string> propNames;
	for (size_t index = 0; index < obj->size(); index++) {
		if (!obj->get(index)->query<raco::core::HiddenProperty>()) {
			auto anno = obj->get(index)->query<raco::core::FeatureLevel>();
			if (!anno || *anno->featureLevel_ <= app->activeRaCoProject().project()->featureLevel()) {
				propNames.emplace_back(obj->name(index));
			}
		}
	}
	return py::cast(propNames);
}

std::vector<std::string> getTags(raco::core::SEditorObject obj, std::string tagPropertyName) {
	raco::core::PropertyDescriptor desc(obj, {tagPropertyName});
	checkHiddenProperty(desc);
	raco::core::ValueHandle handle(desc);
	if (handle.query<raco::core::TagContainerAnnotation>() || handle.query<raco::core::UserTagContainerAnnotation>()) {
		return handle.constValueRef()->asTable().asVector<std::string>();
	} else {
		throw std::runtime_error(fmt::format("Property '{}' is not a tag container.", desc.getPropertyPath()));
	}
	return std::vector<std::string>();
}

void setTags(raco::core::SEditorObject obj, std::vector<std::string> tags, std::string tagPropertyName) {
	raco::core::PropertyDescriptor desc(obj, {tagPropertyName});
	checkHiddenProperty(desc);
	raco::core::ValueHandle handle(desc);
	if (handle.query<raco::core::TagContainerAnnotation>() || handle.query<raco::core::UserTagContainerAnnotation>()) {
		app->activeRaCoProject().commandInterface()->setTags(handle, tags);
		app->doOneLoop();
	} else {
		throw std::runtime_error(fmt::format("Property '{}' is not a tag container.", desc.getPropertyPath()));
	}
}

}  // namespace

PYBIND11_EMBEDDED_MODULE(raco_py_io, m) {
	struct raco_py_stdout {
		raco_py_stdout() = default;
		raco_py_stdout(const raco_py_stdout&) = default;
		raco_py_stdout(raco_py_stdout&&) = default;
	};

	struct raco_py_stderr {
		raco_py_stderr() = default;
		raco_py_stderr(const raco_py_stderr&) = default;
		raco_py_stderr(raco_py_stderr&&) = default;
	};

	py::class_<raco_py_stdout> raco_py_stdout(m, "raco_py_stdout");
	raco_py_stdout.def_static("write", [](py::object buffer) {
		currentRunStatus.stdOutBuffer.append(buffer.cast<std::string>());
	});
	raco_py_stdout.def_static("flush", []() {
		// empty flush redefinition needed for overwriting of sys.stdout
	});

	py::class_<raco_py_stderr> raco_py_stderr(m, "raco_py_stderr");
	raco_py_stderr.def_static("write", [](py::object buffer) {
		currentRunStatus.stdErrBuffer.append(buffer.cast<std::string>());
	});
	raco_py_stderr.def_static("flush", []() {
		// empty flush redefinition needed for overwriting of sys.stderr
	});

	m.def("hook_stdout", []() {
		auto py_sys = py::module::import("sys");
		auto pyIo = py::module::import("raco_py_io");
		py_sys.attr("stdout") = pyIo.attr("raco_py_stdout");
		py_sys.attr("stderr") = pyIo.attr("raco_py_stderr");
	});
}


PYBIND11_EMBEDDED_MODULE(raco, m) {
	using namespace raco::user_types;

	py::enum_<ECullMode>(m, "ECullMode")
		.value("Disabled", ECullMode::Disabled)
		.value("Front", ECullMode::FrontFacing)
		.value("Back", ECullMode::BackFacing)
		.value("FrontAndBack", ECullMode::FrontAndBackFacing);

	py::enum_<EBlendOperation>(m, "EBlendOperation")
		.value("Disabled", EBlendOperation::Disabled)
		.value("Add", EBlendOperation::Add)
		.value("Subtract", EBlendOperation::Subtract)
		.value("ReverseSubtract", EBlendOperation::ReverseSubtract)
		.value("Min", EBlendOperation::Min)
		.value("Max", EBlendOperation::Max);

	py::enum_<EBlendFactor>(m, "EBlendFactor")
		.value("Zero", EBlendFactor::Zero)
		.value("One", EBlendFactor::One)
		.value("SrcAlpha", EBlendFactor::SrcAlpha)
		.value("OneMinusSrcAlpha", EBlendFactor::OneMinusSrcAlpha)
		.value("DstAlpha", EBlendFactor::DstAlpha)
		.value("OneMinusDstAlpha", EBlendFactor::OneMinusDstAlpha)
		.value("SrcColor", EBlendFactor::SrcColor)
		.value("OneMinusSrcColor", EBlendFactor::OneMinusSrcColor)
		.value("DstColor", EBlendFactor::DstColor)
		.value("OneMinusDstColor", EBlendFactor::OneMinusDstColor)
		.value("ConstColor", EBlendFactor::ConstColor)
		.value("OneMinusConstColor", EBlendFactor::OneMinusConstColor)
		.value("ConstAlpha", EBlendFactor::ConstAlpha)
		.value("OneMinusConstAlpha", EBlendFactor::OneMinusConstAlpha)
		.value("AlphaSaturate", EBlendFactor::AlphaSaturate);

	py::enum_<EDepthFunc>(m, "EDepthFunction")
		.value("Disabled", EDepthFunc::Disabled)
		.value("Greater", EDepthFunc::Greater)
		.value("GreaterEqual", EDepthFunc::GreaterEqual)
		.value("Less", EDepthFunc::Less)
		.value("LessEqual", EDepthFunc::LessEqual)
		.value("Equal", EDepthFunc::Equal)
		.value("NotEqual", EDepthFunc::NotEqual)
		.value("Always", EDepthFunc::Always)
		.value("Never", EDepthFunc::Never);

	py::enum_<EStencilFunc>(m, "EStencilFunction")
		.value("Disabled", EStencilFunc::Disabled)
		.value("Never", EStencilFunc::Never)
		.value("Always", EStencilFunc::Always)
		.value("Equal", EStencilFunc::Equal)
		.value("NotEqual", EStencilFunc::NotEqual)
		.value("Less", EStencilFunc::Less)
		.value("LessEqual", EStencilFunc::LessEqual)
		.value("Greater", EStencilFunc::Greater)
		.value("GreaterEqual", EStencilFunc::GreaterEqual);

	py::enum_<EStencilOperation>(m, "EStencilOperation")
		.value("Keep", EStencilOperation::Keep)
		.value("Zero", EStencilOperation::Zero)
		.value("Replace", EStencilOperation::Replace)
		.value("Increment", EStencilOperation::Increment)
		.value("IncrementWrap", EStencilOperation::IncrementWrap)
		.value("Decrement", EStencilOperation::Decrement)
		.value("DecrementWrap", EStencilOperation::DecrementWrap)
		.value("Invert", EStencilOperation::Invert);

	py::enum_<ETextureAddressMode>(m, "ETextureAddressMode")
		.value("Clamp", ETextureAddressMode::Clamp)
		.value("Repeat", ETextureAddressMode::Repeat)
		.value("Mirror", ETextureAddressMode::Mirror);

	py::enum_<ETextureSamplingMethod>(m, "ETextureSamplingMethod")
		.value("Nearest", ETextureSamplingMethod::Nearest)
		.value("Linear", ETextureSamplingMethod::Linear)
		.value("Nearest_MipMapNearest", ETextureSamplingMethod::Nearest_MipMapNearest)
		.value("Nearest_MipMapLinear", ETextureSamplingMethod::Nearest_MipMapLinear)
		.value("Linear_MipMapNearest", ETextureSamplingMethod::Linear_MipMapNearest)
		.value("Linear_MipMapLinear", ETextureSamplingMethod::Linear_MipMapLinear);

	py::enum_<ETextureFormat>(m, "ETextureFormat")
		.value("R8", ETextureFormat::R8)
		.value("RG8", ETextureFormat::RG8)
		.value("RGB8", ETextureFormat::RGB8)
		.value("RGBA8", ETextureFormat::RGBA8)
		.value("RGB16F", ETextureFormat::RGB16F)
		.value("RGBA16F", ETextureFormat::RGBA16F)
		.value("SRGB8", ETextureFormat::SRGB8)
		.value("SRGB8_ALPHA8", ETextureFormat::SRGB8_ALPHA8);

	py::enum_<ERenderBufferFormat>(m, "ERenderBufferFormat")
		.value("RGBA4", ERenderBufferFormat::RGBA4)
		.value("R8", ERenderBufferFormat::R8)
		.value("RG8", ERenderBufferFormat::RG8)
		.value("RGB8", ERenderBufferFormat::RGB8)
		.value("RGBA8", ERenderBufferFormat::RGBA8)
		.value("R16F", ERenderBufferFormat::R16F)
		.value("R32F", ERenderBufferFormat::R32F)
		.value("RG16F", ERenderBufferFormat::RG16F)
		.value("RG32F", ERenderBufferFormat::RG32F)
		.value("RGB16F", ERenderBufferFormat::RGB16F)
		.value("RGB32F", ERenderBufferFormat::RGB32F)
		.value("RGBA16F", ERenderBufferFormat::RGBA16F)
		.value("RGBA32F", ERenderBufferFormat::RGBA32F)

		.value("Depth24", ERenderBufferFormat::Depth24)
		.value("Depth24_Stencil8", ERenderBufferFormat::Depth24_Stencil8);

	py::enum_<ERenderLayerOrder>(m, "ERenderLayerOrder")
		.value("Manual", ERenderLayerOrder::Manual)
		.value("SceneGraph", ERenderLayerOrder::SceneGraph);

	py::enum_<ERenderLayerMaterialFilterMode>(m, "ERenderLayerMaterialFilterMode")
		.value("Inclusive", ERenderLayerMaterialFilterMode::Inclusive)
		.value("Exclusive", ERenderLayerMaterialFilterMode::Exclusive);

	py::enum_<EFrustumType>(m, "EFrustumType")
		.value("Aspect_FoV", EFrustumType::Aspect_FieldOfView)
		.value("Planes", EFrustumType::Planes);

	py::enum_<ErrorCategory>(m, "ErrorCategory")
		.value("GENERAL", ErrorCategory::GENERAL)
		.value("PARSING", ErrorCategory::PARSING)
		.value("FILESYSTEM", ErrorCategory::FILESYSTEM)
		.value("RAMSES_LOGIC_RUNTIME", ErrorCategory::RAMSES_LOGIC_RUNTIME)
		.value("EXTERNAL_REFERENCE", ErrorCategory::EXTERNAL_REFERENCE)
		.value("MIGRATION", ErrorCategory::MIGRATION);

	py::enum_<ErrorLevel>(m, "ErrorLevel")
		.value("NONE", ErrorLevel::NONE)
		.value("INFORMATION", ErrorLevel::INFORMATION)
		.value("WARNING", ErrorLevel::WARNING)
		.value("ERROR", ErrorLevel::ERROR);

	py::enum_<raco::application::ELuaSavingMode>(m, "ELuaSavingMode")
		.value("SourceCodeOnly", raco::application::ELuaSavingMode::SourceCodeOnly)
		.value("ByteCodeOnly", raco::application::ELuaSavingMode::ByteCodeOnly)
		.value("SourceAndByteCode", raco::application::ELuaSavingMode::SourceAndByteCode);


	m.def("load", [](std::string path) {
		python_load_project(path, -1);
	});

	m.def("load", [](std::string path, int featureLevel) {
		python_load_project(path, featureLevel);
	});

	m.def("reset", []() {
		if (app->isRunningInUI()) {
			throw std::runtime_error(fmt::format("Can not reset project: project-switching Python functions currently not allowed in UI."));
		}

		app->switchActiveRaCoProject(QString(), {}, false);
	});

	m.def("reset", [](int featureLevel) {
		if (app->isRunningInUI()) {
			throw std::runtime_error(fmt::format("Can not reset project: project-switching Python functions currently not allowed in UI."));
		}

		app->switchActiveRaCoProject(QString(), {}, false, featureLevel);
	});

	m.def("save", [](std::string path) {
		if (app->canSaveActiveProject()) {
			std::string errorMsg;
			if (!app->activeRaCoProject().saveAs(QString::fromStdString(path), errorMsg, app->activeProjectPath().empty())) {
				throw std::runtime_error(fmt::format("Saving project to '{}' failed with error '{}'.", path, errorMsg));
			}
		} else {
			throw std::runtime_error(fmt::format("Can not save project: externally referenced projects not clean."));
		}
	});

	m.def("save", [](std::string path, bool setNewIDs) {
		if (app->isRunningInUI()) {
			throw std::runtime_error(fmt::format("Can not load project: project-switching Python functions currently not allowed in UI."));
		}

		if (app->canSaveActiveProject()) {
			std::string errorMsg;
			bool success;
			if (setNewIDs) {
				success = app->saveAsWithNewIDs(QString::fromStdString(path), errorMsg, app->activeProjectPath().empty());
			} else {
				success = app->activeRaCoProject().saveAs(QString::fromStdString(path), errorMsg, app->activeProjectPath().empty());
			}
			if (!success) {
				throw std::runtime_error(fmt::format("Saving project to '{}' failed with error '{}'.", path, errorMsg));
			}
		} else {
			throw std::runtime_error(fmt::format("Can not save project: externally referenced projects not clean."));
		}
	});

	m.def("projectPath", []() {
		return app->activeProjectPath();
	});

	m.def("projectFeatureLevel", []() {
		return app->activeRaCoProject().project()->featureLevel();
	});
	m.def("minFeatureLevel", []() {
		return app->minFeatureLevel();
	});
	m.def("maxFeatureLevel", []() {
		return app->maxFeatureLevel();
	});

	m.def("externalProjects", []() {
		py::list externalPaths;
		for (auto const& [id, info] : app->activeRaCoProject().project()->externalProjectsMap()) {
			auto absPath = raco::utils::u8path(info.path).normalizedAbsolutePath(app->activeRaCoProject().project()->currentFolder());
			externalPaths.append(py::cast(absPath.string()));
		}
		return externalPaths;
	});

	m.def("export", [](std::string ramsesExport, std::string logicExport, bool compress) {
		std::string outError;
		if (!app->exportProject(ramsesExport, logicExport, compress, outError)) {
			throw std::runtime_error(fmt::format(("Export failed: {}", outError)));
		}
	});

	m.def("export", [](std::string ramsesExport, std::string logicExport, bool compress, raco::application::ELuaSavingMode luaSavingMode) {
		std::string outError;
		if (!app->exportProject(ramsesExport, logicExport, compress, outError, false, luaSavingMode)) {
			throw std::runtime_error(fmt::format(("Export failed: {}", outError)));
		}
	});

	m.def("isRunningInUi", []() {
		return app->isRunningInUI();
	});

	m.def("importGLTF", [](const std::string path) {
		python_import_gltf(path, nullptr);
	});

	m.def("importGLTF", [](const std::string path, const raco::core::SEditorObject parent) {
		checkObject(parent);
		python_import_gltf(path, parent);
	});

	m.def("addExternalProject", [](const std::string& path) {
		python_add_external_project(path);
	});

	m.def("addExternalReferences", [](const std::string& path, const std::vector<std::string> types) -> py::object {
		return python_add_external_references(path, types);
	});

	m.def("addExternalReferences", [](const std::string& path, const std::string& type) -> py::object {
		return python_add_external_references(path, {type});
	});

	py::class_<raco::core::PropertyDescriptor>(m, "PropertyDescriptor")
		.def("__repr__", [](const raco::core::PropertyDescriptor& desc) {
			auto handle = raco::core::ValueHandle(desc);
			return fmt::format("<Property[{}]: '{}'>", handle.constValueRef()->baseTypeName(), desc.getPropertyPath());
		})
		.def("__dir__", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			return getPropertyChildProperties(desc);
		})
		.def("keys", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			return getPropertyChildProperties(desc);
		})
		.def("__getattr__", [](const raco::core::PropertyDescriptor& desc, const std::string& name) -> py::object {
			auto childDesc = desc.child(name);
			checkProperty(childDesc);
			return py::cast(childDesc);
		})
		.def("__setattr__", [](const raco::core::PropertyDescriptor& desc, const std::string& name, py::object value) {
			python_set_value(desc.child(name), value);
		})
		.def("__getitem__", [](const raco::core::PropertyDescriptor& desc, const std::string& name) -> py::object {
			auto childDesc = desc.child(name);
			checkProperty(childDesc);
			return py::cast(childDesc);
		})
		.def("__setitem__", [](const raco::core::PropertyDescriptor& desc, const std::string& name, py::object value) {
			python_set_value(desc.child(name), value);
		})
		.def(py::self == py::self)
		.def("object", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			checkObject(desc.object());
			return py::cast(desc.object());
		})
		// TODO get parent property
		.def("typeName", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			checkProperty(desc);
			return py::cast(raco::core::ValueHandle(desc).constValueRef()->baseTypeName());
		})
		.def("propName", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			checkProperty(desc);
			return py::cast(raco::core::ValueHandle(desc).getPropName());
		})
		.def("hasSubstructure", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			checkProperty(desc);
			return py::cast(raco::core::ValueHandle(desc).hasSubstructure());
		})
		.def("isReadOnly", [](const raco::core::PropertyDescriptor& desc) {
			checkProperty(desc);
			return raco::core::Queries::isReadOnly(*app->activeRaCoProject().project(), raco::core::ValueHandle(desc));
		})
		.def("isValidLinkStart", [](const raco::core::PropertyDescriptor& desc) {
			checkProperty(desc);
			return raco::core::Queries::isValidLinkStart(raco::core::ValueHandle(desc));
		})
		.def("isValidLinkEnd", [](const raco::core::PropertyDescriptor& desc) {
			checkProperty(desc);
			return raco::core::Queries::isValidLinkEnd(*app->activeRaCoProject().project(), raco::core::ValueHandle(desc));
		})
		.def("value", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			checkProperty(desc);
			auto handle = raco::core::ValueHandle(desc);
			if (!handle.hasSubstructure()) {
				return python_get_scalar_value(handle);
			} else {
				throw std::runtime_error(fmt::format("Can't read property value: '{}' is not a scalar property.", desc.getPropertyPath()));
			}
		});

	py::class_<raco::core::EditorObject, raco::core::SEditorObject>(m, "EditorObject")
		.def("__repr__", [](const raco::core::EditorObject& obj) {
			return fmt::format("<{}: '{}'>", obj.getTypeDescription().typeName, obj.objectName());
		})
		.def("__dir__", [](raco::core::SEditorObject obj) {
			return getObjectChildProperties(obj);
		})
		.def("keys", [](raco::core::SEditorObject obj) {
			return getObjectChildProperties(obj);
		})
		.def("__getattr__", [](raco::core::SEditorObject obj, std::string name) -> py::object {
			raco::core::PropertyDescriptor desc(obj, {name});
			checkProperty(desc);
			return py::cast(desc);
		})
		.def("__setattr__", [](raco::core::SEditorObject obj, std::string name, py::object value) {
			python_set_value({obj, {name}}, value);
		})
		.def("__getitem__", [](raco::core::SEditorObject obj, std::string name) -> py::object {
			raco::core::PropertyDescriptor desc(obj, {name});
			checkProperty(desc);
			return py::cast(desc);
		})
		.def("__setitem__", [](raco::core::SEditorObject obj, std::string name, py::object value) {
			python_set_value({obj, {name}}, value);
		})
		.def("typeName", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return obj->getTypeDescription().typeName;
		})
		.def("children", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return obj->children_->asVector<raco::core::SEditorObject>();
		})
		.def("isReadOnly", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return raco::core::Queries::isReadOnly(obj);
		})
		.def("isResource", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return raco::core::Queries::isResource(obj);
		})
		.def("isExternalReference", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return obj->query<raco::core::ExternalReferenceAnnotation>() != nullptr;
		})
		.def("getPrefab", [](raco::core::SEditorObject obj) -> py::object {
			checkObject(obj);
			return py::cast(raco::core::SEditorObject(raco::core::PrefabOperations::findContainingPrefab(obj)));
		})
		.def("getPrefabInstance", [](raco::core::SEditorObject obj) -> py::object {
			checkObject(obj);
			return py::cast(raco::core::SEditorObject(raco::core::PrefabOperations::findContainingPrefabInstance(obj)));
		})
		.def("getOuterContainingPrefabInstance", [](raco::core::SEditorObject obj) -> py::object {
			checkObject(obj);
			return py::cast(raco::core::SEditorObject(raco::core::PrefabOperations::findOuterContainingPrefabInstance(obj)));
		})
		.def("parent", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return py::cast(obj->getParent());
		})
		.def("objectID", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			return py::cast(obj->objectID());
		})
		.def("metadata", [](raco::core::SEditorObject obj) {
			checkObject(obj);
			if (obj->isType<raco::user_types::Mesh>()) {
				auto metaData = obj->as<raco::user_types::Mesh>()->metaData_;
				if (!metaData.empty()) {
					return py::cast(metaData);
				}
			}
			return py::cast(nullptr);
		})
		.def("getTags", [](raco::core::SEditorObject obj) -> std::vector<std::string> {
			return getTags(obj, "tags");
		})
		.def("setTags", [](raco::core::SEditorObject obj, std::vector<std::string> tags) {
			setTags(obj, tags, "tags");
		})
		.def("getMaterialFilterTags", [](raco::core::SEditorObject obj) -> std::vector<std::string> {
			return getTags(obj, "materialFilterTags");
		})
		.def("setMaterialFilterTags", [](raco::core::SEditorObject obj, std::vector<std::string> tags) {
			setTags(obj, tags, "materialFilterTags");
		})
		.def("getUserTags", [](raco::core::SEditorObject obj) -> std::vector<std::string> {
			return getTags(obj, "userTags");
		})
		.def("setUserTags", [](raco::core::SEditorObject obj, std::vector<std::string> tags) {
			setTags(obj, tags, "userTags");
		})
		.def("getRenderableTags", [](raco::core::SEditorObject obj) -> std::vector<std::pair<std::string, int>> {
			checkTypedObject<raco::user_types::RenderLayer>(obj);
			raco::core::ValueHandle handle(obj, &raco::user_types::RenderLayer::renderableTags_);
			std::vector<std::pair<std::string, int>> renderables;
			for (size_t index = 0; index < handle.size(); index++) {
				renderables.emplace_back(handle[index].getPropName(), handle[index].asInt());
			}
			return renderables;
		})
		.def("setRenderableTags", [](raco::core::SEditorObject obj, std::vector<std::pair<std::string, int>> renderables) {
			checkTypedObject<raco::user_types::RenderLayer>(obj);
			raco::core::ValueHandle handle(obj, &raco::user_types::RenderLayer::renderableTags_);
			if (handle) {
				app->activeRaCoProject().commandInterface()->setRenderableTags(handle, renderables);
				app->doOneLoop();
			}
		});

	py::class_<raco::core::LinkDescriptor>(m, "LinkDescriptor")
		.def("__repr__", [](const raco::core::LinkDescriptor& desc) {
			return fmt::format("<Link: start='{}' end='{}' valid='{}' weak='{}'>", desc.start.getPropertyPath(), desc.end.getPropertyPath(), desc.isValid, desc.isWeak);
		})
		.def(py::self == py::self)
		.def_readonly("start", &raco::core::LinkDescriptor::start)
		.def_readonly("end", &raco::core::LinkDescriptor::end)
		.def_readonly("valid", &raco::core::LinkDescriptor::isValid)
		.def_readonly("weak", &raco::core::LinkDescriptor::isWeak);

	m.def("instances", []() {
		return app->activeRaCoProject().project()->instances();
	});

	m.def("create", [](std::string typeName, std::string objectName) {
		auto object = app->activeRaCoProject().commandInterface()->createObject(typeName, objectName);
		app->doOneLoop();
		return object;
	});

	m.def("delete", [](raco::core::SEditorObject obj) {
		auto result = app->activeRaCoProject().commandInterface()->deleteObjects({obj});
		app->doOneLoop();
		return result;
	});

	m.def("delete", [](std::vector<raco::core::SEditorObject> objects) {
		auto result = app->activeRaCoProject().commandInterface()->deleteObjects(objects);
		app->doOneLoop();
		return result;
	});

	m.def("moveScenegraph", [](raco::core::SEditorObject object, raco::core::SEditorObject newParent) {
		app->activeRaCoProject().commandInterface()->moveScenegraphChildren({object}, newParent);
		app->doOneLoop();
	});
	m.def("moveScenegraph", [](raco::core::SEditorObject object, raco::core::SEditorObject newParent, int insertBeforeIndex) {
		app->activeRaCoProject().commandInterface()->moveScenegraphChildren({object}, newParent, insertBeforeIndex);
		app->doOneLoop();
	});

	m.def("links", []() {
		const auto& links = app->activeRaCoProject().project()->links();
		py::list pyLinks;
		for (auto link : links) {
			pyLinks.append(py::cast(link->descriptor()));
		}
		return pyLinks;
	});

	m.def("getLink", [](const raco::core::PropertyDescriptor& end) -> py::object {
		checkObject(end.object());
		if (auto link = raco::core::Queries::getLink(*app->activeRaCoProject().project(), end)) {
			return py::cast(link->descriptor());
		}
		return py::none();
	});

	m.def("addLink", [](const raco::core::PropertyDescriptor& start, const raco::core::PropertyDescriptor& end) -> py::object {
		if (auto newLink = app->activeRaCoProject().commandInterface()->addLink(raco::core::ValueHandle(start), raco::core::ValueHandle(end))) {
			app->doOneLoop();
			return py::cast(newLink->descriptor());
		}
		return py::none();
	});

	m.def("addLink", [](const raco::core::PropertyDescriptor& start, const raco::core::PropertyDescriptor& end, bool isWeak) -> py::object {
		if (auto newLink = app->activeRaCoProject().commandInterface()->addLink(raco::core::ValueHandle(start), raco::core::ValueHandle(end), isWeak)) {
			app->doOneLoop();
			return py::cast(newLink->descriptor());
		}
		return py::none();
	});

	m.def("removeLink", [](const raco::core::PropertyDescriptor& end) {
		app->activeRaCoProject().commandInterface()->removeLink(end);
		app->doOneLoop();
	});

	m.def("getInstanceById", [](const std::string& id) -> py::object {
		return py::cast(app->activeRaCoProject().project()->getInstanceByID(id));
	});

	py::class_<raco::core::ErrorItem>(m, "ErrorItem")
		.def("__repr__", [](const raco::core::ErrorItem& errorItem) {
			auto pyobj = py::cast(errorItem).attr("handle")();
			std::string handleRepr = py::repr(pyobj);
			return fmt::format("<Error: category='{}' level='{}' message='{}' handle='{}'>", errorItem.category(), errorItem.level(), errorItem.message(), handleRepr);
		})
		.def("category", &raco::core::ErrorItem::category)
		.def("level", &raco::core::ErrorItem::level)
		.def("message", &raco::core::ErrorItem::message)
		.def("handle", [](const raco::core::ErrorItem& errorItem) -> py::object {
			if (errorItem.valueHandle() == raco::core::ValueHandle()) {
				// Project global errors -> return None
				return py::none();
			} else if (errorItem.valueHandle().isObject()) {
				// Object errors -> return EditorObject
				auto obj = errorItem.valueHandle().rootObject();
				checkObject(obj);
				return py::cast(obj);
			}
			// Property errors -> return PropertyDescriptor
			auto desc = errorItem.valueHandle().getDescriptor();
			checkProperty(desc);
			return py::cast(desc);
		});

	m.def("getErrors", []() {
		py::list pyErrorItems;
		for (const auto& [object, objErrors] : app->activeRaCoProject().errors()->getAllErrors()) {
			for (const auto& [handle, error] : objErrors) {
				pyErrorItems.append(raco::core::ErrorItem(error));
			}
		}
		return pyErrorItems;
	});

	m.def("resolveUriPropertyToAbsolutePath", [](const PropertyDescriptor& desc) {
		checkProperty(desc);
		const auto handle = ValueHandle(desc);
		const auto uriAnnotation = handle.constValueRef()->query<URIAnnotation>();
		if (uriAnnotation) {
			return PathQueries::resolveUriPropertyToAbsolutePath(*app->activeRaCoProject().project(), handle);
		} else {
			throw std::runtime_error(fmt::format("Can't resolve uri to absolute path: '{}' is not a uri property.", desc.getPropertyPath()));
		}
	});
}

namespace raco::python_api {

bool preparePythonEnvironment(std::wstring argv0, const std::vector<std::wstring>& pythonSearchPaths, bool searchPythonFolderForTest) {
	PyPreConfig preconfig;
	PyPreConfig_InitIsolatedConfig(&preconfig);
	const auto status = Py_PreInitialize(&preconfig);
	if (PyStatus_IsError(status) != 0) {
		LOG_ERROR(raco::log_system::PYTHON, "Py_PreInitialize failed. Error: '{}'", status.err_msg);
		return false;
	}

	const std::string_view pythonFolder = PYTHON_FOLDER;
	const std::string_view pythonVersion = Py_GetVersion();
	auto shippedPythonRoot = (core::PathManager::executableDirectory() / ".." / pythonFolder).normalized();
	if (!shippedPythonRoot.existsDirectory() && searchPythonFolderForTest) {
		// Special case for the tests - search for /release/bin/pythonFolder. Very ugly hack.
		auto exePath = core::PathManager::executableDirectory();
		// The tests use weird dummy directories, first go up to the real directories.
		while (!exePath.exists()) {
			exePath = exePath.parent_path();
		}
		while (exePath.exists() && !shippedPythonRoot.existsDirectory()) {
			shippedPythonRoot = (exePath / "release" / "bin" / pythonFolder).normalized();
			exePath = exePath.parent_path();
		}
	}

#if defined(_WIN32)
	const std::wstring pythonPathDelimiter{L";"};
	std::wstring pythonPaths;
	for (auto& path : pythonSearchPaths) {
		pythonPaths += path + pythonPathDelimiter;
	}
	pythonPaths += L"" + pythonPathDelimiter +
				   shippedPythonRoot.wstring() + pythonPathDelimiter +
				   (shippedPythonRoot / "python38.zip").wstring() + pythonPathDelimiter +
				   (shippedPythonRoot / "lib" / "site-packages").wstring();
	auto pip_prefix = shippedPythonRoot.wstring();
	std::replace(std::begin(pip_prefix), std::end(pip_prefix), L'/', L'\\');
	// It is necessary to set PIP_PREFIX as an environment variable - otherwise pip just uses the default installation directory (Linux) or the executable dir (Windows)
	// as the target directory. See also https://pip.pypa.io/en/stable/topics/configuration/  and https://docs.python.org/3/install/index.html ("How installation works")
	// The environment variable needs to contain backslashes - if it uses slashes, pip gets confused and installs everything into C:xxx instead of C:/xxx.
	LOG_INFO(raco::log_system::COMMON, "Setting environment variable PIP_PREFIX to '{}'", shippedPythonRoot.string());
	if (_wputenv_s(L"PIP_PREFIX", pip_prefix.data()) != 0) {
		LOG_ERROR(raco::log_system::COMMON, "Failed to set PIP prefix. You might get odd errors using pip. errno={}", errno);
		return false;
	}
#else
	const std::wstring pythonPathDelimiter{L":"};
	std::wstring pythonPaths;
	for (auto& path : pythonSearchPaths) {
		pythonPaths += path + pythonPathDelimiter;
	}
	pythonPaths += L"" + pythonPathDelimiter +
				   (shippedPythonRoot / "python3.8").wstring() + pythonPathDelimiter +
				   (shippedPythonRoot / "python3.8" / "lib-dynload").wstring() + pythonPathDelimiter +
				   (shippedPythonRoot / "python3.8" / "lib" / "python3.8" / "site-packages").wstring();
	const auto pip_prefix = (shippedPythonRoot / "python3.8").string();
	// It is necessary to set PIP_PREFIX as an environment variable - otherwise pip just uses the default installation directory (Linux) or the executable dir (Windows)
	// as the target directory. See also https://pip.pypa.io/en/stable/topics/configuration/ and https://docs.python.org/3/install/index.html ("How installation works")
	LOG_INFO(raco::log_system::COMMON, "Setting environment variable PIP_PREFIX to '{}'", (shippedPythonRoot / "python3.8").string());
	if (setenv("PIP_PREFIX", pip_prefix.data(), 1) != 0) {
		LOG_ERROR(raco::log_system::COMMON, "Failed to set PIP prefix. You might get odd errors using pip. errno={}", errno);
		return false;
	}
#endif
	// Documentation of Py_SetProgramName says the buffer specified in the parameter needs to live for the duration of the program -
	// so programname needs to be a static.
	static std::wstring programname = argv0;
	std::string programnameUTF8(1024, 0);
	programnameUTF8.resize(std::wcstombs(programnameUTF8.data(), programname.data(), programnameUTF8.size()));
	LOG_INFO(raco::log_system::PYTHON, "Calling Py_SetProgramName with '{}'", programnameUTF8);
	Py_SetProgramName(programname.data());
	std::string pythonPathsUTF8(1024, 0);
	pythonPathsUTF8.resize(std::wcstombs(pythonPathsUTF8.data(), pythonPaths.data(), pythonPathsUTF8.size()));
	LOG_INFO(raco::log_system::PYTHON, "Calling Py_SetPath with '{}'", pythonPathsUTF8);
	Py_SetPath(pythonPaths.data());

	return true;
}

void setup(raco::application::RaCoApplication* racoApp) {
	::app = racoApp;

	const std::wstring pythonPaths = Py_GetPath();
	std::string pythonPathsUTF8(1024, 0);
	pythonPathsUTF8.resize(std::wcstombs(pythonPathsUTF8.data(), pythonPaths.data(), pythonPathsUTF8.size()));
	LOG_DEBUG(raco::log_system::PYTHON, "Python module search paths: {}", pythonPathsUTF8);

	py::module::import("raco_py_io").attr("hook_stdout")();
}

PythonRunStatus runPythonScript(raco::application::RaCoApplication* app, const std::wstring& applicationPath, const std::string& pythonScriptPath, const std::vector<std::wstring>& pythonSearchPaths, const std::vector<const char*>& pos_argv_cp) {
	currentRunStatus.stdOutBuffer.clear();
	currentRunStatus.stdErrBuffer.clear();

	if (raco::python_api::preparePythonEnvironment(applicationPath, pythonSearchPaths)) {
		py::scoped_interpreter pyGuard{true, static_cast<int>(pos_argv_cp.size()), pos_argv_cp.data()};

		raco::python_api::setup(app);
		currentRunStatus.stdOutBuffer.append(fmt::format("running python script {}\n\n", pythonScriptPath));
		try {
			py::eval_file(pythonScriptPath);
		} catch (py::error_already_set& e) {
			if (e.matches(PyExc_SystemExit)) {
				auto exitCode = py::cast<int>(e.value().attr("code"));
				currentRunStatus.stdErrBuffer.append(fmt::format("Exit called from Python: exit code '{}'\n", exitCode));
				currentRunStatus.exitCode = py::cast<int>(e.value().attr("code"));
				return currentRunStatus;
			} else {
				currentRunStatus.stdErrBuffer.append(fmt::format("Python exception:\n{}", e.what()));
				currentRunStatus.exitCode = 1;
				return currentRunStatus;
			}
		} catch (std::exception& e) {
			currentRunStatus.stdErrBuffer.append(fmt::format("Error thrown in Python script:\n{}", e.what()));
			currentRunStatus.exitCode = 1;
			// TODO exit code
			// how do we get here?
			// need a test
			return currentRunStatus;
		}
	} else {
		currentRunStatus.stdErrBuffer.append("Failed to prepare the Python environment.\n");
		currentRunStatus.exitCode = 1;
		return currentRunStatus;
	}
	currentRunStatus.exitCode = 0;

	return currentRunStatus;
}

}  // namespace raco::python_api
