/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined(WIN32) && defined(_DEBUG) && defined(PYTHON_DEBUG_LIBRARY_AVAILABLE)
// Needed to avoid pybind11/embed.h to cause linking to the non-debug DLL if the debug DLL is available.
// See https://github.com/pybind/pybind11/issues/3403#issuecomment-962878324
#define Py_DEBUG
#endif

#include <raco_pybind11_embed.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

#include "python_api/PythonAPI.h"

#include "application/RaCoApplication.h"

#include "core/Handles.h"
#include "core/PathManager.h"
#include "core/Queries.h"

#include "user_types/Enumerations.h"

#include <ramses-client-api/AppearanceEnums.h>
#include <ramses-client-api/TextureEnums.h>

#include <spdlog/fmt/fmt.h>

#include <stdlib.h>

namespace py = pybind11;

namespace {
raco::application::RaCoApplication* app;

py::object python_get_scalar_value(raco::core::ValueHandle handle) {
	switch (handle.type()) {
		case raco::data_storage::PrimitiveType::Bool:
			return py::cast(handle.asBool());
			break;
		case raco::data_storage::PrimitiveType::Int:
			if (auto anno = handle.query<raco::core::EnumerationAnnotation>()) {
				switch (static_cast<raco::core::EngineEnumeration>(anno->type_.asInt())) {
					case raco::core::EngineEnumeration::CullMode:
						return py::cast(static_cast<ramses::ECullMode>(handle.asInt()));

					case raco::core::EngineEnumeration::BlendOperation:
						return py::cast(static_cast<ramses::EBlendOperation>(handle.asInt()));

					case raco::core::EngineEnumeration::BlendFactor:
						return py::cast(static_cast<ramses::EBlendFactor>(handle.asInt()));

					case raco::core::EngineEnumeration::DepthFunction:
						return py::cast(static_cast<ramses::EDepthFunc>(handle.asInt()));

					case raco::core::EngineEnumeration::TextureAddressMode:
						return py::cast(static_cast<ramses::ETextureAddressMode>(handle.asInt()));

					case raco::core::EngineEnumeration::TextureMinSamplingMethod:
					case raco::core::EngineEnumeration::TextureMagSamplingMethod:
						return py::cast(static_cast<ramses::ETextureSamplingMethod>(handle.asInt()));

					case raco::core::EngineEnumeration::TextureFormat:
						return py::cast(static_cast<ramses::ETextureFormat>(handle.asInt()));

					case raco::core::EngineEnumeration::RenderBufferFormat:
						return py::cast(static_cast<ramses::ERenderBufferFormat>(handle.asInt()));

					case raco::core::EngineEnumeration::RenderLayerOrder:
						return py::cast(static_cast<raco::user_types::ERenderLayerOrder>(handle.asInt()));

					case raco::core::EngineEnumeration::RenderLayerMaterialFilterMode:
						return py::cast(static_cast<raco::user_types::ERenderLayerMaterialFilterMode>(handle.asInt()));

					default:
						assert(false);
						return py::none();
				}
			} else {
				return py::cast(handle.asInt());
			}
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
			return py::none();
	}
}

void checkProperty(const raco::core::PropertyDescriptor& desc) {
	raco::core::ValueHandle handle{desc};
	if (!handle || handle.constValueRef()->query<raco::core::HiddenProperty>()) {
		throw std::runtime_error(fmt::format("Property '{}' doesn't exist in object '{}'", desc.getPropertyPath(), desc.object()->objectName()));
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
	} else {
		throw std::runtime_error(fmt::format("Set property '{}': can't set complex properties directly.", handle.getPropertyPath()));
	}
}

}  // namespace

