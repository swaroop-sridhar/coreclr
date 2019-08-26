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
    BundleInfo(LPCWSTR bundlePath, INT64(*probe)(LPCSTR), LPCSTR (*unicodeToUtf8)(LPCWSTR))
    {
        m_path = bundlePath;
        m_probe = probe;
        m_unicodeToUtf8 = unicodeToUtf8;
    }

    INT64 Probe(LPCWSTR path) const
    {
        return (m_probe != nullptr) ? m_probe(m_unicodeToUtf8(path)) : 0;
    }

    LPCWSTR Path() const
    {
        return m_path;
    }

private:

    LPCWSTR  m_path;
    INT64(*m_probe)(LPCSTR);
    LPCSTR (*m_unicodeToUtf8)(LPCWSTR str);
};

#endif // _COR_BUNDLE_H_
// EOF =======================================================================
