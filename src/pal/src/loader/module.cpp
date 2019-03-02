// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/*++



Module Name:

    module.c

Abstract:

    Implementation of module related functions in the Win32 API



--*/

#include "pal/dbgmsg.h"
SET_DEFAULT_DEBUG_CHANNEL(LOADER); // some headers have code with asserts, so do this first

#include "pal/thread.hpp"
#include "pal/malloc.hpp"
#include "pal/file.hpp"
#include "pal/palinternal.h"
#include "pal/module.h"
#include "pal/cs.hpp"
#include "pal/process.h"
#include "pal/file.h"
#include "pal/utils.h"
#include "pal/init.h"
#include "pal/modulename.h"
#include "pal/environ.h"
#include "pal/virtual.h"
#include "pal/map.hpp"
#include "pal/stackstring.hpp"

#include <sys/param.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#if NEED_DLCOMPAT
#include "dlcompat.h"
#else   // NEED_DLCOMPAT
#include <dlfcn.h>
#endif  // NEED_DLCOMPAT
#include <stdlib.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#endif // __APPLE__

#include <sys/types.h>
#include <sys/mman.h>

#if HAVE_GNU_LIBNAMES_H
#include <gnu/lib-names.h>
#endif

using namespace CorUnix;

// In safemath.h, Template SafeInt uses macro _ASSERTE, which need to use variable
// defdbgchan defined by SET_DEFAULT_DEBUG_CHANNEL. Therefore, the include statement
// should be placed after the SET_DEFAULT_DEBUG_CHANNEL(LOADER)
#include <safemath.h>

/* macro definitions **********************************************************/

/* Which path should FindLibrary search? */
#if defined(__APPLE__)
#define LIBSEARCHPATH "DYLD_LIBRARY_PATH"
#else
#define LIBSEARCHPATH "LD_LIBRARY_PATH"
#endif

#define LIBC_NAME_WITHOUT_EXTENSION "libc"

/* static variables ***********************************************************/

/* critical section that regulates access to the module map */
CRITICAL_SECTION module_critsec;

HMODULE pal_handle = nullptr;
LPWSTR pal_path = nullptr;

HMODULE exe_handle;
LPWSTR exe_name;   

char * g_szCoreCLRPath = nullptr;
int MaxWCharToAcpLength = 3;


/* static function declarations ***********************************************/

template<class TChar> static bool LOADVerifyLibraryPath(const TChar *libraryPath);
static bool LOADConvertLibraryPathWideStringToMultibyteString(
    LPCWSTR wideLibraryPath,
    LPSTR multibyteLibraryPath,
    INT *multibyteLibraryPathLengthRef);
static HMODULE LOADLoadLibrary(LPCSTR shortAsciiName, BOOL fDynamic);

/* API function definitions ***************************************************/

