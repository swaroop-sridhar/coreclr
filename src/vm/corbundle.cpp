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


bool BundleInfo::Probe(LPCWSTR path, INT64* size, INT64* offset) const
{
    LPCWSTR fileName = wcsrchr(path, DIRECTORY_SEPARATOR_CHAR_W);
    fileName = (fileName) ? fileName++ : path;

    return m_probe(m_unicodeToUtf8(fileName), size, offset);
}
