# Dllmap Design 
This document is intended to describe a design of a Dllmap feature for .NET Core.

Author: Anna Aniol (@annaaniol)

​             [edited by @swaroop-sridhar]

## Background

### .NET Core P/invoke mechanism
There is a .NET directive ([DllImport](https://msdn.microsoft.com/en-us/library/system.runtime.interopservices.dllimportattribute(v=vs.110).aspx)) indicating that the attributed method is exposed by an unmanaged DLL as a static entry point. It enables to call a function exported from an unmanaged DLL inside a managed code. To make such a call, library name must be provided. Names of native libraries are OS-specific, so once the name is defined, the call can be executed correctly on one OS only.

Right now, CoreCLR has no good way to handle the differences between platforms when it comes to p/invoking native libraries. Here is an example usage of DllImport:
```c#
// Use DllImport to import the Win32 MessageBox function.
[DllImport("user32.dll", CharSet = CharSet.Unicode)]
public  static  extern  int  MessageBox(IntPtr  hWnd, String text, String caption, uint type);
```
This import works on Windows, but it doesn’t work on any other OS. If run e.g. on Linux, a DllNotFoundException will be thrown,
which means that a DLL specified in a DLL import directive cannot be found.

### Dllmap

Dllmap is a means to realize cross-platform p/invoke in .NET. Using Dllmap, a user can control interop methods by defining custom mapping between OS-specific dll names.

### Mono Dllmap

Mono provides a feature that addresses the problem of cross-platform p/invoke support. Mono’s [Dllmap](http://www.mono-project.com/docs/advanced/pinvoke/dllmap/) 
enables to configure p/invoke signatures at runtime. By providing an XML configuration file, 
user can define a custom mapping between OS-specific library names and method names.
Thanks to that, even if a library defined in DllImport is incompatible with an OS that is currently running the application, a correct unmanaged method can be called (if it exists for this OS).

In Mono Dllmap feature custom mapping can be tightly specified based on the OS name, CPU name and a wordsize. This simple Mono example maps references to the cygwin1.dll library to the libc.so.6 file:

```xml
<configuration>
    <dllmap  dll="cygwin1.dll" target="libc.so.6"/>
</configuration>
```



## .NET Core Dllmap

### Mono compatibility

The Dllmap method is meant as a compatibility feature for Mono and provides users with a straightforward migration story from Mono to .NET Core applications.

The Mono-like dllmap behavior will consume a configuration file of [the same style](http://www.mono-project.com/docs/advanced/pinvoke/dllmap/) as Mono does.
Users will be able to use their old Mono configuration files when specifying the mapping for .NET Core applications.
Configuration files must be placed next to the assemblies that they describe.

#### New users: flexibility
New users, who plan to support p/invokes in their cross-platform applications, should implement their custom mapping policies that satisfies their needs.
The runtime will use a specific callback on each dll load attempt.
The user’s code can subscribe to these callbacks and define any mapping strategy.
The default Mono-like mapping methods are provided for an easier migration from Mono.
For newcomers, it’s highly recommended to use callbacks and implement their own handers. Details of the callback strategy are described in the Design section.

### Usage example (XML configuration)

Let’s say there is a customer developing a .NET application with p/invokes on Windows.
The customer wants the application to run on both Windows and Linux without introducing any changes in the code.

The application calls a function `MyFunction()` from an OS-specific library `libWindows.dll`.
```c#
[DllImport("libWindows.dll")]
static  extern  uint  MyFunction();
```

To make it work on Linux, that does not have `libWindows.dll`, user must define a mapping of the dll.
To achieve this, the user puts an XML configuration file next to the dll that is about to be loaded. The file looks like this:
```xml
<configuration>
    <dllmap dll="libWindows.dll" target="libLinux.so.6" />
</configuration>
```

With this file, all calls to `libWindows.dll` get automatically mapped to calls to `libLinux.so.6` on runtime.
If only `libLinux.so.6` has a corresponding `MyFunction()` method, it gets called and the end user
can’t see any difference in application’s behavior. Running the application cross-platform does not require any OS-specific changes in the code.
All the mapping is defined in advance in the external configuration file.

This is a very basic scenario and it can be extended to different operating systems and libraries.
It assumes that user does not implement any custom actions (handlers) but uses the default Mono-like dllmap behavior.

## Design
### XML configuration file
For a basic case, the mapping must be defined in an XML configuration file and placed next to the assembly that requires mapping of p/invokes.
The file must be named AssemblyName.config where AssemblyName is a name of the executable for which the mapping is defined.

XML parsing will be implemented in corefx.labs using XML parsers that .NET provides.

### Library mapping
In [dllimport.cpp](https://github.com/dotnet/coreclr/blob/master/src/vm/dllimport.cpp) file there is a method that loads the DLL and finds the procaddress for an N/Direct call.
```c++
VOID NDirect::NDirectLink(NDirectMethodDesc *pMD)
{
    …
    HINSTANCE hmod = LoadLibraryModule( pMD, &errorTracker );
    …
    LPVOID pvTarget = NDirectGetEntryPoint(pMD, hmod);
    …
    pMD->SetNDirectTarget(pvTarget);
}
```
`LoadLibraryModule`  is responsible for loading a correct library and `NDirectGetEntryPoint ` is responsible for resolving a right entrypoint.

There are several functions that get called in `LoadLibraryModule`  to get an hmod of the unmanaged dll.
If any of them returns a valid hmod, execution flow ends and the hmod gets returned.

The proposed change extracts the logic of `LoadLibraryModule` and places it in a new function called `LoadLibraryModuleHierarchy`.
`LoadLibraryModule` calls two functions:

```c++
hmod = LoadLibraryViaCallback(Assembly* pAssembly, AppDomain* pDomain, const wchar_t* wszLibName, BOOL searchAssemblyDirectory, DWORD dllImportSearchPathFlag);
if(hmod == null)
{
    hmod = LoadLibraryModuleHierarchy(Assembly *pAssembly, LPCWSTR wszLibName, BOOL searchAssemblyDirectory, DWORD dllImportSearchPathFlag);
}
```

`LoadLibraryViaCallback`  executs a callback for all assemblies except `System.Private.CoreLib`,
which can’t be mapped at any time. The check to determine if the assembly is `System.Private.CoreLib` will happen on runtime in the unmanaged code.

`LoadLibraryViaCallback` makes a call to managed code and managed code decides which dll to load (if mapping/custom behavior has been defined).
Managed code calls `LoadLibraryModuleHierarchy` with updated parameters (e.g. with a mapped name) and passes a result back to unmanaged code.

If there was no custom loading behavior defined for the assembly that triggered the callback, then LoadLibraryViaCallback returns a null pointer
and LoadLibraryModuleHierarchy gets called with the original parameters from the DllImport directive.

### Callbacks

On each dll load attempt, the unmanaged code will make a call to a managed method(`LoadLibraryCallback`). On the managed side there is a map that maps
an assembly to a specific callback that should be executed when `LoadLibraryCallback` gets called.

Users can implement their own callback functions and register them for a specific assembly or for all project assemblies. Implementing own customized handlers is highly recommended.
That gives a user full flexibility when using dllmap and doesn't limit defining the mapping to only XML-based style.
Callbacks can be executed for all assemblies except `System.Private.CoreLib`.

For those, who only want Mono compatibility, implementation of Mono-style callbacks will be placed in `corefx.labs`.

Registering more than one callback per assembly will throw an exception.

### Example resolution flow

**User’s code**

```c#
using System.Runtime.InteropServices;

// Implements a handler of AssemblyLoad event (raised for every assembly on its load)
public static void AssemblyLoadCallbackHandler(object sender, AssemblyLoadEventArgs args)
{
    Assembly assembly = args.LoadedAssembly;
    // Registers a speific callback for the given assembly
    NativeLibrary.RegisterNativeLibraryLoadCallback(assembly, SimpleCallbackHandler);
}


// Implements a custom callback function that does the mapping and calls
// NativeLibrary.Load to load a library with updated parameters
public static Func<LoadNativeLibraryArgs, NativeLibrary> SimpleCallbackHandler = SimpleCallbackHandlerLogic;

public static NativeLibrary SimpleCallbackHandlerLogic(LoadNativeLibraryArgs args)
{
    string libraryName = args.LibraryName;
    DllImportSearchPath dllImportSearchPath = args.DllImportSearchPath;
    Assembly assembly = args.CallingAssembly;

    if (libraryName == "TheNameToReplace")
    {
        libraryName = "TheCorrectName";
        NativeLibrary nativeLibrary = NativeLibrary.Load(libraryName, dllImportSearchPath, assembly);
        return nativeLibrary;
    }
    return new NativeLibrary("LibraryNotFound", IntPtr.Zero);
}


// Declares the actual interop function
[DllImport("TheNameToReplace.dll", EntryPoint="MyFunction")]
static  extern  int  MyFunction();

public static int Main()
{
    // Registers a callback per each assembly of the current domain that gets loaded
    AppDomain.CurrentDomain.AssemblyLoad += AssemblyLoadCallbackHandler;
    // Executes the imported function
    // It will execute MyFunction() from TheCorrectName.dll
    MyFunction();
}
```

**Runtime [unmanaged code]**
-   Calls `LoadLibraryCallback` that executes a `callback` function to check for mappings and load a dll if there was a mapping strategy defined
    

**System.Runtime.InteropServices.LoadLibrary**
-   Is a public sealed class with `Name` and `Handle`
-   Exposes a `Load` method for loading a native library
```c#
public static NativeLibrary Load(string libraryName);
public static NativeLibrary Load(string libraryName, DllImportSearchPath dllImportSearchPath, Assembly assembly);
```
- Defines a method to register a callback per assembly
```c#
public static void RegisterNativeLibraryLoadCallback(Assembly assembly, Func<LoadNativeLibraryArgs, NativeLibrary> callback)
```

**corefx.labs**
-   Implements Mono compatibile handler with XML parsing

## Testing

**Tests**

* [Mono tests](https://github.com/mono/mono/tree/0bcbe39b148bb498742fc68416f8293ccd350fb6/mcs/tools/mono-shlib-cop) 
* Custom unit tests, to include tests for:
  * DLLs naming: using vatiants of relative/absolute paths, filename/extension  specification, etc.
  * Fault tolerance, and error handling.

**Operating Systems**. 

Windows, Linux, Mac

**Platforms ** 

X64, X86, ARM, ARM64

### Related discussions
[Lightweight and dynamic driving of P/Invoke](https://github.com/dotnet/coreclr/issues/19112)

[Handling p/invokes for different platforms and discussions about dllmap](https://github.com/dotnet/coreclr/issues/930)