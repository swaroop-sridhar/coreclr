// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using TestLibrary;

using Console = Internal.Console;
public class NativeLibraryTest
{
    public static int Main()
    {
        bool success = true;

        Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
        string testBin = assembly.Location;
        string testBinDir = Path.GetDirectoryName(testBin);
        string libName;
        IntPtr handle;
#if false
        // -----------------------------------------------
        //         Simple LoadLibrary() API Tests
        // -----------------------------------------------

        // Calls on correct full-path to native lib
        libName = Path.Combine(testBinDir, GetNativeLibraryName());
        success |= EXPECT(LoadLibrarySimple(libName));
        success |= EXPECT(TryLoadLibrarySimple(libName));

        // Calls on non-existant file
        libName = Path.Combine(testBinDir, "notfound");
        success |= EXPECT(LoadLibrarySimple(libName), 1);
        success |= EXPECT(TryLoadLibrarySimple(libName), 1);

        // Calls on an invalid file
        libName = Path.Combine(testBinDir, "NativeLibrary.cpp");
        success |= EXPECT(LoadLibrarySimple(libName), 2);
        success |= EXPECT(TryLoadLibrarySimple(libName), 1);


        // -----------------------------------------------
        //         Advanced LoadLibrary() API Tests
        // -----------------------------------------------

        // Advanced LoadLibrary() API Tests
        // Calls on correct full-path to native lib
        libName = Path.Combine(testBinDir, GetNativeLibraryName());
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null));
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null));

        // Calls on non-existant file
        libName = Path.Combine(testBinDir, "notfound");
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null), 1);
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null), 1);

        // Calls on an invalid file
        libName = Path.Combine(testBinDir, "NativeLibrary.cpp");
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null), 2);
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null), 1);

        // Calls on just Native Library name
        libName = GetNativeLibraryPlainName();
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null));
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null));

        // Calls on Native Library name with correct prefix-suffix
        libName = GetNativeLibraryName();
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null));
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null));

        // Calls on absolute path without prefix-siffix
        libName = Path.Combine(testBinDir, GetNativeLibraryPlainName());
        success |= EXPECT(LoadLibraryAdvanced(libName, assembly, null));
        success |= EXPECT(TryLoadLibraryAdvanced(libName, assembly, null));

        // Calls on Native Library name, exclude search in assembly directory
        if (TestLibrary.Utilities.IsWindows)
        {
            libName = GetNativeLibraryPlainName();
            success |= EXPECT(LoadLibraryAdvanced("mapi32", assembly, DllImportSearchPath.System32));
            success |= EXPECT(TryLoadLibraryAdvanced("mapi32", assembly, DllImportSearchPath.System32));
        }
#endif
        // -----------------------------------------------
        //         FreeLibrary Tests
        // -----------------------------------------------

        // Valid Free
        libName = Path.Combine(testBinDir, GetNativeLibraryName());
        handle = Marshal.LoadLibrary(libName);
        success |= EXPECT(FreeLibrary(handle));

        // Double Free
        success |= EXPECT(FreeLibrary(handle), 1);

        // Null Free
        success |= EXPECT(FreeLibrary((IntPtr) null), 1);

        // Valid Free
        success |= EXPECT(FreeLibrary((IntPtr) 0x1234), 1);

        // -----------------------------------------------
        //         GetLibraryExport Tests
        // -----------------------------------------------

        Console.WriteLine("The End");
        return (success) ? 100 : -100;
    }

    static string GetNativeLibraryPlainName()
    {
        return "NativeLibrary";
    }

    static string GetNativeLibraryName()
    {
        string baseName = GetNativeLibraryPlainName();

        if (TestLibrary.Utilities.IsWindows)
        {
            return baseName + ".dll";
        }
        if (TestLibrary.Utilities.IsUnix)
        {
            return "lib" + baseName + ".so";
        }
        if (TestLibrary.Utilities.IsMacOSX)
        {
            return "lib" + baseName + ".dylib";
        }

        return "ERROR";
    }

    static bool EXPECT(int actualValue, int expectedValue = 0)
    {
        if (actualValue == expectedValue)
        {
            Console.WriteLine(" [OK]");
            return true;
        }
        else
        {
            Console.WriteLine(" [FAIL]");
            return false;
        }
    }

    static int LoadLibrarySimple(string libPath)
    {
        Console.Write(String.Format("LoadLibrary({0}) ", libPath));

        IntPtr handle;
        try
        {
            handle = Marshal.LoadLibrary(libPath);
        }
        catch (DllNotFoundException e)
        {
            Console.Write("DllNotFoundException");
            return 1;
        }
        catch (BadImageFormatException e)
        {
            Console.Write("BadImageFormatException");
            return 2;
        }
        catch (Exception e)
        {
            Console.Write(String.Format("Unexpected Exception {0}", e.ToString()));
            return 3;
        }

        if (handle == null)
        {
            Console.Write("Handle is null");
            return 4;
        }

        return 0;
    }

    static int TryLoadLibrarySimple(string libPath)
    {
        Console.Write(String.Format("TryLoadLibrary({0}) ", libPath));

        IntPtr handle;
        bool success = true;
        try
        {
            success = Marshal.TryLoadLibrary(libPath, out handle);
        }
        catch (Exception e)
        {
            Console.Write("Threw Exception");
            return 3;
        }

        if (!success)
        {
            Console.Write("Returned False");
            return 1;
        }

        if (handle == null)
        {
            Console.Write("Handle is null");
            return 2;
        }

        return 0;
    }

    static int LoadLibraryAdvanced(string libName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        Console.Write(String.Format("LoadLibrary({0}, {1}, {2}) ", libName, "assembly", searchPath));

        IntPtr handle;
        try
        {
            handle = Marshal.LoadLibrary(libName, assembly, searchPath);
        }
        catch (DllNotFoundException e)
        {
            Console.Write("DllNotFoundException");
            return 1;
        }
        catch (BadImageFormatException e)
        {
            Console.Write("BadImageFormatException");
            return 2;
        }
        catch (Exception e)
        {
            Console.Write(String.Format("Unexpected Exception {0}", e.ToString()));
            return 3;
        }

        if (handle == null)
        {
            Console.Write("Handle is null");
            return 4;
        }

        return 0;
    }

    static int TryLoadLibraryAdvanced(string libName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        Console.Write(String.Format("TryLoadLibrary({0}, {1}, {2}) ", libName, "assembly", searchPath));

        IntPtr handle;
        bool success = true;
        try
        {
            success = Marshal.TryLoadLibrary(libName, assembly, searchPath, out handle);
        }
        catch (Exception e)
        {
            Console.Write("Threw Exception");
            return 3;
        }

        if (!success)
        {
            Console.Write("Returned False");
            return 1;
        }

        if (handle == null)
        {
            Console.Write("Handle is null");
            return 2;
        }
        return 0;
    }

    static int FreeLibrary(IntPtr handle)
    {
        Console.Write(String.Format("FreeLibrary({0}) ", handle));

        try
        {
            Marshal.FreeLibrary(handle);
        }
        catch (InvalidOperationException e)
        {
            Console.Write("InvalidOperationException");
            return 1;
        }
        catch (Exception e)
        {
            Console.Write(String.Format("Unexpected Exception {0}", e.ToString()));
            return 2;
        }

        return 0;
    }



}

