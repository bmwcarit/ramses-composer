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

#include <raco_pybind11_embed.h>

#include <gtest/gtest.h>

#include "testing/RaCoApplicationTest.h"

#include "ramses_adaptor/SceneBackend.h"
#include "application/RaCoApplication.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "python_api/PythonAPI.h"

namespace py = pybind11;

using raco::application::RaCoApplication;


class PythonTest : public RaCoApplicationTest {
public:
	PythonTest() : RaCoApplicationTest() {
		raco::python_api::preparePythonEnvironment(QCoreApplication::applicationFilePath().toStdWString(), {}, true);
		pyGuard = std::make_unique<py::scoped_interpreter>();
		raco::python_api::setup(&application);
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
raco.export("empty.ramses", "empty.rlogic", False)
)");
}

TEST_F(PythonTest, export_single) {
	py::exec(R"(	
import raco
raco.load(")" + (test_path() / "example_scene.rca").string() + R"(")
raco.export("test.ramses", "test.rlogic", False)
)");
}

TEST_F(PythonTest, export_multi) {
	py::exec(R"(	
import raco
raco.load(")" + (test_path() / "example_scene.rca").string() + R"(")
raco.export("test-duck.ramses", "test-duck.rlogic", False)

raco.load(")" + (test_path() / "empty.rca").string() + R"(")
raco.export("test-empty.ramses", "test-empty.rlogic", False)
)");
}

TEST_F(PythonTest, exception_throw_python) {
	EXPECT_THROW(py::exec(R"(
raise RunTimeError("something wrong here!")
)"), std::runtime_error);
}
