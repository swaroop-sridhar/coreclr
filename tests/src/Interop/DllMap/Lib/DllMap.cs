// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Interop
{
    public static class DllMap
    {
        public static bool Register(Assembly assembly)
        {
            if(!ReadDllMap(assembly.Location))
            {
                return false;
            }

            try
            {
                NativeLibrary.SetDllImportResolver(assembly, Map);
            }
            catch(InvalidOperationException)
            {
                return false;
            }

            return true;
        }

        private static bool ReadDllMap(string assemblyLocation)
        {
            return true;
        }

        private static IntPtr Map(string libraryName, Assembly asm, DllImportSearchPath? dllImportSearchPath)
        {
            return NativeLibrary.Load("NewLib", asm, null);
        }

        [DllImport("NativeLibrary")]
        static extern int RunExportedFunction(IntPtr address, int arg1, int arg2);

    }
}
