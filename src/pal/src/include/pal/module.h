// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/*++



Module Name:

    include/pal/module.h

Abstract:
    Header file for modle management utilities.



--*/

#ifndef _PAL_MODULE_H_
#define _PAL_MODULE_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*++
Function :
    LOADInitializeModules

    Initialize the process-wide list of modules

Parameters :
    None

Return value :
    TRUE on success, FALSE on failure

--*/
BOOL LOADInitializeModules();

/*++
Function :
    LOADSetExeName

    Set the exe name path

Parameters :
    LPWSTR man exe path and name

Return value :
    TRUE  if initialization succeedded
    FALSE otherwise

--*/
BOOL LOADSetExeName(LPWSTR name);

/*++
Function:
  PAL_LOADLoadPEFile

Abstract
  Loads a PE file into memory.  Properly maps all of the sections in the PE file.  Returns a pointer to the
  loaded base.

Parameters:
    IN hFile    - The file to load

Return value:
    A valid base address if successful.
    0 if failure
--*/
void * PAL_LOADLoadPEFile(HANDLE hFile);

/*++
    PAL_LOADUnloadPEFile

    Unload a PE file that was loaded by PAL_LOADLoadPEFile().

Parameters:
    IN ptr - the file pointer returned by PAL_LOADLoadPEFile()

Return value:
    TRUE - success
    FALSE - failure (incorrect ptr, etc.)
--*/
BOOL PAL_LOADUnloadPEFile(void * ptr);

/*++
    LOADInitializeCoreCLRModule

    Run the initialization methods for CoreCLR module.

Parameters:
    None

Return value:
    TRUE if successful
    FALSE if failure
--*/
BOOL LOADInitializeCoreCLRModule();

/*++
Function :
    LOADGetPalLibrary

    Load and initialize the PAL module.

Parameters :
    None

Return value :
    The PAL Module handle

--*/
HMODULE LOADGetPalLibrary();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _PAL_MODULE_H_ */

