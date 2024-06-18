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

namespace raco::application {
class RaCoApplication;
}

namespace raco::python_api {

struct PythonRunStatus {
	int exitCode;
	std::string stdOutBuffer;
	std::string stdErrBuffer;
};

std::string getPythonVersion();
bool preparePythonEnvironment(std::wstring argv0, const std::vector<std::wstring>& pythonSearchPaths, bool searchPythonFolderForTest = false);
void setApp(application::RaCoApplication* racoApp);
bool importRaCoModule();
bool importCompleter();

bool initializeInterpreter(application::RaCoApplication* racoApp, const std::wstring& racoAppPath, const std::vector<std::wstring>& pythonSearchPaths, const std::vector<std::string>& cmdLineArgs);
void finalizeInterpreter();

PythonRunStatus runPythonScript(const std::string& pythonScript);
PythonRunStatus runPythonScriptFromFile(const std::string& pythonScriptPath);

std::vector<std::string> getCompletions(const std::string& prefix);
bool isCompleteCommand(const std::string& command);

}
