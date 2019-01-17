// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

using Console = Internal.Console;

[assembly: DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
public class CallBackTests
{
    public static int Main()
    {
#if false
        try
        {
            Assembly assembly = Assembly.GetExecutingAssembly();

            DllImportResolver resolver =
                (string libraryName, Assembly asm, DllImportSearchPath? dllImportSearchPath) =>
                {
                    if (dllImportSearchPath != DllImportSearchPath.System32)
                    {
                        Console.WriteLine($"Unexpected dllImportSearchPath: {dllImportSearchPath.ToString()}");
                        throw new ArgumentException();
                    }

                    return NativeLibrary.Load("ResolveLib", asm, null);
                };

            DllImportResolver anotherResolver =
                (string libraryName, Assembly asm, DllImportSearchPath? dllImportSearchPath) =>
                    IntPtr.Zero;

            try
            {
                NativeSum(10, 10);
                Console.WriteLine("Exception expected: no callback registered yet");
                return 101;
            }
            catch (DllNotFoundException e) {}

            try
            {
                NativeLibrary.SetDllImportResolver(null, resolver);

                Console.WriteLine("Exception expected: assembly parameter null");
                return 102;
            }
            catch (ArgumentNullException e) { }

            try
            {
                NativeLibrary.SetDllImportResolver(assembly, null);

                Console.WriteLine("Exception expected: resolver parameter null");
                return 103;
            }
            catch (ArgumentNullException e) { }

            // Set a resolver callback
            NativeLibrary.SetDllImportResolver(assembly, resolver);

            try
            {
                // Try to set another resolver on the same assembly.
                NativeLibrary.SetDllImportResolver(assembly, anotherResolver);

                Console.WriteLine("Exception expected: Trying to register second resolver");
                return 104;
            }
            catch (InvalidOperationException e) { }

            if (NativeSum(10, 10) != 20)
            {
                Console.WriteLine("Unexpected ReturnValue from NativeSum()");
                return 105;
            }
        }
        catch (Exception e)
        {
            Console.WriteLine($"Unexpected exception: {e.ToString()} {e.Message}");
            return 106;
        }
#endif

        return P1(1, 1) + P2(2, 2) + P3(3, 3) + P4(4, 4) + P5(5, 5) + P6(6, 6) + P7(7, 7) + P8(8, 8) + P9(9, 9) + P10(10, 10) + P11(11, 11) + P12(12, 12);
    }

    [DllImport("NativeLib1", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P1(int arg1, int arg2);

    [DllImport("NativeLib2", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P2(int arg1, int arg2);

    [DllImport("NativeLib3", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P3(int arg1, int arg2);

    [DllImport("NativeLib4", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P4(int arg1, int arg2);

    [DllImport("NativeLib5", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P5(int arg1, int arg2);

    [DllImport("NativeLib6", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P6(int arg1, int arg2);

    [DllImport("NativeLib7", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P7(int arg1, int arg2);

    [DllImport("NativeLib8", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P8(int arg1, int arg2);

    [DllImport("NativeLib9", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P9(int arg1, int arg2);

    [DllImport("NativeLib10", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P10(int arg1, int arg2);

    [DllImport("NativeLib11", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P11(int arg1, int arg2);

    [DllImport("NativeLib12", EntryPoint = "NativeSum")]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    static extern int P12(int arg1, int arg2);
}
