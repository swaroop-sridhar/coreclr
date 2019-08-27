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

class BundleInfo
{
public:
    BundleInfo(LPCWSTR bundlePath, bool(*probe)(LPCSTR, INT64*, INT64*), LPCSTR (*unicodeToUtf8)(LPCWSTR))
    {
        m_path = bundlePath;
        m_probe = probe;
        m_unicodeToUtf8 = unicodeToUtf8;
    }

    bool Probe(LPCWSTR path, INT64 *size, INT64 *offset) const
    {
        return m_probe(m_unicodeToUtf8(path), size, offset);
    }

    LPCWSTR Path() const
    {
        return m_path;
    }

private:

    LPCWSTR  m_path;
    bool(*m_probe)(LPCSTR, INT64*, INT64*);
    LPCSTR (*m_unicodeToUtf8)(LPCWSTR str);
};

#endif // _COR_BUNDLE_H_
// EOF =======================================================================
