using System;
using System.IO;

namespace Microsoft.Dotnet
{
    class Bundle
    {
        static void Main(string[] args)
        {
            string hostName = args[0];
            BinaryWriter host = new BinaryWriter(File.Open(hostName, FileMode.Open));

            string[] files = Directory.GetFiles(args[1]);
            foreach (string file in files)
            {
                if(Path.GetFullPath(file).Equals(Path.GetFullPath(hostName)))
                {
                    Console.WriteLine("Skip Host");
                    continue;
                }

                BinaryReader resource = new BinaryReader(File.Open(file, FileMode.Open));



            }

            GetFiles(String, String)

            using (BinaryReader reader = new BinaryReader(File.Open(fileName, FileMode.Open)))
            {

            }

        }
    }
}
