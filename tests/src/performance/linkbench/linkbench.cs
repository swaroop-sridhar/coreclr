using Microsoft.Xunit.Performance;
using Microsoft.Xunit.Performance.Api;
using Xunit;
using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;
using System;
using System.Text;
using System.Globalization;

namespace LinkBench
{
    public class Benchmark
    {
        public string Name;

        public string UnlinkedDir;
        public string LinkedDir;
        public double UnlinkedMsilSize;
        public double LinkedMsilSize;
        public double UnlinkedDirSize;
        public double LinkedDirSize;
        public double MsilSizeReduction;
        public double DirSizeReduction;

        private DirectoryInfo unlinkedDirInfo;
        private DirectoryInfo linkedDirInfo;
        private double certDiff;
        private string workspace;
        const double MB = 1024 * 1024;

        public Benchmark(string _Name, string _workspace, string _UnlinkedDir, string _LinkedDir)
        {
            Name = _Name;
            workspace = _workspace;
            UnlinkedDir = _UnlinkedDir;
            LinkedDir = _LinkedDir;
            unlinkedDirInfo = new DirectoryInfo(UnlinkedDir);
            linkedDirInfo = new DirectoryInfo(LinkedDir);
        }

        public void Compute()
        {
            ComputeCertDiff();
            UnlinkedMsilSize = GetMSILSize(UnlinkedDir);
            LinkedMsilSize = GetMSILSize(LinkedDir);
            UnlinkedDirSize = GetDirSize(unlinkedDirInfo);
            LinkedDirSize = GetDirSize(linkedDirInfo);

            MsilSizeReduction = (UnlinkedMsilSize - LinkedMsilSize) / UnlinkedMsilSize * 100;
            DirSizeReduction = (UnlinkedDirSize - LinkedDirSize) / UnlinkedDirSize * 100;
        }

        // Compute total size of a directory, in MegaBytes
        // Includes all files and subdirectories recursively
        private double GetDirSize(DirectoryInfo dir)
        {
            double size = 0;
            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo fileInfo in files)
            {
                size += fileInfo.Length;
            }
            DirectoryInfo[] subDirs = dir.GetDirectories();
            foreach (DirectoryInfo dirInfo in subDirs)
            {
                size += GetDirSize(dirInfo);
            }

            return size / MB;
        }

        // Compute the size of MSIL files in a directory, in MegaBytes
        // Top level only, excludes crossgen files.
        private double GetMSILSize(string dir)
        {
            string[] files = Directory.GetFiles(dir);
            long msilSize = 0;

            foreach (string file in files)
            {
                if (file.EndsWith(".ni.dll") || file.EndsWith(".ni.exe"))
                {
                    continue;
                }
                try
                {
                    AssemblyLoadContext.GetAssemblyName(file);
                }
                catch (BadImageFormatException)
                {
                    continue;
                }

                msilSize += new FileInfo(file).Length;
            }

            return msilSize / MB;
        }

        // Gets the size of the Certificate header in a MSIL or ReadyToRun binary.
        private long GetCertSize(string file)
        {
            Process p = new Process();
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.FileName = workspace + "\\tests\\src\\performance\\linkbench\\GetCert.cmd";
            p.StartInfo.Arguments = file;
            p.Start();
            string output = p.StandardOutput.ReadToEnd();
            p.WaitForExit();
            long size = Int32.Parse(output.Substring(18, 8),
                NumberStyles.AllowLeadingWhite | NumberStyles.AllowTrailingWhite | NumberStyles.HexNumber);
            return size;
        }

