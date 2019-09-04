// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//*****************************************************************************
// CorHost.cpp
//
// Implementation for the meta data dispenser code.
//

//*****************************************************************************

#include "common.h"
#include "corbundle.h"
#include <utilcode.h>
#include <corhost.h>

static LPCSTR UnicodeToUtf8(LPCWSTR str)
{
    STANDARD_VM_CONTRACT;

    int length = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, 0, 0);
    _ASSERTE(length != 0);

    LPSTR result = new (nothrow) CHAR[length];
    _ASSERTE(result != NULL);

    length = WideCharToMultiByte(CP_UTF8, 0, str, -1, result, length, 0, 0);
    _ASSERTE(length != 0);

    return result;
}

BundleInfo::BundleInfo(LPCWSTR bundlePath, bool(*probe)(LPCSTR, INT64*, INT64*))
{
    LIMITED_METHOD_CONTRACT;
	
    m_path = bundlePath;
    m_probe = probe;
}

LPCWSTR BundleInfo::Path() const
{ 
    LIMITED_METHOD_CONTRACT; 
	
    return m_path; 
}

bool BundleInfo::Probe(LPCWSTR path, INT64* size, INT64* offset) const
{
    STANDARD_VM_CONTRACT;

    LPCWSTR fileName = wcsrchr(path, DIRECTORY_SEPARATOR_CHAR_W);
    fileName = (fileName) ? fileName + 1 : path;

    return m_probe(UnicodeToUtf8(fileName), size, offset);
}
