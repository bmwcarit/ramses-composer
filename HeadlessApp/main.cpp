/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Needs to be first
#include <raco_pybind11_embed.h>

#include "application/RaCoApplication.h"
#include "components/DataChangeDispatcher.h"
#include "components/RaCoNameConstants.h"
#include "core/PathManager.h"
#include "core/ProjectMigration.h"
#include "log_system/log.h"
#include "ramses_adaptor/SceneBackend.h"
#include "ramses_base/HeadlessEngineBackend.h"
#include "utils/CrashDump.h"
#include "utils/u8path.h"

#include "python_api/PythonAPI.h"

#include <QCoreApplication>
#include <QTimer>
#include <iostream>

namespace py = pybind11;

class Worker : public QObject {
	Q_OBJECT

public:
	Worker(QObject* parent, QString& projectFile, QString& exportPath, QString& pythonScriptPath, bool compressExport, QStringList positionalArguments)
		: QObject(parent), projectFile_(projectFile), exportPath_(exportPath), pythonScriptPath_(pythonScriptPath), compressExport_(compressExport), positionalArguments_(positionalArguments) {
	}

public Q_SLOTS:
	void run() {
		raco::ramses_base::HeadlessEngineBackend backend{};
		std::unique_ptr<raco::application::RaCoApplication> app;

		try {
			app = std::make_unique<raco::application::RaCoApplication>(backend, raco::application::RaCoApplicationLaunchSettings{projectFile_, false, true});
		} catch (const raco::application::FutureFileVersion& error) {
			LOG_ERROR(raco::log_system::COMMON, "File load error: project file was created with newer file version {} but current file version is {}.", error.fileVersion_, raco::serialization::RAMSES_PROJECT_FILE_VERSION);
			app.reset();
			exitCode_ = 1;
		} catch (const raco::core::ExtrefError& error) {
			LOG_ERROR(raco::log_system::COMMON, "File Load Error: external reference update failed with error {}.", error.what());
			app.reset();
			exitCode_ = 1;
		} catch (const std::exception& error) {
			LOG_ERROR(raco::log_system::COMMON, "File Load Error: {}", error.what());
			app.reset();
			exitCode_ = 1;
		}

		if (app) {
			if (!pythonScriptPath_.isEmpty()) {
				std::vector<std::string> pos_argv_s;
				pos_argv_s.emplace_back(pythonScriptPath_.toStdString());
				for (auto arg : positionalArguments_) {
					pos_argv_s.emplace_back(arg.toStdString());
				}
				std::vector<const char*> pos_argv_cp;
				for (auto& s : pos_argv_s) {
					pos_argv_cp.emplace_back(s.c_str());
				}

				if (raco::python_api::preparePythonEnvironment(QCoreApplication::applicationFilePath().toStdWString())) {
					py::scoped_interpreter pyGuard{true, static_cast<int>(pos_argv_cp.size()), pos_argv_cp.data()};

					raco::python_api::setup(app.get());
					LOG_INFO(raco::log_system::COMMON, "running python script {}", pythonScriptPath_.toStdString());
					try {
						py::eval_file(pythonScriptPath_.toStdString().c_str());
					} catch (py::error_already_set& e) {
						if (e.matches(PyExc_SystemExit)) {
							exitCode_ = py::cast<int>(e.value().attr("code"));
							LOG_ERROR(raco::log_system::COMMON, "Exit called from Python: exit code '{}'", exitCode_);
						} else {
							LOG_ERROR(raco::log_system::COMMON, "Python exception:\n{}", e.what());
							exitCode_ = 1;
						}
					} catch (std::exception& e) {
						LOG_ERROR(raco::log_system::COMMON, "Error thrown in Python script:\n{}", e.what());
						// TODO exit code
						// how do we get here?
						// need a test
					}
				} else {
					LOG_ERROR(raco::log_system::COMMON, "Failed to prepare the Python environment.");
					exitCode_ = 1;
				}
			} else if (!exportPath_.isEmpty()) {
				QString ramsesPath = exportPath_ + "." + raco::names::FILE_EXTENSION_RAMSES_EXPORT;
				QString logicPath = exportPath_ + "." + raco::names::FILE_EXTENSION_LOGIC_EXPORT;

				std::string error;
				if (!app->exportProject(ramsesPath.toStdString(), logicPath.toStdString(), compressExport_, error)) {
					LOG_ERROR(raco::log_system::COMMON, "error exporting to {}\n{}", error.c_str(), ramsesPath.toStdString());
					exitCode_ = 1;
				}
			}
		}

		Q_EMIT finished(exitCode_);
	}

Q_SIGNALS:
	void finished(int returnCode);

private:
	QString projectFile_;
	QString exportPath_;
	QString& pythonScriptPath_;
	bool compressExport_;
	QStringList positionalArguments_;
	int exitCode_ = 0;
};

#include "main.moc"

spdlog::level::level_enum getLevelFromArg(const QString& arg) {
	bool logLevelValid;
	int logLevel = arg.toInt(&logLevelValid);
	spdlog::level::level_enum spdLogLevel;
	switch (logLevelValid ? logLevel : -1) {
		case 0:
			return spdlog::level::level_enum::off;
		case 1:
			return spdlog::level::level_enum::critical;
		case 2:
			return spdlog::level::level_enum::err;
		case 3:
			return spdlog::level::level_enum::warn;
		case 4:
			return spdlog::level::level_enum::info;
		case 5:
			return spdlog::level::level_enum::debug;
		case 6:
			return spdlog::level::level_enum::trace;
		default:
			LOG_WARNING(raco::log_system::COMMON, "Invalid Log Level: \"{}\". Continuing with verbose log output.", arg.toStdString().c_str());
			return spdlog::level::level_enum::trace;
	}
}