PYBIND11_EMBEDDED_MODULE(raco, m) {
	py::enum_<ramses::ECullMode>(m, "ECullMode")
		.value("Disabled", ramses::ECullMode_Disabled)
		.value("Front", ramses::ECullMode_FrontFacing)
		.value("Back", ramses::ECullMode_BackFacing)
		.value("FrontAndBack", ramses::ECullMode_FrontAndBackFacing);

	py::enum_<ramses::EBlendOperation>(m, "EBlendOperation")
		.value("Disabled", ramses::EBlendOperation_Disabled)
		.value("Add", ramses::EBlendOperation_Add)
		.value("Subtract", ramses::EBlendOperation_Subtract)
		.value("ReverseSubtract", ramses::EBlendOperation_ReverseSubtract)
		.value("Min", ramses::EBlendOperation_Min)
		.value("Max", ramses::EBlendOperation_Max);

	py::enum_<ramses::EBlendFactor>(m, "EBlendFactor")
		.value("Zero", ramses::EBlendFactor_Zero)
		.value("One", ramses::EBlendFactor_One)
		.value("SrcAlpha", ramses::EBlendFactor_SrcAlpha)
		.value("OneMinusSrcAlpha", ramses::EBlendFactor_OneMinusSrcAlpha)
		.value("DstAlpha", ramses::EBlendFactor_DstAlpha)
		.value("OneMinusDstAlpha", ramses::EBlendFactor_OneMinusDstAlpha)
		.value("SrcColor", ramses::EBlendFactor_SrcColor)
		.value("OneMinusSrcColor", ramses::EBlendFactor_OneMinusSrcColor)
		.value("DstColor", ramses::EBlendFactor_DstColor)
		.value("OneMinusDstColor", ramses::EBlendFactor_OneMinusDstColor)
		.value("ConstColor", ramses::EBlendFactor_ConstColor)
		.value("OneMinusConstColor", ramses::EBlendFactor_OneMinusConstColor)
		.value("ConstAlpha", ramses::EBlendFactor_ConstAlpha)
		.value("OneMinusConstAlpha", ramses::EBlendFactor_OneMinusConstAlpha)
		.value("AlphaSaturate", ramses::EBlendFactor_AlphaSaturate);

	py::enum_<ramses::EDepthFunc>(m, "EDepthFunction")
		.value("Disabled", ramses::EDepthFunc_Disabled)
		.value("Greater", ramses::EDepthFunc_Greater)
		.value("GreaterEqual", ramses::EDepthFunc_GreaterEqual)
		.value("Less", ramses::EDepthFunc_Less)
		.value("LessEqual", ramses::EDepthFunc_LessEqual)
		.value("Equal", ramses::EDepthFunc_Equal)
		.value("NotEqual", ramses::EDepthFunc_NotEqual)
		.value("Always", ramses::EDepthFunc_Always)
		.value("Never", ramses::EDepthFunc_Never);

	py::enum_<ramses::ETextureAddressMode>(m, "ETextureAddressMode")
		.value("Clamp", ramses::ETextureAddressMode_Clamp)
		.value("Repeat", ramses::ETextureAddressMode_Repeat)
		.value("Mirror", ramses::ETextureAddressMode_Mirror);

	py::enum_<ramses::ETextureSamplingMethod>(m, "ETextureSamplingMethod")
		.value("Nearest", ramses::ETextureSamplingMethod_Nearest)
		.value("Linear", ramses::ETextureSamplingMethod_Linear)
		.value("Nearest_MipMapNearest", ramses::ETextureSamplingMethod_Nearest_MipMapNearest)
		.value("Nearest_MipMapLinear", ramses::ETextureSamplingMethod_Nearest_MipMapLinear)
		.value("Linear_MipMapNearest", ramses::ETextureSamplingMethod_Linear_MipMapNearest)
		.value("Linear_MipMapLinear", ramses::ETextureSamplingMethod_Linear_MipMapLinear);

	py::enum_<ramses::ETextureFormat>(m, "ETextureFormat")
		.value("R8", ramses::ETextureFormat::R8)
		.value("RG8", ramses::ETextureFormat::RG8)
		.value("RGB8", ramses::ETextureFormat::RGB8)
		.value("RGBA8", ramses::ETextureFormat::RGBA8)
		.value("RGB16F", ramses::ETextureFormat::RGB16F)
		.value("RGBA16F", ramses::ETextureFormat::RGBA16F)
		.value("SRGB8", ramses::ETextureFormat::SRGB8)
		.value("SRGB8_ALPHA8", ramses::ETextureFormat::SRGB8_ALPHA8);

	py::enum_<ramses::ERenderBufferFormat>(m, "ERenderBufferFormat")
		.value("RGBA4", ramses::ERenderBufferFormat_RGBA4)
		.value("R8", ramses::ERenderBufferFormat_R8)
		.value("RG8", ramses::ERenderBufferFormat_RG8)
		.value("RGB8", ramses::ERenderBufferFormat_RGB8)
		.value("RGBA8", ramses::ERenderBufferFormat_RGBA8)
		.value("R16F", ramses::ERenderBufferFormat_R16F)
		.value("R32F", ramses::ERenderBufferFormat_R32F)
		.value("RG16F", ramses::ERenderBufferFormat_RG16F)
		.value("RG32F", ramses::ERenderBufferFormat_RG32F)
		.value("RGB16F", ramses::ERenderBufferFormat_RGB16F)
		.value("RGB32F", ramses::ERenderBufferFormat_RGB32F)
		.value("RGBA16F", ramses::ERenderBufferFormat_RGBA16F)
		.value("RGBA32F", ramses::ERenderBufferFormat_RGBA32F)

		.value("Depth24", ramses::ERenderBufferFormat_Depth24)
		.value("Depth24_Stencil8", ramses::ERenderBufferFormat_Depth24_Stencil8);

	py::enum_<raco::user_types::ERenderLayerOrder>(m, "ERenderLayerOrder")
		.value("Manual", raco::user_types::ERenderLayerOrder::Manual)
		.value("SceneGraph", raco::user_types::ERenderLayerOrder::SceneGraph);

	py::enum_<raco::user_types::ERenderLayerMaterialFilterMode>(m, "ERenderLayerMaterialFilterMode")
		.value("Inclusive", raco::user_types::ERenderLayerMaterialFilterMode::Inclusive)
		.value("Exclusive", raco::user_types::ERenderLayerMaterialFilterMode::Exclusive);


	m.def("load", [](std::string path) {
		if (!path.empty()) {
			try {
				app->switchActiveRaCoProject(QString::fromStdString(path), false);
			} catch (std::exception& e) {
				app->switchActiveRaCoProject(QString(), false);
				throw e;
			}
		} else {
			app->switchActiveRaCoProject(QString(), false);
			throw std::runtime_error("Load project: file name is empty.");
		}
	});

	m.def("reset", []() {
		app->switchActiveRaCoProject(QString(), false);
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

	m.def("projectPath", [](){
		return app->activeProjectPath();
	});
	
	m.def("externalProjects", []() {
		py::list externalPaths;
		for (auto const & [ id, info ] : app->activeRaCoProject().project()->externalProjectsMap()) {
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

	py::class_<raco::core::PropertyDescriptor>(m, "PropertyDescriptor")
		.def("__repr__", [](const raco::core::PropertyDescriptor& desc) {
			auto handle = raco::core::ValueHandle(desc);
			return fmt::format("<Property[{}]: '{}'>", handle.constValueRef()->baseTypeName(), desc.getPropertyPath());
		})
		.def("__dir__", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			auto handle = raco::core::ValueHandle(desc);
			std::vector<std::string> propNames;
			if (handle && handle.hasSubstructure()) {
				for (size_t index = 0; index < handle.size(); index++) {
					propNames.emplace_back(handle[index].getPropName());
				}
			}
			return py::cast(propNames);
		})
		.def("__getattr__", [](const raco::core::PropertyDescriptor& desc, const std::string& name) -> py::object {
			auto childDesc = desc.child(name);
			checkProperty(childDesc);
			return py::cast(childDesc);
		})
		.def("__setattr__", [](const raco::core::PropertyDescriptor& desc, const std::string& name, py::object value) {
			python_set_value(desc.child(name), value);
		})
		.def(py::self == py::self)
		.def("object", &raco::core::PropertyDescriptor::object)
		.def("typeName", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			auto handle = raco::core::ValueHandle(desc);
			if (handle) {
				return py::cast(handle.constValueRef()->baseTypeName());
			}
			return py::none();
		})
		.def("propName", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			auto handle = raco::core::ValueHandle(desc);
			if (handle) {
				return py::cast(handle.getPropName());
			}
			return py::none();
		})
		.def("value", [](const raco::core::PropertyDescriptor& desc) -> py::object {
			auto handle = raco::core::ValueHandle(desc);
			if (handle && !handle.hasSubstructure()) {
				return python_get_scalar_value(handle);
			}
			return py::none();
		});

	py::class_<raco::core::EditorObject, raco::core::SEditorObject>(m, "EditorObject")
		.def("__repr__", [](const raco::core::EditorObject& obj) {
			return fmt::format("<{}: '{}'>", obj.getTypeDescription().typeName, obj.objectName());
		})
		.def("__dir__", [](raco::core::SEditorObject obj) {
			std::vector<std::string> propNames;
			for (size_t index = 0; index < obj->size(); index++) {
				if (!obj->get(index)->query<raco::core::HiddenProperty>()) {
					propNames.emplace_back(obj->name(index));
				}
			}
			return py::cast(propNames);
		})
		.def("__getattr__", [](raco::core::SEditorObject obj, std::string name) -> py::object {
			raco::core::PropertyDescriptor desc(obj, {name});
			checkProperty(desc);
			return py::cast(desc);
		})
		.def("__setattr__", [](raco::core::SEditorObject obj, std::string name, py::object value) {
			python_set_value({obj, {name}}, value);
		})
		.def("typeName", [](raco::core::SEditorObject obj) {
			return obj->getTypeDescription().typeName;
		})
		.def("children", [](raco::core::SEditorObject obj) {
			return obj->children_->asVector<raco::core::SEditorObject>();
		})
		.def("parent", &raco::core::EditorObject::getParent)
		.def("objectID", &raco::core::EditorObject::objectID);

	py::class_<raco::core::LinkDescriptor>(m, "LinkDescriptor")
		.def("__repr__", [](const raco::core::LinkDescriptor& desc) {
			return fmt::format("<Link: start='{}' end='{}' valid='{}'>", desc.start.getPropertyPath(), desc.end.getPropertyPath(), desc.isValid);
		})
		.def(py::self == py::self)
		.def_readonly("start", &raco::core::LinkDescriptor::start)
		.def_readonly("end", &raco::core::LinkDescriptor::end)
		.def_readonly("valid", &raco::core::LinkDescriptor::isValid);

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
		if (!app->activeRaCoProject().project()->isInstance(end.object())) {
			throw std::runtime_error(fmt::format("getLink: object '{}' not in project", end.object()->objectName()));
		}
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

	m.def("removeLink", [](const raco::core::PropertyDescriptor& end) {
		app->activeRaCoProject().commandInterface()->removeLink(end);
		app->doOneLoop();
	});
}

namespace raco::python_api {

bool preparePythonEnvironment(std::wstring argv0, bool searchPythonFolderForTest) {
	PyPreConfig preconfig;
	PyPreConfig_InitIsolatedConfig(&preconfig);
	const auto status = Py_PreInitialize(&preconfig);
	if (PyStatus_IsError(status) != 0) {
		LOG_ERROR(raco::log_system::COMMON, "Py_PreInitialize failed. Error: '{}'", status.err_msg);
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
	const static std::wstring pythonPaths {
		L"" + pythonPathDelimiter +
		shippedPythonRoot.wstring() + pythonPathDelimiter +
		(shippedPythonRoot / "python38.zip").wstring() + pythonPathDelimiter +
		(shippedPythonRoot / "lib" / "site-packages").wstring()
	};
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
	const std::wstring pythonPaths {
		L"" + pythonPathDelimiter +
		(shippedPythonRoot / "python3.8").wstring() + pythonPathDelimiter +
		(shippedPythonRoot / "python3.8" / "lib-dynload").wstring() + pythonPathDelimiter +
		(shippedPythonRoot / "python3.8" / "lib" / "python3.8" / "site-packages").wstring()
	};
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
	LOG_INFO(raco::log_system::COMMON, "Calling Py_SetProgramName with '{}'", programnameUTF8);
	Py_SetProgramName(programname.data());
	std::string pythonPathsUTF8(1024, 0);
	pythonPathsUTF8.resize(std::wcstombs(pythonPathsUTF8.data(), pythonPaths.data(), pythonPathsUTF8.size()));
	LOG_INFO(raco::log_system::COMMON, "Calling Py_SetPath with '{}'", pythonPathsUTF8);
	Py_SetPath(pythonPaths.data());

	return true;
}

void setup(raco::application::RaCoApplication* racoApp) {
	::app = racoApp;

	const std::wstring pythonPaths = Py_GetPath();
	std::string pythonPathsUTF8(1024, 0);
	pythonPathsUTF8.resize(std::wcstombs(pythonPathsUTF8.data(), pythonPaths.data(), pythonPathsUTF8.size()));
	LOG_INFO(raco::log_system::COMMON, "Python module search paths: {}", pythonPathsUTF8);
}

}  // namespace raco::python_api
