/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#if (defined(__WIN32) || defined(_WIN32))

#include "utils/CrashDump.h"

#include "strsafe.h"

#include <Windows.h>
#include <Dbghelp.h>
#include <crtdbg.h>

/*
* This code is derived from the following article on stack overflow
*  http://stackoverflow.com/questions/5028781/how-to-write-a-sample-code-that-will-crash-and-produce-dump-file
*/

namespace raco::utils::crashdump {

#define MAX_DUMP_PATH 10000

void create_minidump(EXCEPTION_POINTERS* e) {
	auto hDbgHelp = LoadLibraryA("dbghelp");
	if (hDbgHelp == nullptr)
		return;
	auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
	if (pMiniDumpWriteDump == nullptr)
		return;

	char dumpFileName[MAX_DUMP_PATH];
	auto nameEnd = dumpFileName + GetModuleFileNameA(GetModuleHandleA(0), dumpFileName, MAX_DUMP_PATH);
	SYSTEMTIME t;
	GetSystemTime(&t);
	StringCbPrintfA(
		nameEnd - strlen(".exe"),
		MAX_DUMP_PATH,
		"_%d.%d.%d_%4d%02d%02d_%02d%02d%02d.dmp",
		RACO_VERSION_MAJOR, RACO_VERSION_MINOR, RACO_VERSION_PATCH,
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

	auto hFile = CreateFileA(dumpFileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	auto dumped = pMiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
		e ? &exceptionInfo : nullptr,
		nullptr,
		nullptr);

	CloseHandle(hFile);

	return;
}

LONG CALLBACK minidump_exception_handler(EXCEPTION_POINTERS* e) {
	create_minidump(e);
	return EXCEPTION_CONTINUE_SEARCH;
}

void installCrashDumpHandler(bool noDumpFiles) {
	// Do not show "RaCoHeadless.exe stopped working" dialog
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#if defined(_DEBUG)
	if (!IsDebuggerPresent()) {
		// Do not pop up assert dialog
		_set_error_mode(_OUT_TO_STDERR);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	}
#endif
	
	if (!noDumpFiles) {
		SetUnhandledExceptionFilter(minidump_exception_handler);
	}
}

}  // namespace raco

#else

namespace raco::utils::crashdump {

void installCrashDumpHandler(bool noDumpFiles) {
}

}

#endif