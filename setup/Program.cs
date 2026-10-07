using System;
using System.IO;
using System.Threading;
using System.Windows.Forms;
using System.Reflection;
using System.Runtime.InteropServices;

[assembly: AssemblyTitle("Setup Boom Boom Rocket")]
[assembly: AssemblyDescription("Portable game and optional DLC installer")]
[assembly: AssemblyProduct("Boom Boom Rocket")]
[assembly: AssemblyVersion("0.9.1.0")]
[assembly: AssemblyFileVersion("0.9.1.0")]
[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]
namespace Bbr.Setup {
    internal static class Program {
        [DllImport("user32.dll")] static extern bool SetProcessDPIAware();
        [STAThread] static int Main(string[] args) {
            try {
                // Automation uses the same installation engine as the graphical interface.
                if(args.Length>0 && args[0]=="--install") {
                    string game=null,dlc=null,destination=null;
                    for(int i=1;i<args.Length;i+=2) {
                        if(i+1>=args.Length) throw new ArgumentException("Missing argument value.");
                        if(args[i]=="--game")game=args[i+1]; else if(args[i]=="--dlc")dlc=args[i+1]; else if(args[i]=="--destination")destination=args[i+1]; else throw new ArgumentException("Unknown option: "+args[i]);
                    }
                    if(destination==null) throw new ArgumentException("--destination is required.");
                    new InstallEngine().Install(game,dlc,destination,(p,s)=>Console.WriteLine(p+"% "+s),CancellationToken.None); return 0;
                }
                if(args.Length>0 && args[0]=="--verify-payload") {
                    using(var s=Assembly.GetExecutingAssembly().GetManifestResourceStream("Bbr.Payload"))
                    using(var zip=new System.IO.Compression.ZipArchive(s)) foreach(var e in zip.Entries) Console.WriteLine(e.FullName);
                    return 0;
                }
                SetProcessDPIAware(); Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new SetupForm()); return 0;
            } catch(Exception error) {
                if(args.Length>0) Console.Error.WriteLine(error.ToString());
                else MessageBox.Show(error.Message,"Boom Boom Rocket Setup",MessageBoxButtons.OK,MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}
