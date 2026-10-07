using System;
using System.IO;
internal static class LaunchProbe {
    static void Main() {
        string target=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"launch-probe.txt");
        File.WriteAllLines(target+".tmp",new[]{Environment.CurrentDirectory,AppDomain.CurrentDomain.BaseDirectory});
        File.Move(target+".tmp",target);
    }
}