int main(int argc, char* argv[]) {
	QCoreApplication::setApplicationName("Ramses Composer Headless");
	QCoreApplication::setApplicationVersion(RACO_OSS_VERSION);

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.setApplicationDescription("Ramses Composer Headless for command line use");

	QCommandLineOption loadProjectAction(
		QStringList() << "p"
					  << "project",
		"Load a scene from specified path.",
		"project-path");
	QCommandLineOption exportProjectAction(
		QStringList() << "e"
					  << "export",
		"Export Ramses scene and logic to path. File extensions are added automatically (ignored if '-r' is used).",
		"export-path");
	QCommandLineOption compressExportAction(
		QStringList() << "c"
					  << "compress",
		"Compress Ramses scene on export (ignored if '-r' is used).");
	QCommandLineOption noDumpFileCheckOption(
		QStringList() << "d"
					  << "nodump",
		"Don't generate crash dumps on unhandled exceptions.");
	QCommandLineOption logLevelOption(
		QStringList() << "l"
					  << "loglevel",
		"Maximum information level that should be printed as console log output. Possible options: 0 (off), 1 (critical), 2 (error), 3 (warn), 4 (info), 5 (debug), 6 (trace).",
		"log-level",
		"6");
	QCommandLineOption pyrunOption(
		QStringList() << "r"
					  << "run",
		"Run Python script.",
		"script-path");
	QCommandLineOption logFileOutputOption(
		QStringList() << "o"
					  << "outlogfile",
		"File name to write log file to.",
		"log-file-name",
		"");
	parser.addOption(loadProjectAction);
	parser.addOption(exportProjectAction);
	parser.addOption(compressExportAction);
	parser.addOption(noDumpFileCheckOption);
	parser.addOption(logLevelOption);
	parser.addOption(pyrunOption);
	parser.addOption(logFileOutputOption);

	// application must be instantiated before parsing command line
	QCoreApplication a(argc, argv);

	parser.process(QCoreApplication::arguments());

	bool noDumpFiles = parser.isSet(noDumpFileCheckOption);
	raco::utils::crashdump::installCrashDumpHandler(noDumpFiles);

	auto appDataPath = raco::utils::u8path(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString()).parent_path() / "RamsesComposer";
	raco::core::PathManager::init(QCoreApplication::applicationDirPath().toStdString(), appDataPath);


	auto customLogFileName = parser.value(logFileOutputOption).toStdString();
	auto logFilePath = raco::utils::u8path(customLogFileName);
	bool customLogFileNameFailed = false;

	if (!customLogFileName.empty()) {
		if (!logFilePath.parent_path().empty() && logFilePath.parent_path() != logFilePath && !logFilePath.parent_path().exists()) {
			std::filesystem::create_directories(logFilePath.parent_path());

			if (!logFilePath.parent_path().exists()) {
				logFilePath = raco::core::PathManager::logFileHeadlessName();
				customLogFileNameFailed = true;
			}
		}
	} else {
		logFilePath = raco::core::PathManager::logFileHeadlessName();
	}

	raco::log_system::init(logFilePath.internalPath().native());
	if (customLogFileNameFailed) {
		// TODO why is this an error: we seem to be able to continue so make this a warning instead!?
		// TODO this error only appears in log file but not on stdout if invoked from command line!! why??
		LOG_ERROR(raco::log_system::LOGGING, "Could not create log file at: " + customLogFileName + ". Using default location instead: " + logFilePath.string());
	}

	auto logLevel = getLevelFromArg(parser.value(logLevelOption));
	raco::log_system::setConsoleLogLevel(logLevel);
	raco::ramses_base::setRamsesLogLevel(logLevel);
	raco::ramses_base::setLogicLogLevel(logLevel);

	QString projectFile{};
	if (parser.isSet(loadProjectAction)) {
		QFileInfo path(parser.value(loadProjectAction));
		projectFile = path.absoluteFilePath();
	}

	QString exportPath{};
	bool compressExport = parser.isSet(compressExportAction);
	if (parser.isSet(exportProjectAction)) {
		QFileInfo path(parser.value(exportProjectAction));

		exportPath = path.absoluteFilePath();
		if (path.suffix().compare(raco::names::FILE_EXTENSION_RAMSES_EXPORT, Qt::CaseInsensitive) == 0) {
			exportPath.chop(static_cast<int>(strlen(raco::names::FILE_EXTENSION_RAMSES_EXPORT) + 1));
		} else if (path.suffix().compare(raco::names::FILE_EXTENSION_LOGIC_EXPORT, Qt::CaseInsensitive) == 0) {
			exportPath.chop(static_cast<int>(strlen(raco::names::FILE_EXTENSION_LOGIC_EXPORT) + 1));
		}
	}

	QString pythonScriptPath{};
	if (parser.isSet(pyrunOption)) {
		QFileInfo path(parser.value(pyrunOption));
		if (path.exists()) {
			if (raco::utils::u8path(path.filePath().toStdString()).userHasReadAccess()) {
				pythonScriptPath = path.absoluteFilePath();
			} else {
				LOG_ERROR(raco::log_system::COMMON, "Python script file could not be read {}", path.filePath().toStdString());
				// TODO needs test
				exit(1);
			}
		} else {
			LOG_ERROR(raco::log_system::COMMON, "Python script file not found {}", path.filePath().toStdString());
			exit(1);
		}
	}

	LOG_INFO(raco::log_system::COMMON, "positional arguments = {}", parser.positionalArguments().join(", ").toStdString());

	Worker* task = new Worker(&a, projectFile, exportPath, pythonScriptPath, compressExport, parser.positionalArguments());
	QObject::connect(task, &Worker::finished, &QCoreApplication::exit);
	QTimer::singleShot(0, task, &Worker::run);

	return a.exec();
}
