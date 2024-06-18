/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <raco_pybind11_embed.h>

#include <gtest/gtest.h>

#include "testing/RaCoApplicationTest.h"

#include "application/RaCoApplication.h"
#include "python_api/PythonAPI.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "testing/TestUtil.h"

#include "user_types/Node.h"

namespace py = pybind11;

using raco::application::RaCoApplication;

class PythonTest : public RaCoApplicationTest {
public:
	PythonTest() : RaCoApplicationTest() {
		python_api::preparePythonEnvironment(QCoreApplication::applicationFilePath().toStdWString(), {}, true);
		pyGuard = std::make_unique<py::scoped_interpreter>();
		raco::python_api::setApp(&application);
		raco::python_api::importRaCoModule();
	}

	std::unique_ptr<py::scoped_interpreter> pyGuard;
};

TEST_F(PythonTest, hello_world) {
	py::exec(R"(	
import raco
print("Hello world from Python!")
)");
}

TEST_F(PythonTest, export_empty) {
	py::exec(R"(	
import raco
raco.export("empty.ramses", False)
)");
}

TEST_F(PythonTest, export_single) {
	py::exec(R"(	
import raco
raco.load(")" + (test_path() / "example_scene.rca").string() + R"(")
raco.export("test.ramses", False)
)");
}

TEST_F(PythonTest, export_multi) {
	py::exec(R"(	
import raco
raco.load(")" + (test_path() / "example_scene.rca").string() + R"(")
raco.export("test-duck.ramses", False)

raco.load(")" + (test_path() / "empty.rca").string() + R"(")
raco.export("test-empty.ramses", False)
)");
}

TEST_F(PythonTest, exception_throw_python) {
	EXPECT_THROW(py::exec(R"(
raise RunTimeError("something wrong here!")
)"),
		std::runtime_error);
}

TEST_F(PythonTest, separate_commands) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	py::exec(R"(
import raco
raco.create("Node", "node_1")
raco.create("Node", "node_2")
)");
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 2);
}

TEST_F(PythonTest, composite_command) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	py::exec(R"(
import raco
with raco.compositeCommand("test"):
	raco.create("Node", "node_1")
	raco.create("Node", "node_2")
)");
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 1);
}

TEST_F(PythonTest, composite_command_nested) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	py::exec(R"(
import raco
with raco.compositeCommand("outer"):
	with raco.compositeCommand("inner"):
		raco.create("Node", "node_1")
	raco.create("Node", "node_2")
)");
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 1);
}

TEST_F(PythonTest, composite_command_exception) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	try {
		py::exec(R"(
import raco
with raco.compositeCommand("test"):
	raco.create("Node", "node_1")
	raco.create("Node", "node_2")
	raise RuntimeError("error")
)");
	} catch (std::exception&) {
	}
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 0);
}

TEST_F(PythonTest, composite_command_nested_exception_inner) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	try {
		py::exec(R"(
import raco
with raco.compositeCommand("outer"):
	with raco.compositeCommand("inner"):
		raco.create("Node", "node_1")
		raise RuntimeError("error")
	raco.create("Node", "node_2")
)");
	} catch (std::exception&) {
	}
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 0);
}

TEST_F(PythonTest, composite_command_nested_exception_outer) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	try {
		py::exec(R"(
import raco
with raco.compositeCommand("outer"):
	with raco.compositeCommand("inner"):
		raco.create("Node", "node_1")
	raco.create("Node", "node_2")
	raise RuntimeError("error")
)");
	} catch (std::exception&) {
	}
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(end_index - start_index, 0);
}

TEST_F(PythonTest, composite_command_nested_exception_inner_caught) {
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	try {
		py::exec(R"(
import raco
with raco.compositeCommand("outer"):
	try:
		with raco.compositeCommand("inner"):
			raco.create("Node", "node_1")
			raise RuntimeError("error")
	except:
		pass
	raco.create("Node", "node_2")
)");
	} catch (std::exception&) {
	}
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();

	EXPECT_EQ(end_index - start_index, 1);
	EXPECT_TRUE(raco::select<user_types::Node>(project().instances(), "node_1") != nullptr);
	EXPECT_TRUE(raco::select<user_types::Node>(project().instances(), "node_2") != nullptr);
}

TEST_F(PythonTest, composite_command_fail_reset) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.reset()
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}

TEST_F(PythonTest, composite_command_fail_reset_with_feature_level) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.reset(1)
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}

TEST_F(PythonTest, composite_command_fail_load) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.load("nosuchfile")
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}

TEST_F(PythonTest, composite_command_fail_load_with_feature_level) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.load("nosuchfile", 1)
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}

TEST_F(PythonTest, composite_command_fail_save) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.save("nosuchfile")
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}

TEST_F(PythonTest, composite_command_fail_save_with_newids) {
	std::string script = R"(
import raco
with raco.compositeCommand("test"):
	raco.save("nosuchfile", True)
)";
	auto start_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_THROW(py::exec(script), std::runtime_error);
	auto end_index = application.activeRaCoProject().undoStack()->getIndex();
	EXPECT_EQ(start_index, end_index);
}
