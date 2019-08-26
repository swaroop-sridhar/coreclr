// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/*****************************************************************************
 **                                                                         **
 ** corbundle.h - Header to process single-file bundles                     **
 **                                                                         **
 *****************************************************************************/

#ifndef _COR_BUNDLE_H_
#define _COR_BUNDLE_H_

typedef INT64 BundleProbe(LPCWSTR filePath);

class BundleInfo
{
public:
    BundleInfo(LPCSTR bundlePath, INT64(*probe)(LPCSTR))
    {
        m_path = StringToUnicode(bundlePath);
        m_probe = probe;
    }

    INT64 Probe(LPCWSTR path)
    {
        return m_probe(UnicodeToUtf8(path));
    }

    LPCWSTR Path()
    {
        return m_path;
    }

private:
    static LPCSTR UnicodeToUtf8(LPCWSTR str)
    {
        int length = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, 0, 0);
        LPSTR result = new (nothrow) CHAR[length];
        WideCharToMultiByte(CP_UTF8, 0, str, -1, result, length, 0, 0);
        return result;
    }

    static LPCWSTR Utf8ToUnicode(LPCSTR str)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        LPWSTR result = new (nothrow) WCHAR[length];
        MultiByteToWideChar(CP_UTF8, 0, str, -1, result, length);

        return result;
    }

    LPCWSTR  m_path;
    INT64(*m_probe)(LPCSTR);
};

#endif // _COR_BUNDLE_H_
// EOF =======================================================================