/*++
Function:
  LoadLibraryA

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryA(
    IN LPCSTR lpLibFileName)
{
    return LoadLibraryExA(lpLibFileName, nullptr, 0);
}

/*++
Function:
  LoadLibraryW

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryW(
    IN LPCWSTR lpLibFileName)
{
    return LoadLibraryExW(lpLibFileName, nullptr, 0);
}

/*++
Function:
LoadLibraryExA

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryExA(
    IN LPCSTR lpLibFileName,
    IN /*Reserved*/ HANDLE hFile,
    IN DWORD dwFlags)
{
    if (dwFlags != 0) 
    {
        // UNIXTODO: Implement this!
        ASSERT("Needs Implementation!!!");
        return nullptr;
    }

    LPSTR lpstr = nullptr;
    HMODULE handle = nullptr;

    PERF_ENTRY(LoadLibraryA);
    ENTRY("LoadLibraryExA (lpLibFileName=%p (%s)) \n",
          (lpLibFileName) ? lpLibFileName : "NULL",
          (lpLibFileName) ? lpLibFileName : "NULL");

    if (!LOADVerifyLibraryPath(lpLibFileName))
    {
        goto Done;
    }

    /* do the Dos/Unix conversion on our own copy of the name */
    lpstr = strdup(lpLibFileName);
    if (!lpstr)
    {
        ERROR("strdup failure!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Done;
    }
    FILEDosToUnixPathA(lpstr);

    handle = LOADLoadLibrary(lpstr, TRUE);

    /* let LOADLoadLibrary call SetLastError */
 Done:
    if (lpstr != nullptr)
    {
        free(lpstr);
    }

    LOGEXIT("LoadLibraryExA returns HMODULE %p\n", handle);
    PERF_EXIT(LoadLibraryExA);
    return handle;
    
}

/*++
Function:
LoadLibraryExW

See MSDN doc.
--*/
HMODULE
PALAPI
LoadLibraryExW(
    IN LPCWSTR lpLibFileName,
    IN /*Reserved*/ HANDLE hFile,
    IN DWORD dwFlags)
{
    if (dwFlags != 0) 
    {
        // UNIXTODO: Implement this!
        ASSERT("Needs Implementation!!!");
        return nullptr;
    }
    
    CHAR * lpstr;
    INT name_length;
    PathCharString pathstr;
    HMODULE handle = nullptr;

    PERF_ENTRY(LoadLibraryExW);
    ENTRY("LoadLibraryExW (lpLibFileName=%p (%S)) \n",
          lpLibFileName ? lpLibFileName : W16_NULLSTRING,
          lpLibFileName ? lpLibFileName : W16_NULLSTRING);

    if (!LOADVerifyLibraryPath(lpLibFileName))
    {
        goto done;
    }

    lpstr = pathstr.OpenStringBuffer((PAL_wcslen(lpLibFileName)+1) * MaxWCharToAcpLength);
    if (nullptr == lpstr)
    {
        goto done;
    }
    if (!LOADConvertLibraryPathWideStringToMultibyteString(lpLibFileName, lpstr, &name_length))
    {
        goto done;
    }

    /* do the Dos/Unix conversion on our own copy of the name */
    FILEDosToUnixPathA(lpstr);
    pathstr.CloseBuffer(name_length);

    /* let LOADLoadLibrary call SetLastError in case of failure */
    handle = LOADLoadLibrary(lpstr, TRUE);

done:
    LOGEXIT("LoadLibraryExW returns HMODULE %p\n", handle);
    PERF_EXIT(LoadLibraryExW);
    return handle;
}

/*++
Function:
  GetProcAddress

See MSDN doc.
--*/
FARPROC
PALAPI
GetProcAddress(
    IN HMODULE handle,
    IN LPCSTR lpProcName)
{
    FARPROC ProcAddress = nullptr;
    LPCSTR symbolName = lpProcName;

    PERF_ENTRY(GetProcAddress);
    ENTRY("GetProcAddress (hModule=%p, lpProcName=%p (%s))\n",
          handle, lpProcName ? lpProcName : "NULL", lpProcName ? lpProcName : "NULL");

    /* try to assert on attempt to locate symbol by ordinal */
    /* this can't be an exact test for HIWORD((DWORD)lpProcName) == 0
       because of the address range reserved for ordinals contain can
       be a valid string address on non-Windows systems
    */
    if ((DWORD_PTR)lpProcName < GetVirtualPageSize())
    {
        ASSERT("Attempt to locate symbol by ordinal?!\n");
    }

    /* parameter validation */

    if ((lpProcName == nullptr) || (*lpProcName == '\0'))
    {
        TRACE("No function name given\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    // Get the symbol's address.
    // If we're looking for a symbol inside the PAL, we try the PAL_ variant
    // first because otherwise we run the risk of having the non-PAL_
    // variant preferred over the PAL's implementation.
    if (pal_handle && handle == pal_handle)
    {
        int iLen = 4 + strlen(lpProcName) + 1;
        LPSTR lpPALProcName = (LPSTR) alloca(iLen);

        if (strcpy_s(lpPALProcName, iLen, "PAL_") != SAFECRT_SUCCESS)
        {
            ERROR("strcpy_s failed!\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto done;
        }

        if (strcat_s(lpPALProcName, iLen, lpProcName) != SAFECRT_SUCCESS)
        {
            ERROR("strcat_s failed!\n");
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto done;
        }

        ProcAddress = (FARPROC) dlsym(handle, lpPALProcName);
        symbolName = lpPALProcName;
    }

    // If we aren't looking inside the PAL or we didn't find a PAL_ variant
    // inside the PAL, fall back to a normal search.
    if (ProcAddress == nullptr)
    {
        ProcAddress = (FARPROC) dlsym(handle, lpProcName);
    }

    if (ProcAddress)
    {
        TRACE("Symbol %s found at address %p in module %p\n",
              lpProcName, ProcAddress, handle);
    }
    else
    {
        TRACE("Symbol %s not found in module %p\n",
              lpProcName, handle);
        SetLastError(ERROR_PROC_NOT_FOUND);
    }
done:
    LOGEXIT("GetProcAddress returns FARPROC %p\n", ProcAddress);
    PERF_EXIT(GetProcAddress);
    return ProcAddress;
}

/*++
Function:
  FreeLibrary

See MSDN doc.
--*/
BOOL
PALAPI
FreeLibrary(
    IN OUT HMODULE handle)
{
    PERF_ENTRY(FreeLibrary);
    ENTRY("FreeLibrary (hLibModule=%p)\n", handle);

    if (terminator)
    {
        /* PAL shutdown is in progress - ignore FreeLibrary calls */
        goto done;
    }
    if (dlclose(handle) != 0)
    {
        /* report dlclose() failure, but proceed anyway. */
        WARN("dlclose() call failed!\n");
    }

done:
    LOGEXIT("FreeLibrary returns BOOL %d\n", TRUE);
    PERF_EXIT(FreeLibrary);
    return TRUE;
}

/*++
Function:
  FreeLibraryAndExitThread

See MSDN doc.

--*/
PALIMPORT
VOID
PALAPI
FreeLibraryAndExitThread(
    IN HMODULE handle,
    IN DWORD dwExitCode)
{
    PERF_ENTRY(FreeLibraryAndExitThread);
    ENTRY("FreeLibraryAndExitThread()\n"); 
    FreeLibrary(handle);
    ExitThread(dwExitCode);
    LOGEXIT("FreeLibraryAndExitThread\n");
    PERF_EXIT(FreeLibraryAndExitThread);
}

/*++
Function:
  GetModuleFileNameA

See MSDN doc.

Notes :
    If hModule is NULL, the full path of the executable is always returned.
    Otherwise, because of limitations in the dlopen() mechanism, this will only return null.
--*/
DWORD
PALAPI
GetModuleFileNameA(
    IN HMODULE hModule,
    OUT LPSTR lpFileName,
    IN DWORD nSize)
{
    INT name_length;
    DWORD retval = 0;
    LPWSTR wide_name = nullptr;

    PERF_ENTRY(GetModuleFileNameA);
    ENTRY("GetModuleFileNameA (hModule=%p, lpFileName=%p, nSize=%u)\n",
          hModule, lpFileName, nSize);

    LockModuleList();
    if (hModule && !LOADValidateModule((MODSTRUCT *)hModule))
    {
        TRACE("Can't find name for invalid module handle %p\n", hModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }
    wide_name = LOADGetModuleFileName((MODSTRUCT *)hModule);

    if (!wide_name)
    {
        TRACE("File name of module %p is Unknown\n", hModule);
        goto done;
    }

    /* Convert module name to Ascii, place it in the supplied buffer */

    name_length = WideCharToMultiByte(CP_ACP, 0, wide_name, -1, lpFileName,
                                      nSize, nullptr, nullptr);
    if (name_length == 0)
    {
        TRACE("Buffer too small to copy module's file name.\n");
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }

    TRACE("File name of module %p is %s\n", hModule, lpFileName);
    retval = name_length;
done:
    UnlockModuleList();
    LOGEXIT("GetModuleFileNameA returns DWORD %d\n", retval);
    PERF_EXIT(GetModuleFileNameA);
    return retval;
}

/*++
Function:
  GetModuleFileNameW

See MSDN doc.

Notes :
    because of limitations in the dlopen() mechanism, this will only return the
    full path name if a relative or absolute path was given to LoadLibrary, or
    if the module was used in a GetProcAddress call. otherwise, this will return
    the short name as given to LoadLibrary. The exception is if hModule is
    NULL : in this case, the full path of the executable is always returned.
--*/
DWORD
PALAPI
GetModuleFileNameW(
    IN HMODULE hModule,
    OUT LPWSTR lpFileName,
    IN DWORD nSize)
{
    INT name_length;
    DWORD retval = 0;
    LPWSTR wide_name = nullptr;

    PERF_ENTRY(GetModuleFileNameW);
    ENTRY("GetModuleFileNameW (hModule=%p, lpFileName=%p, nSize=%u)\n",
          hModule, lpFileName, nSize);

    LockModuleList();

    wcscpy_s(lpFileName, nSize, W(""));

    if (hModule && !LOADValidateModule((MODSTRUCT *)hModule))
    {
        TRACE("Can't find name for invalid module handle %p\n", hModule);
        SetLastError(ERROR_INVALID_HANDLE);
        goto done;
    }
    wide_name = LOADGetModuleFileName((MODSTRUCT *)hModule);

    if (!wide_name)
    {
        TRACE("Can't find name for valid module handle %p\n", hModule);
        SetLastError(ERROR_INTERNAL_ERROR);
        goto done;
    }

    /* Copy module name into supplied buffer */

    name_length = PAL_wcslen(wide_name);
    if (name_length >= (INT)nSize)
    {
        TRACE("Buffer too small (%u) to copy module's file name (%u).\n", nSize, name_length);
        retval = (INT)nSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        goto done;
    }
    
    wcscpy_s(lpFileName, nSize, wide_name);

    TRACE("file name of module %p is %S\n", hModule, lpFileName);
    retval = (DWORD)name_length;
done:
    UnlockModuleList();
    LOGEXIT("GetModuleFileNameW returns DWORD %u\n", retval);
    PERF_EXIT(GetModuleFileNameW);
    return retval;
}

LPCSTR FixLibCName(LPCSTR shortAsciiName)
{
    // Check whether we have been requested to load 'libc'. If that's the case, then:
    // * For Linux, use the full name of the library that is defined in <gnu/lib-names.h> by the
    //   LIBC_SO constant. The problem is that calling dlopen("libc.so") will fail for libc even
    //   though it works for other libraries. The reason is that libc.so is just linker script
    //   (i.e. a test file).
    //   As a result, we have to use the full name (i.e. lib.so.6) that is defined by LIBC_SO.
    // * For macOS, use constant value absolute path "/usr/lib/libc.dylib".
    // * For FreeBSD, use constant value "libc.so.7".
    // * For rest of Unices, use constant value "libc.so".
    if (strcmp(shortAsciiName, LIBC_NAME_WITHOUT_EXTENSION) == 0)
    {
#if defined(__APPLE__)
        return "/usr/lib/libc.dylib";
#elif defined(__FreeBSD__)
        return "libc.so.7";
#elif defined(LIBC_SO)
        return LIBC_SO;
#else
        return "libc.so";
#endif
    }

    return shortAsciiName;
}

/*++
    PAL_LOADLoadPEFile

    Map a PE format file into memory like Windows LoadLibrary() would do.
    Doesn't apply base relocations if the function is relocated.

Parameters:
    IN hFile - file to map

Return value:
    non-NULL - the base address of the mapped image
    NULL - error, with last error set.
--*/
void *
PALAPI
PAL_LOADLoadPEFile(HANDLE hFile)
{
    ENTRY("PAL_LOADLoadPEFile (hFile=%p)\n", hFile);

    void * loadedBase = MAPMapPEFile(hFile);

#ifdef _DEBUG
    if (loadedBase != nullptr)
    {
        char* envVar = EnvironGetenv("PAL_ForcePEMapFailure");
        if (envVar)
        {
            if (strlen(envVar) > 0)
            {
                TRACE("Forcing failure of PE file map, and retry\n");
                PAL_LOADUnloadPEFile(loadedBase); // unload it
                loadedBase = MAPMapPEFile(hFile); // load it again
            }

            free(envVar);
        }
    }
#endif // _DEBUG

    LOGEXIT("PAL_LOADLoadPEFile returns %p\n", loadedBase);
    return loadedBase;
}

/*++
    PAL_LOADUnloadPEFile

    Unload a PE file that was loaded by PAL_LOADLoadPEFile().

Parameters:
    IN ptr - the file pointer returned by PAL_LOADLoadPEFile()

Return value:
    TRUE - success
    FALSE - failure (incorrect ptr, etc.)
--*/
BOOL 
PALAPI
PAL_LOADUnloadPEFile(void * ptr)
{
    BOOL retval = FALSE;

    ENTRY("PAL_LOADUnloadPEFile (ptr=%p)\n", ptr);

    if (nullptr == ptr)
    {
        ERROR( "Invalid pointer value\n" );
    }
    else
    {
        retval = MAPUnmapPEFile(ptr);
    }

    LOGEXIT("PAL_LOADUnloadPEFile returns %d\n", retval);
    return retval;
}

/*++
    PAL_GetSymbolModuleBase

    Get base address of the module containing a given symbol 

Parameters:
    void *symbol - address of symbol

Return value:
    module base address
--*/
LPCVOID
PALAPI
PAL_GetSymbolModuleBase(void *symbol)
{
    LPCVOID retval = nullptr;

    PERF_ENTRY(PAL_GetPalModuleBase);
    ENTRY("PAL_GetPalModuleBase\n");

    if (symbol == nullptr)
    {
        TRACE("Can't get base address. Argument symbol == nullptr\n");
        SetLastError(ERROR_INVALID_DATA);
    }
    else 
    {
        Dl_info info;
        if (dladdr(symbol, &info) != 0)
        {
            retval = info.dli_fbase;
        }
        else 
        {
            TRACE("Can't get base address of the current module\n");
            SetLastError(ERROR_INVALID_DATA);
        }        
    }

    LOGEXIT("PAL_GetPalModuleBase returns %p\n", retval);
    PERF_EXIT(PAL_GetPalModuleBase);
    return retval;
}

/*++
    PAL_GetLoadLibraryError

    Wrapper for dlerror() to be used by PAL functions

Return value:

A LPCSTR containing the output of dlerror()

--*/
PALIMPORT
LPCSTR
PALAPI
PAL_GetLoadLibraryError()
{

    PERF_ENTRY(PAL_GetLoadLibraryError);
    ENTRY("PAL_GetLoadLibraryError");

    LPCSTR last_error = dlerror();

    LOGEXIT("PAL_GetLoadLibraryError returns %p\n", last_error);
    PERF_EXIT(PAL_GetLoadLibraryError);
    return last_error;
}


/* Internal PAL functions *****************************************************/

/*++
Function :
    LOADInitializeModules

    Initialize the process-wide list of modules

Parameters :
    None

Return value :
    TRUE  if initialization succeedded
    FALSE otherwise

--*/
extern "C"
BOOL LOADInitializeModules()
{
    InternalInitializeCriticalSection(&module_critsec);

    // Initialize module for main executable
    TRACE("Initializing module for main executable\n");

    exe_handle = dlopen(nullptr, RTLD_LAZY);
    if (exe_handle == nullptr)
    {
        ERROR("Executable module will be broken : dlopen(nullptr) failed\n");
        return FALSE;
    }
    exe_name = nullptr;
    return TRUE;
}

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
extern "C"
BOOL LOADSetExeName(LPWSTR name)
{
    exe_name = name;
    return TRUE;
}

/*++
Function :
    LOADGetExeName

    Get the exe name path

Return value :
    LPWSTR exe path and name

--*/
extern "C"
LPCWSTR LOADGetExeName()
{
    return exe_name;
}

/*++
Function :
    LOADGetExeHandle

    Get the exe module handle

Return value :
    The Exe Module Handle

--*/
extern "C"
HMODULE LOADGetExeHandle()
{
    return exe_handle;
}

// Checks the library path for null or empty string. On error, calls SetLastError() and returns false.
template<class TChar>
static bool LOADVerifyLibraryPath(const TChar *libraryPath)
{
    if (libraryPath == nullptr)
    {
        ERROR("libraryPath is null\n");
        SetLastError(ERROR_MOD_NOT_FOUND);
        return false;
    }
    if (libraryPath[0] == '\0')
    {
        ERROR("libraryPath is empty\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    return true;
}

// Converts the wide char library path string into a multibyte-char string. On error, calls SetLastError() and returns false.
static bool LOADConvertLibraryPathWideStringToMultibyteString(
    LPCWSTR wideLibraryPath,
    LPSTR multibyteLibraryPath,
    INT *multibyteLibraryPathLengthRef)
{
    _ASSERTE(multibyteLibraryPathLengthRef != nullptr);
    _ASSERTE(wideLibraryPath != nullptr);

    size_t length = (PAL_wcslen(wideLibraryPath)+1) * MaxWCharToAcpLength;
    *multibyteLibraryPathLengthRef = WideCharToMultiByte(CP_ACP, 0, wideLibraryPath, -1, multibyteLibraryPath,
                                                        length, nullptr, nullptr);
    
    if (*multibyteLibraryPathLengthRef == 0)
    {
        DWORD dwLastError = GetLastError();
        
        ASSERT("WideCharToMultiByte failure! error is %d\n", dwLastError);
        
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }
    return true;
}

/*++
Function :
/*++
Function :
    LOADGetModuleFileName [internal]

    Retrieve the module's full path if it is known, the short name given to
    LoadLibrary otherwise.

Parameters :
    MODSTRUCT *module : module to check

Return value :
    pointer to internal buffer with name of module (Unicode)

Notes :
    this function assumes that the module critical section is held, and that
    the module has already been validated.
--*/
static LPWSTR LOADGetModuleFileName(MODSTRUCT *module)
{
    LPWSTR module_name;
    /* special case : if module is NULL, we want the name of the executable */
    if (!module)
    {
        TRACE("Returning name of main executable\n");
        return exe_name;
    }

    /* Otherwise, there's no API to find the name of the module, so we return null */
    TRACE("Returning null\n");
    return null;
}
    LOADLoadLibrary [internal]

    implementation of LoadLibrary (for use by the A/W variants)

Parameters :
    LPSTR shortAsciiName : name of module as specified to LoadLibrary

    BOOL fDynamic : TRUE if dynamic load through LoadLibrary, FALSE if static load through RegisterLibrary

Return value :
    handle to loaded module

--*/
static HMODULE LOADLoadLibrary(LPCSTR shortAsciiName, BOOL fDynamic)
{
    shortAsciiName = FixLibCName(shortAsciiName);
    
    _ASSERTE(shortAsciiName != nullptr);
    _ASSERTE(shortAsciiName[0] != '\0');

    HMODULE handle = dlopen(shortAsciiName, RTLD_LAZY);
    if (handle == nullptr)
    {
        SetLastError(ERROR_MOD_NOT_FOUND);
    }
    else
    {
        TRACE("dlopen() found module %s\n", shortAsciiName);
    }

    return handle;
}

/*++
    LOADInitializeCoreCLRModule

    Run the initialization methods for CoreCLR module (the module containing this PAL).

Parameters:
    None

Return value:
    TRUE if successful
    FALSE if failure
--*/
BOOL LOADInitializeCoreCLRModule()
{
    HMODULE handle = LOADGetPalHandle();
    if (!handle)
    {
        ERROR("Can not load the PAL module\n");
        return FALSE;
    }

    typedef BOOL (PALAPI *PDLLMAIN)(HMODULE, DWORD, LPVOID);
    PDLLMAIN pRuntimeDllMain = (PDLLMAIN)dlsym(handle, "CoreDllMain");

    if (!pRuntimeDllMain)
    {
        ERROR("Can not find the CoreDllMain entry point\n");
        return FALSE;
    }

    return pRuntimeDllMain(handle, DLL_PROCESS_ATTACH, nullptr);
}

/*++
Function :
    LOADPalLibrary

    Load and initialize the PAL module.

Parameters :
    None

Return value :
    The PAL Module handle

--*/

static HMODULE LOADPalLibrary()
{
    if (pal_handle == nullptr)
    {
        // Initialize the pal module (the module containing LOADPalLibrary). Assumes that 
        // the PAL is linked into the coreclr module because we use the module name containing 
        // this function for the coreclr path.
        TRACE("Loading module for PAL library\n");

        Dl_info info;
        if (dladdr((PVOID)&LOADPalLibrary, &info) == 0)
        {
            ERROR("LOADPalLibrary: dladdr() failed.\n");
            goto exit;
        }
        // Stash a copy of the CoreCLR installation path in a global variable.
        // Make sure it's terminated with a slash.
        if (g_szCoreCLRPath == nullptr)
        {
            size_t  cbszCoreCLRPath = strlen(info.dli_fname) + 1;
            g_szCoreCLRPath = (char*) InternalMalloc(cbszCoreCLRPath);

            if (g_szCoreCLRPath == nullptr)
            {
                ERROR("LOADPalLibrary: InternalMalloc failed!");
                goto exit;
            }

            if (strcpy_s(g_szCoreCLRPath, cbszCoreCLRPath, info.dli_fname) != SAFECRT_SUCCESS)
            {
                ERROR("LOADPalLibrary: strcpy_s failed!");
                goto exit;
            }
        }
        
        pal_path = UTIL_MBToWC_Alloc(info.dli_fname, -1);
        pal_handle = LOADLoadLibrary(info.dli_fname, FALSE);
    }

exit:
    return pal_handle;
}


/*++
Function :
    LOADGetPalHandle

    Load and initialize the PAL module.

Parameters :
    None

Return value :
    The PAL Module handle

--*/
HMODULE LOADGetPalHandle()
{
    if (pal_handle == nullptr)
    {
        LOADPalLibrary();
    }

    return pal_handle;
}

/*++
Function :
    LOADGetPalPath

    Load and initialize the PAL module.

Parameters :
    None

Return value :
    The PAL Module handle

--*/
LPCWSTR LOADGetPalPath()
{
    if (pal_path == nullptr)
    {
        LOADPalLibrary();
    }
    
    return pal_path;
}
