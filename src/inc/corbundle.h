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

#include <sys/types.h>

typedef off_t BundleProbe(LPCWSTR filePath);

struct BundleInfo
{
    LPCWSTR BundlePath;
    BundleProbe* Probe;
};

#endif // _COR_BUNDLE_H_
// EOF =======================================================================
