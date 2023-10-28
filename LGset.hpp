/* This is LGset.hpp file of LocalGen.                                   */
/* Copyright (c) 2023 SZXC Work Group; All rights reserved.              */
/* Developers: http://github.com/SZXC-WG                                 */
/* Project: http://github.com/SZXC-WG/LocalGen-new                       */
/*                                                                       */
/* This project is licensed under the MIT license. That means you can    */
/* download, use and share a copy of the product of this project. You    */
/* may modify the source code and make contribution to it too. But, you  */
/* must print the copyright information at the front of your product.    */
/*                                                                       */
/* The full MIT license this project uses can be found here:             */
/* http://github.com/SZXC-WG/LocalGen-new/blob/main/LICENSE.md           */

#ifndef __LGSET_HPP__
#define __LGSET_HPP__

#include "LGdef.hpp"

namespace LGset {

	// namespace for setting file operation.
	inline namespace file {
		inline vector<wchar_t> getBuf() {
			vector<wchar_t> buf;
			buf.push_back(VER_BUILD>>16);
			buf.push_back(VER_BUILD&((1u<<16)-1u));
			for(wchar_t wch : userName) buf.push_back(wch);
			buf.push_back(L'\n');
			buf.push_back(enableGodPower);
			buf.push_back(defaultPlayerNum);
			buf.push_back(defaultSpeed);
			buf.push_back(defaultUserId);
			buf.push_back(enableGongSound);
			buf.push_back(L'\n');
			for(wchar_t wch : replayFileName) buf.push_back(wch);
			buf.push_back(L'\n');
			buf.push_back(enableBetaTag);
			buf.push_back(socketPort);
			buf.push_back(L'\n');
			for(wchar_t wch : mainFontName) buf.push_back(wch);
			buf.push_back(L'\n');
			buf.push_back(blockMinFontSize);
			buf.push_back(blockMaxFontSize);
			return buf;
		}
		inline bool check() {
			/* CHECK WHETHER THE SETTING FILE EXISTS */ {
				WIN32_FIND_DATAA FileData;
				HANDLE FileHandle = FindFirstFileA(settingFile.c_str(),&FileData);
				if(FileHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND) {
					HANDLE hFile =
					    CreateFileA(settingFile.c_str(),
					                0,
					                0,
					                NULL,
					                CREATE_ALWAYS,
					                FILE_ATTRIBUTE_HIDDEN,
					                NULL);
					CloseHandle(hFile);
					return true;
				} else CloseHandle(FileHandle);
			}
			return false;
		}
		inline void read() {
			if(settingLength == 0) settingLength = getBuf().size();
			if(check()) write();
			HANDLE hFile = CreateFileA(settingFile.c_str(),
			                           GENERIC_READ,
			                           FILE_SHARE_READ,
			                           NULL,
			                           OPEN_EXISTING,
			                           FILE_ATTRIBUTE_HIDDEN,
			                           NULL);
			if(hFile == INVALID_HANDLE_VALUE)
				MessageBoxW(GetConsoleWindow(),(L"Open File Failed: CODE "+to_wstring(GetLastError())).c_str(),L"ERROR",MB_OK);
			vector<wchar_t> buf(settingLength*2);
			DWORD dwReadedSize;
			bool f = ReadFile(hFile,
			                  buf.data(),
			                  settingLength * sizeof(wchar_t),
			                  &dwReadedSize,
			                  NULL);
			if(!f) {
				MessageBoxW(GetConsoleWindow(),
				            (L"Reading setting file failed: CODE " + to_wstring(GetLastError())).c_str(),
				            L"ERROR",
				            MB_HELP);
			}
			CloseHandle(hFile);
			int i=0,rdBuild=0;
			rdBuild=(buf[0]<<16)|buf[1];
			i=2;
			userName.clear();
			while(buf[i++]!=L'\n') userName.push_back(buf[i-1]);
			userName.resize(16);
			enableGodPower = buf[i++];
			defaultPlayerNum = buf[i++];
			defaultSpeed = buf[i++];
			defaultUserId = buf[i++];
			enableGongSound = buf[i++];
			buf[i++]; // L'\n'
			replayFileName.clear();
			while(buf[i++]!=L'\n') replayFileName.push_back(buf[i-1]);
			replayFileName.resize(50);
			enableBetaTag = buf[i++];
			socketPort = buf[i++];
			buf[i++]; // L'\n'
			mainFontName.clear();
			while(buf[i++]!=L'\n') mainFontName.push_back(buf[i-1]);
			mainFontName.resize(30);
			// settings before build 2365 (inclusive)
			if(rdBuild<=2365) return; // build 2365 settings
			blockMinFontSize = buf[i++];
			blockMaxFontSize = buf[i++];
		}
		inline void write() {
			vector<wchar_t> buf = getBuf();
			HANDLE hFile = CreateFileA(settingFile.c_str(),
			                           GENERIC_WRITE,
			                           FILE_SHARE_WRITE,
			                           NULL,
			                           OPEN_ALWAYS,
			                           FILE_ATTRIBUTE_HIDDEN,
			                           NULL);
			if(hFile == INVALID_HANDLE_VALUE)
				MessageBoxW(GetConsoleWindow(),(L"Open File Failed: CODE "+to_wstring(GetLastError())).c_str(),L"ERROR",MB_OK);
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
			DWORD dwWritedDataSize;
			bool f = WriteFile(hFile,
			                   buf.data(),
			                   buf.size() * sizeof(wchar_t),
			                   &dwWritedDataSize,
			                   NULL);
			if(!f) {
				MessageBoxW(GetConsoleWindow(),
				            (L"Writing setting file failed: CODE " + to_wstring(GetLastError())).c_str(),
				            L"ERROR",
				            MB_HELP);
			}
			CloseHandle(hFile);
		}
	} // inline namespace file

} // namespace LGset

#endif // __LGSET_HPP__