        // Get the total size difference for all certificates in all managed binaries 
        // in 
        private double ComputeCertDiff()
        {
            string[] files = Directory.GetFiles(LinkedDir);
            long totalDiff = 0;

            foreach (string file in files)
            {
                try
                {
                    AssemblyLoadContext.GetAssemblyName(file);
                }
                catch (BadImageFormatException)
                {
                    continue;
                }

                FileInfo fileInfo = new FileInfo(file);
                long linkedCert = GetCertSize(file);
                long unlinkedCert = GetCertSize(UnlinkedDir + "\\" + fileInfo.Name);
                totalDiff += (unlinkedCert - linkedCert);
            }

            return totalDiff / MB;
        }
    }

    public class LinkBench
    {
        private const int Timeoutms = 2000000;
        private static ScenarioConfiguration s_scenarioConfiguration = new ScenarioConfiguration(new TimeSpan(Timeoutms));
        private static MetricModel SizeMetric = new MetricModel { Name = "Size", DisplayName = "File Size", Unit = "MB" };
        private static MetricModel PercMetric = new MetricModel { Name = "Perc", DisplayName = "% Reduction", Unit = "%" };
        private static Benchmark CurrentBenchmark;
        static string Workspace = Environment.GetEnvironmentVariable("CORECLR_REPO");

        public static int Main(string[] args)
        {
            // Workspace is the ROOT of the coreclr tree.
            // This variable is set by the lab scripts.
            if(Workspace == null)
            {
                Console.WriteLine("CORECLR_REPO undefined");
                return -1;
            }

            Benchmark[] Benchmarks =
            {
                new Benchmark("HelloWorld", Workspace,
                              "LinkBench\\HelloWorld\\bin\\release\\netcoreapp2.0\\win10-x64\\publish", 
                              "LinkBench\\HelloWorld\\bin\\release\\netcoreapp2.0\\win10-x64\\linked"),
                new Benchmark("MusicStore", Workspace,
                              "LinkBench\\JitBench\\src\\MusicStore\\bin\\release\\netcoreapp2.0\\win10-x64\\publish",
                              "LinkBench\\JitBench\\src\\MusicStore\\bin\\release\\netcoreapp2.0\\win10-x64\\linked"),
                new Benchmark("MusicStore_R2R", Workspace,
                              "LinkBench\\JitBench\\src\\MusicStore\\bin\\release\\netcoreapp2.0\\win10-x64\\publish_r2r",
                              "LinkBench\\JitBench\\src\\MusicStore\\bin\\release\\netcoreapp2.0\\win10-x64\\linked_r2r"),
                new Benchmark("Corefx", Workspace,
                              "LinkBench\\corefx\\bin\\ILLinkTrimAssembly\\netcoreapp-Windows_NT-Release-x64\\pretrimmed",
                              "LinkBench\\corefx\\bin\\ILLinkTrimAssembly\\netcoreapp-Windows_NT-Release-x64\\trimmed"),
                new Benchmark("Roslyn", Workspace,
                              "LinkBench\\roslyn\\Binaries\\Release\\Exes\\CscCore",
                              "LinkBench\\roslyn\\Binaries\\Release\\Exes\\Linked"),
            };

            // Run the setup Script, which clones, builds and links the benchmarks.
            using (var setup = new Process())
            {
                setup.StartInfo.FileName = Workspace + "\\tests\\src\\performance\\linkbench\\setup.cmd";
                setup.Start();
                setup.WaitForExit();
                if(setup.ExitCode != 0)
                {
                    Console.WriteLine("Setup failed");
                    return -2;
                }
            }

            using (var h = new XunitPerformanceHarness(args))
            {
                // Since this is a size measurement scenario, there are no iterations 
                // to perform. So, create a process that does nothing, to satisfy XUnit.
                // All size measurements are performed PostRun()
                var emptyCmd = new ProcessStartInfo()
                {
                    FileName = Workspace + "\\tests\\src\\performance\\linkbench\\empty.cmd"
                };

                for (int i=0; i < Benchmarks.Length; i++)
                {
                    CurrentBenchmark = Benchmarks[i];
                    h.RunScenario(
                      emptyCmd,
                      PreIteration,
                      PostIteration,
                      PostRun,
                      s_scenarioConfiguration);
                }
            }

            return 0;
        }

        private static void PreIteration()
        {
        }

        private static void PostIteration()
        {
        }

        private static ScenarioBenchmark PostRun()
        {
            // The XUnit output doesn't print the benchmark name, so print it now.
            Console.WriteLine("{0}", CurrentBenchmark.Name);
            var benchmark = new ScenarioBenchmark(CurrentBenchmark.Name)
            {
                Namespace = "LinkBench"
            };

            CurrentBenchmark.Compute();


            addMeasurement(ref benchmark, "MSIL Unlinked", SizeMetric, CurrentBenchmark.UnlinkedMsilSize);
            addMeasurement(ref benchmark, "MSIL Linked", SizeMetric, CurrentBenchmark.LinkedMsilSize);
            addMeasurement(ref benchmark, "MSIL %Reduction", PercMetric, CurrentBenchmark.MsilSizeReduction);
            addMeasurement(ref benchmark, "Total Uninked", SizeMetric, CurrentBenchmark.UnlinkedDirSize);
            addMeasurement(ref benchmark, "Total Linked", SizeMetric, CurrentBenchmark.LinkedDirSize);
            addMeasurement(ref benchmark, "Total %Reduction", PercMetric, CurrentBenchmark.DirSizeReduction);

            return benchmark;
        }

        private static void addMeasurement(ref ScenarioBenchmark bench, string name, MetricModel metric, double value)
        {
            var iteration = new IterationModel
            {
                Iteration = new Dictionary<string, double>()
            };
            iteration.Iteration.Add(metric.Name, value);

            var size = new ScenarioTestModel(name);
            size.Performance.Metrics.Add(metric);
            size.Performance.IterationModels.Add(iteration);
            bench.Tests.Add(size);
        }
    }
}
