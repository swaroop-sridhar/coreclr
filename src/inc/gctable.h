// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*****************************************************************************/
#ifndef _GCTABLE_H_
#define _GCTABLE_H_
/*****************************************************************************/

//-----------------------------------------------------------------------------
// The current GCInfo Version
//-----------------------------------------------------------------------------

#define GCINFO_VERSION 1

//-----------------------------------------------------------------------------
// GCTable: A wrapper struct that contains the actual GcInfo data 
// and the GcInfo version number.
// - For JIT/Ngened code, Version is always the latest/current version 
// - For ReadyToRun images, the version is extracted from the ready-to-run header
//-----------------------------------------------------------------------------

struct GCTable
{
    PTR_VOID Info;
    UINT32 Version;
};

/*****************************************************************************/
#endif //_GCTABLE_H_
/*****************************************************************************/
