using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Windows.Forms;
using Bbr.Setup;

[assembly: AssemblyTitle("Boom Boom Rocket")]
[assembly: AssemblyDescription("Portable Boom Boom Rocket launcher and game-file importer")]
[assembly: AssemblyProduct("Boom Boom Rocket")]
[assembly: AssemblyVersion("0.9.1.0")]
[assembly: AssemblyFileVersion("0.9.1.0")]
[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

namespace Bbr.Launcher {
    internal static class Program {
        [STAThread] static int Main(string[] args) {
            try {
                AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling",false);
                AppContext.SetSwitch("Switch.System.IO.BlockLongPaths",false);
                var game=new PortableGame(AppDomain.CurrentDomain.BaseDirectory);
                bool manage=false,dlcOnly=false;string input=null;
                foreach(string arg in args) {
                    if(arg=="--manage")manage=true;
                    else if(arg=="--dlc"){manage=true;dlcOnly=true;}
                    else if(arg.StartsWith("--wait-pid=")) {
                        int pid;if(!int.TryParse(arg.Substring(11),out pid))throw new ArgumentException("Invalid game process.");
                        try {using(var process=Process.GetProcessById(pid)) {
                            if(!string.Equals(process.MainModule.FileName,Path.Combine(game.Resources,"boom_boom_rocket.exe"),StringComparison.OrdinalIgnoreCase))throw new IOException("Not this installation's game process.");
                            if(!process.WaitForExit(30000))throw new IOException("The game is still closing. Open Manage Game Files again after it closes.");
                        }}catch(ArgumentException){} // Game already exited before launcher started.
                    }else if(input==null&&(File.Exists(arg)||Directory.Exists(arg)))input=arg;
                    else throw new ArgumentException("Use Boom Boom Rocket.exe [package-or-folder] or --manage.");
                }
                if(!manage&&input==null&&game.Ready()) {
                    game.Prepare((p,s)=>{},System.Threading.CancellationToken.None);
                    game.Launch();return 0;
                }
                Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new ImportForm(game,manage,input,dlcOnly));return 0;
            } catch(Exception error) {
                MessageBox.Show(error.Message,"Boom Boom Rocket",MessageBoxButtons.OK,MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}
