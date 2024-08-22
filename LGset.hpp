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

#ifndef LGSET_HPP_
#define LGSET_HPP_

#include "LGdef.hpp"

namespace LGset {

	// namespace for setting file operation.
	inline namespace file {
		inline vector<wchar_t> getBuf() {
			vector<wchar_t> buf;
			buf.push_back(VER_BUILD >> 16);
			buf.push_back(VER_BUILD & ((1u << 16) - 1u));
			for(wchar_t wch: userName) buf.push_back(wch);
			buf.push_back(L'\n');
			buf.push_back(enableGodPower);
			buf.push_back(defaultPlayerNum);
			buf.push_back(defaultSpeed);
			buf.push_back(defaultUserId);
			buf.push_back(enableGongSound);
			buf.push_back(L'\n');
			for(wchar_t wch: replayFileName) buf.push_back(wch);
			buf.push_back(L'\n');
			buf.push_back(enableBetaTag);
			buf.push_back(socketPort);
			buf.push_back(L'\n');
			for(wchar_t wch: mainFontName) buf.push_back(wch);
			buf.push_back(L'\n');
			// build 2365+ settings
			buf.push_back(blockMinFontSize);
			buf.push_back(blockMaxFontSize);
			buf.push_back(L'\n');
			buf.push_back(enableAnalysisInGame);
			buf.push_back(gameMode);
			buf.push_back(L'\n');
			buf.push_back(modifier::Leapfrog);
			buf.push_back(modifier::CityState);
			buf.push_back(modifier::MistyVeil);
			buf.push_back(modifier::SilentWar);
			buf.push_back(modifier::SuburbPlain);
			buf.push_back(modifier::DeepSwamp);
			buf.push_back(modifier::NeutralResist);
			buf.push_back(L'\n');
			return buf;
		}
		inline bool check() {
			/* CHECK WHETHER THE SETTING FILE EXISTS */ {
				WIN32_FIND_DATAW FileData;
				HANDLE FileHandle = FindFirstFileW(settingFile.c_str(), &FileData);
				if(FileHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND) {
					HANDLE hFile =
					    CreateFileW(settingFile.c_str(),
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
			HANDLE hFile = CreateFileW(settingFile.c_str(),
			                           GENERIC_READ,
			                           FILE_SHARE_READ,
			                           NULL,
			                           OPEN_EXISTING,
			                           FILE_ATTRIBUTE_HIDDEN,
			                           NULL);
			if(hFile == INVALID_HANDLE_VALUE)
				MessageBoxW(getHWnd(), (L"Open File Failed: CODE " + to_wstring(GetLastError())).c_str(), L"ERROR", MB_OK);
			vector<wchar_t> buf(settingLength * 2);
			DWORD dwReadedSize;
			bool f = ReadFile(hFile,
			                  buf.data(),
			                  settingLength * sizeof(wchar_t),
			                  &dwReadedSize,
			                  NULL);
			if(!f) {
				MessageBoxW(getHWnd(),
				            (L"Reading setting file failed: CODE " + to_wstring(GetLastError())).c_str(),
				            L"ERROR",
				            MB_HELP);
			}
			CloseHandle(hFile);
			int i = 0, rdBuild = 0;
			rdBuild = (buf[0] << 16) | buf[1];
			i = 2;
			userName.clear();
			while(buf[i++] != L'\n') userName.push_back(buf[i - 1]);
			userName.resize(16);
			enableGodPower = buf[i++];
			defaultPlayerNum = buf[i++];
			defaultSpeed = buf[i++];
			defaultUserId = buf[i++];
			enableGongSound = buf[i++];
			buf[i++];  // L'\n'
			replayFileName.clear();
			while(buf[i++] != L'\n') replayFileName.push_back(buf[i - 1]);
			replayFileName.resize(50);
			enableBetaTag = buf[i++];
			socketPort = buf[i++];
			buf[i++];  // L'\n'
			mainFontName.clear();
			while(buf[i++] != L'\n') mainFontName.push_back(buf[i - 1]);
			mainFontName.resize(30);
			if(rdBuild <= 2365) return;  // build 2365+ settings
			blockMinFontSize = buf[i++];
			blockMaxFontSize = buf[i++];
			if(rdBuild <= 2619) return;  // build 2619+ settings
			buf[i++];                    // L'\n'
			enableAnalysisInGame = buf[i++];
			gameMode = buf[i++];
			if(rdBuild <= 2630) return;  // build 2630+ settings
			buf[i++];                    // L'\n'
			modifier::Leapfrog = buf[i++];
			modifier::CityState = buf[i++];
			modifier::MistyVeil = buf[i++];
			modifier::SilentWar = buf[i++];
			if(rdBuild <= 2746) return;  // build 2746+ settings
			modifier::SuburbPlain = buf[i++];
			modifier::DeepSwamp = buf[i++];
			modifier::NeutralResist = buf[i++];
			buf[i++];  // L'\n'
		}
		inline void write() {
			vector<wchar_t> buf = getBuf();
			HANDLE hFile = CreateFileW(settingFile.c_str(),
			                           GENERIC_WRITE,
			                           FILE_SHARE_WRITE,
			                           NULL,
			                           OPEN_ALWAYS,
			                           FILE_ATTRIBUTE_HIDDEN,
			                           NULL);
			if(hFile == INVALID_HANDLE_VALUE)
				MessageBoxW(getHWnd(), (L"Open File Failed: CODE " + to_wstring(GetLastError())).c_str(), L"ERROR", MB_OK);
			SetFilePointer(hFile, 0, 0, FILE_BEGIN);
			DWORD dwWritedDataSize;
			bool f = WriteFile(hFile,
			                   buf.data(),
			                   buf.size() * sizeof(wchar_t),
			                   &dwWritedDataSize,
			                   NULL);
			if(!f) {
				MessageBoxW(getHWnd(),
				            (L"Writing setting file failed: CODE " + to_wstring(GetLastError())).c_str(),
				            L"ERROR",
				            MB_HELP);
			}
			CloseHandle(hFile);
		}
	}  // namespace file

}  // namespace LGset

#endif  // LGSET_HPP_
