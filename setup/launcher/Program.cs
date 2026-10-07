using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

[assembly: AssemblyTitle("Boom Boom Rocket")]
[assembly: AssemblyDescription("Portable Boom Boom Rocket play launcher")]
[assembly: AssemblyProduct("Boom Boom Rocket")]
[assembly: AssemblyVersion("0.9.1.0")]
[assembly: AssemblyFileVersion("0.9.1.0")]
[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

namespace Bbr.Launcher {
    internal static class Program {
        [STAThread] static int Main() {
            try {
                // Resolve from this launcher, never the caller's working directory.
                string game=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"resources");
                string exe=Path.Combine(game,"boom_boom_rocket.exe");
                if(!File.Exists(exe) || !File.Exists(Path.Combine(game,"assets","default.xex")))
                    throw new IOException("Run Setup Boom Boom Rocket.exe in the parent folder first. Keep Boom Boom Rocket.exe inside Game, beside the complete resources folder.");
                Process.Start(new ProcessStartInfo(exe) { WorkingDirectory=game, UseShellExecute=false });
                // No hidden launcher process remains after starting the game.
                return 0;
            } catch(Exception error) {
                MessageBox.Show(error.Message,"Boom Boom Rocket",MessageBoxButtons.OK,MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}
