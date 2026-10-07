using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using System.Reflection;
using System.IO.Compression;
using System.Web.Script.Serialization;

[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

namespace Bbr.Setup {
    internal static class InstallerTests {
        static int checks;
        static void Check(bool ok,string label){if(!ok)throw new Exception("FAIL: "+label);checks++;Console.WriteLine("PASS: "+label);}
        static string Hash(string path){using(var s=File.OpenRead(path))using(var h=SHA256.Create())return BitConverter.ToString(h.ComputeHash(s));}
        static void Reject(Action action,string label){bool rejected=false;try{action();}catch{rejected=true;}Check(rejected,label);}
        static void Quiet(int p,string text){}
        static int Main(string[] args) {
            try {
                string game=args[0],dlc=args[1],root=Path.GetFullPath(args[2]);
                if(Directory.Exists(root))throw new IOException("Test output already exists; select a new folder.");
                Directory.CreateDirectory(root);
                string gameHash=Hash(game),dlcHash=Hash(dlc);
                string install=Path.Combine(root,"Base then DLC");
                Directory.CreateDirectory(install);
                File.WriteAllText(Path.Combine(install,"Setup Boom Boom Rocket.exe"),"installer preservation sentinel");
                Check(!Directory.Exists(Path.Combine(install,"Game")),"Clean pre-setup destination has no Game folder");
                using(var payload=Assembly.GetExecutingAssembly().GetManifestResourceStream("Bbr.Payload"))
                using(var zip=new ZipArchive(payload))
                    Check(zip.Entries.All(e=>!e.FullName.EndsWith(".xex",StringComparison.OrdinalIgnoreCase) &&
                          !e.FullName.Contains("/assets/") && !e.FullName.Contains("/userdata/") &&
                          !e.FullName.EndsWith(".cs") && !e.FullName.EndsWith(".cpp")),
                          "Embedded public payload excludes original XEX, assets, saves and development projects");
                var engine=new InstallEngine();
                string launcher=engine.Install(game,null,install,Quiet,CancellationToken.None);
                string gameFolder=Path.Combine(install,"Game"),data=Path.Combine(gameFolder,"resources");
                string freshConfig=File.ReadAllText(Path.Combine(data,"boom_boom_rocket.toml"));
                var assigned=System.Text.RegularExpressions.Regex.Matches(freshConfig,"bbr_player_(?:one|two)_name = \"([^\"]+)\"")
                    .Cast<System.Text.RegularExpressions.Match>().Select(m=>m.Groups[1].Value).ToArray();
                Check(assigned.Length==2 && assigned[0]!=assigned[1] && assigned.All(n=>PlayerNames.Load().Contains(n)),
                    "Fresh installation assigns two distinct names from the supplied list");
                Check(PlayerNames.Load().All(n=>n.Length<=15) && PlayerNames.Load().Distinct().Count()==PlayerNames.Load().Length,
                    "Native name limits respected and duplicate suggestions do not bias selection");
                var sampled=Enumerable.Range(0,100).Select(i=>PlayerNames.Choose()).ToArray();
                Check(sampled.All(pair=>pair[0]!=pair[1]) && sampled.Select(p=>p[0]).Distinct().Count()>1,
                    "Name choices vary per installation without duplicate players");
                Check(InstallEngine.HasGame(install)&&!InstallEngine.HasDlc(install),"Base-only install without DLC");
                Check(launcher==Path.Combine(gameFolder,"Boom Boom Rocket.exe")&&File.Exists(launcher),"Installed play executable is Game/Boom Boom Rocket.exe");
                Check(File.ReadAllText(Path.Combine(install,"Setup Boom Boom Rocket.exe"))=="installer preservation sentinel","Installer in portable root is not replaced");
                Check(File.Exists(Path.Combine(data,"rexgpu-xenos.dll"))&&!File.Exists(Path.Combine(gameFolder,"rexgpu-xenos.dll")),"Runtime and GPU plug-in confined to Game/resources");
                Check(Directory.GetFileSystemEntries(install).Select(Path.GetFileName).OrderBy(n=>n)
                      .SequenceEqual(new[]{"Game","licenses","README.txt","Setup Boom Boom Rocket.exe"}.OrderBy(n=>n)),
                      "Installed release root contains only setup, README, licenses and Game");
                Check(Directory.GetFileSystemEntries(gameFolder).Select(Path.GetFileName).OrderBy(n=>n)
                      .SequenceEqual(new[]{"Boom Boom Rocket.exe","release-manifest.json","resources"}.OrderBy(n=>n)),
                      "Game contains only play executable, resources and release manifest");
                string manifestPath=Path.Combine(gameFolder,"release-manifest.json");
                var manifest=(Dictionary<string,object>)new JavaScriptSerializer().DeserializeObject(File.ReadAllText(manifestPath));
                Check(Convert.ToInt32(manifest["schemaVersion"])==1 && (string)manifest["version"]=="0.9.1" &&
                      (string)manifest["product"]=="Boom Boom Rocket XBLA Recomp" && (string)manifest["xboxTitleId"]=="5841086A",
                      "Release manifest is valid JSON with the expected release and title identity");
                Check((string)manifest["entryPoint"]=="Boom Boom Rocket.exe" && (string)manifest["resources"]=="resources" &&
                      Directory.Exists(Paths.Child(gameFolder,(string)manifest["gameAssets"])) &&
                      File.Exists(Paths.Child(gameFolder,(string)manifest["configuration"])) &&
                      (string)manifest["userData"]=="resources/userdata",
                      "Manifest paths resolve relative to Game, not the setup root or working directory");
                Check((bool)manifest["requiresOriginalGamePackage"] && !(bool)manifest["bundlesOriginalGameContent"],
                      "Manifest records user-supplied game content, not bundled retail content");
                Check((string)manifest["supportedOriginalXexSha256"]==InstallEngine.SupportedXexSha256,
                      "Manifest identifies the same supported XEX revision as setup");
                string installedXex=Path.Combine(data,"assets","default.xex");
                InstallEngine.ValidateSupportedXex(installedXex);
                Check(Hash(installedXex).Replace("-","")==InstallEngine.SupportedXexSha256,
                      "Accepted retail XEX matches the fixed-layout recompilation revision");
                byte[] changedXex=File.ReadAllBytes(installedXex);
                int optionalCount=(changedXex[20]<<24)|(changedXex[21]<<16)|(changedXex[22]<<8)|changedXex[23];
                int entryValue=-1;
                for(int i=0;i<optionalCount;i++) {
                    int p=24+8*i;
                    if(changedXex[p]==0 && changedXex[p+1]==1 && changedXex[p+2]==1 && changedXex[p+3]==0) { entryValue=p+4; break; }
                }
                Check(entryValue>=0,"Supported XEX contains its expected entry-point header");
                changedXex[entryValue]=0x83;
                string incompatibleXex=Path.Combine(root,"unsupported.xex");File.WriteAllBytes(incompatibleXex,changedXex);
                Reject(()=>InstallEngine.ValidateSupportedXex(incompatibleXex),"XEX2 with incompatible guest entry point is rejected");
                // Also guard DLC/update-only runs against an unsupported existing base.
                byte[] originalXex=File.ReadAllBytes(installedXex);File.WriteAllBytes(installedXex,changedXex);
                try { Reject(()=>engine.Install(null,null,install,Quiet,CancellationToken.None),"Update rejects an unsupported existing base before committing files"); }
                finally { File.WriteAllBytes(installedXex,originalXex); }
                var manifestFiles=((object[])manifest["files"]).Cast<Dictionary<string,object>>().ToArray();
                Check(manifestFiles.Length==6 && manifestFiles.All(f=>
                      File.Exists(Paths.Child(gameFolder,(string)f["path"])) &&
                      Hash(Paths.Child(gameFolder,(string)f["path"])).Replace("-","")==((string)f["sha256"]).ToUpperInvariant() &&
                      new FileInfo(Paths.Child(gameFolder,(string)f["path"])).Length==Convert.ToInt64(f["size"])),
                      "Every immutable manifest file resolves and matches its size and SHA256");
                Check(manifestFiles.All(f=>!Path.IsPathRooted((string)f["path"]) &&
                      !((string)f["path"]).Contains("..") && !((string)f["path"]).EndsWith(".xex")),
                      "Manifest contains no absolute installation paths or original game files");
                Check(File.Exists(Path.Combine(install,"README.txt")) && Directory.Exists(Path.Combine(install,"licenses")) &&
                      !File.Exists(Path.Combine(gameFolder,"README.txt")) && !Directory.Exists(Path.Combine(gameFolder,"licenses")),
                      "README and third-party licenses remain in the release root");
                string manifestHash=Hash(manifestPath);
                string ini=Path.Combine(data,"assets","UI","Text","English.ini");
                string text=File.ReadAllText(ini,Encoding.Unicode);
                Check(text.Split(new[]{"IDS_PC_DISPLAY_MODE ="},StringSplitOptions.None).Length==2,"PC settings labels appear once");
                Check(text.Split(new[]{"IDS_PC_KEYBOARD_CONTROLS ="},StringSplitOptions.None).Length==2,
                      "New keyboard title and help labels installed once");
                Check(text.Split(new[]{"IDS_PC_TIMING_CALIBRATION ="},StringSplitOptions.None).Length==2 &&
                      text.Contains("IDS_PC_TIMING_VALUE_0 = -200 ms") && text.Contains("IDS_PC_TIMING_VALUE_40 = +200 ms"),
                      "Calibration title and signed offset labels installed once");
                Check(text.Split(new[]{"IDS_PC_PRESS_ANY_KEY = Press Any Key"},StringSplitOptions.None).Length==2,
                      "Native any-key title prompt is installed exactly once");
                string iniHash=Hash(ini);InstallEngine.PatchLabels(Path.Combine(data,"assets"));
                Check(Hash(ini)==iniHash,"Label patch is idempotent");
                // Simulate the prior 16-label release, then upgrade it in place.
                string olderLabels=text.Substring(0,text.IndexOf("IDS_PC_KEYBOARD_CONTROLS =",StringComparison.Ordinal));
                File.WriteAllText(ini,olderLabels,Encoding.Unicode);
                InstallEngine.PatchLabels(Path.Combine(data,"assets"));
                Check(Hash(ini)==iniHash,"Existing 16-label installs upgrade without shifting IDs or changing earlier text");
                string stage2Labels=text.Substring(0,text.IndexOf("IDS_PC_TIMING_CALIBRATION =",StringComparison.Ordinal));
                File.WriteAllText(ini,stage2Labels,Encoding.Unicode);
                InstallEngine.PatchLabels(Path.Combine(data,"assets"));
                Check(Hash(ini)==iniHash,"Existing 26-label installs upgrade without shifting keyboard or display IDs");
                string stage4Labels=text.Substring(0,text.IndexOf("IDS_PC_PRESS_ANY_KEY =",StringComparison.Ordinal));
                File.WriteAllText(ini,stage4Labels,Encoding.Unicode);
                InstallEngine.PatchLabels(Path.Combine(data,"assets"));
                Check(Hash(ini)==iniHash,"Existing 78-label installs append any-key prompt without shifting earlier IDs");
                string save=Path.Combine(data,"userdata","5841086A","profile","User","setup-test-save");Directory.CreateDirectory(Path.GetDirectoryName(save));File.WriteAllText(save,"preserve this save");
                string config=Path.Combine(data,"boom_boom_rocket.toml");File.AppendAllText(config,"\r\n# settings preservation sentinel\r\n");string configHash=Hash(config);
                engine.Install(null,dlc,install,Quiet,CancellationToken.None);
                Check(InstallEngine.HasDlc(install),"DLC-only install into existing base");
                Check(File.ReadAllText(save)=="preserve this save"&&Hash(config)==configHash&&Hash(ini)==iniHash,"Saves, config and base assets preserved");
                string content=Path.Combine(data,"userdata","0000000000000000","5841086A","00000002");
                Check(Directory.GetDirectories(content).Length==1,"One DLC entry installed");
                string header=Directory.GetFiles(Path.Combine(data,"userdata","0000000000000000","5841086A","Headers","00000002")).Single();
                byte[] b=File.ReadAllBytes(header);Check(b.Length==0x14C&&b[3]==1&&b[7]==2&&b[0x140]==0x58&&b[0x141]==0x41&&b[0x142]==8&&b[0x143]==0x6A&&BitConverter.ToUInt32(b,0x148)==1,"DLC header layout and license match runtime ABI");
                string renamed=Path.Combine(root,"My renamed DLC package");File.Copy(dlc,renamed);
                engine.Install(null,renamed,install,Quiet,CancellationToken.None);
                Check(Directory.GetDirectories(content).Length==1,"Renamed/repeated DLC import does not duplicate pack");
                Reject(()=>{using(var p=new StfsPackage(dlc,false)){}},"DLC rejected in base field");
                Reject(()=>{using(var p=new StfsPackage(game,true)){}},"Base rejected in DLC field");
                byte[] invalid=new byte[0xA000];using(var s=File.OpenRead(game))s.Read(invalid,0,invalid.Length);invalid[0x360]=0;
                string wrong=Path.Combine(root,"wrong-title");File.WriteAllBytes(wrong,invalid);
                Reject(()=>engine.Install(wrong,null,Path.Combine(root,"Wrong"),Quiet,CancellationToken.None),"Wrong title rejected before destination changes");
                Check(!Directory.Exists(Path.Combine(root,"Wrong")),"Rejected package creates no installation");
                invalid[0x360]=0x58;File.WriteAllBytes(wrong,invalid);
                Reject(()=>{using(var p=new StfsPackage(wrong,false)){}},"Truncated STFS rejected");
                Reject(()=>Paths.Child(root,"..\\outside"),"Path traversal rejected");
                Reject(()=>Paths.Child(root,"safe.txt:stream"),"Alternate data stream path rejected");
                string extended=Paths.Extended(root);
                Check(Paths.Child(extended,"Game/licenses/notice.txt")==Path.Combine(extended,"Game","licenses","notice.txt"),"ZIP slashes normalized in extended Windows paths");
                Reject(()=>Paths.Child(extended,"Game/../outside"),"Forward-slash traversal rejected for extended paths");
                string cancelRoot=Path.Combine(root,"Cancelled");
                using(var cancel=new CancellationTokenSource()) {
                    bool cancelled=false;
                    try{engine.Install(game,dlc,cancelRoot,(p,t)=>{if(p>=20)cancel.Cancel();},cancel.Token);}catch(OperationCanceledException){cancelled=true;}
                    Check(cancelled&&!File.Exists(Path.Combine(cancelRoot,"Game","Boom Boom Rocket.exe"))&&!Directory.EnumerateFileSystemEntries(cancelRoot).Any(),"Cancellation removes staging and commits nothing");
                }
                string exe=Path.Combine(data,"boom_boom_rocket.exe"),exeHash=Hash(exe),launcherHash=Hash(launcher);
                using(var fileLock=new FileStream(Path.Combine(data,"rexruntime.dll"),FileMode.Open,FileAccess.Read,FileShare.Read))
                    Reject(()=>engine.Install(null,null,install,Quiet,CancellationToken.None),"Locked runtime aborts installation");
                Check(Hash(exe)==exeHash&&Hash(launcher)==launcherHash&&Hash(config)==configHash&&File.ReadAllText(save)=="preserve this save","Failed commit rolls back launcher/runtime and preserves user data");
                Check(Hash(manifestPath)==manifestHash,"Failed commit restores the previous release manifest");
                engine.Install(null,null,install,Quiet,CancellationToken.None);
                Check(Hash(config)==configHash&&!Directory.GetDirectories(install,".bbr-setup-*").Any(),"Repeat runtime update preserves config and cleans staging");
                Check(Hash(manifestPath)==manifestHash && Hash(exe)==exeHash,"Repeated update retains correct manifest and resource runtime");
                Check(Hash(game)==gameHash&&Hash(dlc)==dlcHash,"Original game and DLC packages remain unchanged");
                string occupied=Path.Combine(root,"Occupied");Directory.CreateDirectory(Path.Combine(occupied,"Game"));
                File.WriteAllText(Path.Combine(occupied,"Setup Boom Boom Rocket.exe"),"setup");
                File.WriteAllText(Path.Combine(occupied,"Game","unrelated.txt"),"keep me");
                Reject(()=>engine.Install(game,null,occupied,Quiet,CancellationToken.None),"Unrelated nonempty Game directory is protected");
                Check(File.ReadAllText(Path.Combine(occupied,"Game","unrelated.txt"))=="keep me","Rejected destination remains unchanged");
                string legacy=Path.Combine(root,"Legacy");Directory.CreateDirectory(Path.Combine(legacy,"Game"));
                File.WriteAllText(Path.Combine(legacy,"Setup Boom Boom Rocket.exe"),"old setup");
                File.WriteAllText(Path.Combine(legacy,"Game","boom_boom_rocket.exe"),"old runtime");
                File.WriteAllText(Path.Combine(legacy,"Game","old-save"),"keep old save");
                Reject(()=>engine.Install(game,null,legacy,Quiet,CancellationToken.None),"Previous release layout is rejected instead of partially migrated");
                Check(File.ReadAllText(Path.Combine(legacy,"Game","old-save"))=="keep old save" &&
                      !Directory.Exists(Path.Combine(legacy,"Game","resources")),"Earlier installation and saves remain untouched");
                // Run the real launcher against a harmless probe after relocating it.
                // This verifies relative paths without launching the actual game in test automation.
                string probe=Paths.Child(root,"Launcher origin");Directory.CreateDirectory(Path.Combine(probe,"Game","resources","assets"));
                File.Copy(launcher,Path.Combine(probe,"Game","Boom Boom Rocket.exe"));
                File.Copy(args[3],Path.Combine(probe,"Game","resources","boom_boom_rocket.exe"));
                File.WriteAllText(Path.Combine(probe,"Game","resources","assets","default.xex"),"probe only");
                string moved=Paths.Child(root,"Launcher moved with spaces");Directory.Move(probe,moved);
                using(var process=Process.Start(new ProcessStartInfo(Path.Combine(moved,"Game","Boom Boom Rocket.exe")){WorkingDirectory=root,UseShellExecute=false})) {
                    Check(process.WaitForExit(10000)&&process.ExitCode==0,"Relocated launcher starts and exits without a lingering process");
                }
                string result=Path.Combine(moved,"Game","resources","launch-probe.txt");
                var timer=Stopwatch.StartNew();while(!File.Exists(result)&&timer.ElapsedMilliseconds<10000)Thread.Sleep(50);
                string[] paths=File.ReadAllLines(result);
                Check(paths.Length==2&&paths.All(p=>p.TrimEnd('\\')==Path.Combine(moved,"Game","resources")),"Launcher uses relocated Game/resources directory, not caller working directory");
                Console.WriteLine("SUCCESS: "+checks+" checks passed.");return 0;
            }catch(Exception e){Console.Error.WriteLine(e);return 1;}
        }
    }
}
