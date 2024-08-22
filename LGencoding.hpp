/* This is LGencoding.hpp file of LocalGen.                              */
/* Copyright (c) 2024 SZXC Work Group; All rights reserved.              */
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

#ifndef LGENCODING_HPP_
#define LGENCODING_HPP_

#include "LGdef.hpp"

std::wstring wcharTransfer(const wstring& ws) {
	std::wstring res;
	for(int i = 0; i < ws.size(); ++i) {
		if(ws[i] >> 7) {
			int c = 1;
			while(ws[i] >> (8 - c - 1) & 1) ++c;
			wchar_t ret = ws[i] & ((1 << (8 - c)) - 1);
			for(int j = i + 1; j < i + c; ++j) ret = (ret << 6) | (ws[j] & ((1 << 6) - 1));
			res.push_back(ret);
			i = i + c - 1;
		} else res.push_back(ws[i]);
	}
	return res;
}

#endif  // LGENCODING_HPP_
