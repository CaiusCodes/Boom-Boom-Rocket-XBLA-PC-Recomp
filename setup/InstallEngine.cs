using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

namespace Bbr.Setup {
    internal sealed class InstallEngine {
        static InstallEngine() {
            // Xbox content nesting plus transaction staging can exceed MAX_PATH.
            // .NET 4.8 uses extended Windows paths without a machine setting change.
            AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling",false);
            AppContext.SetSwitch("Switch.System.IO.BlockLongPaths",false);
        }
        internal static readonly string[] RequiredAssets={"default.xex","Content\\contents.txt","UI\\Text\\English.ini","Textures\\CityTextures.xpr"};
        // Setup embeds an already-recompiled runtime. Its function mappings and
        // native patches use this revision's fixed code/data addresses; Title ID
        // alone cannot establish compatibility. This identifies the one tested
        // revision, not the STFS container (repacked/renamed containers are OK).
        internal const string SupportedXexSha256="B1BC45589CEC79E375E23FC48C122FA5902A2DFD6E3C9C765064DCD9E1A8709B";
        internal static void ValidateSupportedXex(string path) {
            string hash;
            using(var input=File.OpenRead(path)) using(var sha=SHA256.Create())
                hash=BitConverter.ToString(sha.ComputeHash(input)).Replace("-", "");
            if(!string.Equals(hash,SupportedXexSha256,StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("This Boom Boom Rocket executable revision is not supported by the bundled PC runtime. Select the original, unmodified base-game package. Your existing files have not been changed.");
        }
        internal static bool HasGame(string root) { return RequiredAssets.All(p=>File.Exists(Path.Combine(root,"Game","resources","assets",p))); }
        internal static bool HasDlc(string root) { string p=Path.Combine(root,"Game","resources","userdata","0000000000000000","5841086A","00000002"); return Directory.Exists(p) && Directory.GetDirectories(p).Length>0; }
        internal static string DefaultDestination() {
            string root=AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
            return root;
        }
        internal string Install(string gamePath,string dlcPath,string destination,Action<int,string> report,CancellationToken cancel) {
            string root=Path.GetFullPath(destination).TrimEnd('\\');
            if(root.Length<4 || string.Equals(root,Path.GetPathRoot(root).TrimEnd('\\'),StringComparison.OrdinalIgnoreCase)) throw new IOException("Choose a dedicated folder for Boom Boom Rocket.");
            if(root.Length>120) throw new IOException("Choose a shorter installation path (120 characters or fewer).");
            Paths.CheckParents(root);
            // Do not partially migrate or overwrite earlier layouts and saves.
            if(File.Exists(Path.Combine(root,"Game","boom_boom_rocket.exe")))
                throw new IOException("This folder uses the previous release layout. Extract this release into a fresh folder. Your earlier installation and saves will remain untouched.");
            bool existing=HasGame(root);
            if(existing) ValidateSupportedXex(Path.Combine(root,"Game","resources","assets","default.xex"));
            // Reject unrelated folders. A previous BBR install can be updated without touching saves.
            if(Directory.Exists(root) && Directory.EnumerateFileSystemEntries(root).Any() && !File.Exists(Path.Combine(root,"Game","resources","boom_boom_rocket.exe")) && !File.Exists(Path.Combine(root,"Setup Boom Boom Rocket.exe")))
                throw new IOException("This folder contains other files. Choose an empty folder or your existing Boom Boom Rocket installation.");
            if(Directory.Exists(Path.Combine(root,"Game")) && Directory.EnumerateFileSystemEntries(Path.Combine(root,"Game")).Any() && !File.Exists(Path.Combine(root,"Game","resources","boom_boom_rocket.exe")))
                throw new IOException("The Game folder contains unrelated files. Extract this tool into a separate folder first.");
            if(!existing && string.IsNullOrWhiteSpace(gamePath)) throw new IOException("Select your base-game package first.");
            foreach(var process in Process.GetProcessesByName("boom_boom_rocket")) {
                using(process) { try { if(string.Equals(Path.GetDirectoryName(process.MainModule.FileName),Path.Combine(root,"Game","resources"),StringComparison.OrdinalIgnoreCase)) throw new IOException("Close Boom Boom Rocket before installing into this folder."); } catch(System.ComponentModel.Win32Exception) { throw new IOException("Close running Boom Boom Rocket instances before installing."); } }
            }
            string userRoot=root;
            root=Paths.Extended(root);
            StfsPackage game=null,dlc=null;
            try {
                report(1,"Checking your packages...");
                if(!existing) game=new StfsPackage(gamePath,false);
                if(!string.IsNullOrWhiteSpace(dlcPath)) dlc=new StfsPackage(dlcPath,true);
                cancel.ThrowIfCancellationRequested();
                Directory.CreateDirectory(root);
                string lockPath=Paths.Child(root,".bbr-setup.lock");
                using(var installLock=new FileStream(lockPath,FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None,1,FileOptions.DeleteOnClose)) {
                    string stage=Paths.Child(root,".bbr-setup-"+Guid.NewGuid().ToString("N"));
                    Directory.CreateDirectory(stage);
                    bool committed=false,rollbackFailed=false;
                    var moved=new List<string>(); var backups=new List<string>();
                    try {
                        string payload=Path.Combine(stage,"payload"); Directory.CreateDirectory(payload);
                        using(var zipStream=Assembly.GetExecutingAssembly().GetManifestResourceStream("Bbr.Payload")) {
                            if(zipStream==null) throw new IOException("The setup payload is missing. Download the complete setup EXE again.");
                            using(var zip=new ZipArchive(zipStream,ZipArchiveMode.Read)) {
                                foreach(var e in zip.Entries) {
                                    cancel.ThrowIfCancellationRequested();
                                    if(e.FullName.EndsWith("/")) continue;
                                    string path=Paths.Child(payload,e.FullName);
                                    Directory.CreateDirectory(Path.GetDirectoryName(path));
                                    using(var input=e.Open()) using(var output=new FileStream(path,FileMode.CreateNew)) input.CopyTo(output);
                                }
                            }
                        }
                        // Only a fresh config gets random names. Updates and DLC
                        // installs keep the entire existing user-owned config.
                        if(!File.Exists(Path.Combine(root,"Game","resources","boom_boom_rocket.toml")))
                            PlayerNames.AssignFreshConfig(Path.Combine(payload,"Game","resources","boom_boom_rocket.toml"));
                        report(7,"PC runtime ready.");
                        if(game!=null) {
                            string assets=Path.Combine(payload,"Game","resources","assets");
                            game.Extract(assets,(p,n)=>report(8+p*59/100,"Importing game: "+n),cancel);
                            if(!RequiredAssets.All(p=>File.Exists(Path.Combine(assets,p)))) throw new InvalidDataException("The base package is missing required Boom Boom Rocket files.");
                            byte[] xex=File.ReadAllBytes(Path.Combine(assets,"default.xex"));
                            if(xex.Length<4 || Encoding.ASCII.GetString(xex,0,4)!="XEX2") throw new InvalidDataException("The package does not contain a valid Xbox 360 XEX.");
                            ValidateSupportedXex(Path.Combine(assets,"default.xex"));
                            PatchLabels(assets);
                        }
                        if(dlc!=null) {
                            string content=Path.Combine("Game","resources","userdata","0000000000000000","5841086A");
                            string installedContent=Path.Combine(root,content,"00000002");
                            if(Directory.Exists(installedContent)) {
                                // Reuse the runtime's existing name (usually content ID + suffix),
                                // including when the source package has since been renamed.
                                string match=Directory.GetDirectories(installedContent).Select(Path.GetFileName).FirstOrDefault(n=>n.StartsWith(dlc.ContentId,StringComparison.OrdinalIgnoreCase) && (n.Length==40||n.Length==42));
                                if(match!=null)dlc.ContentName=match;
                            }
                            dlc.Extract(Paths.Child(payload,Path.Combine(content,"00000002",dlc.ContentName)),(p,n)=>report(68+p*24/100,"Importing DLC: "+n),cancel);
                            dlc.WriteDlcHeader(Paths.Child(payload,Path.Combine(content,"Headers","00000002",dlc.ContentName+".header")));
                        }
                        report(94,"Preparing your portable game...");
                        var files=Directory.GetFiles(payload,"*",SearchOption.AllDirectories);
                        foreach(string f in files) {
                            string relative=f.Substring(payload.Length+1);
                            // Runtime settings are user-owned, including when installing DLC later.
                            if(relative.Equals("Game\\resources\\boom_boom_rocket.toml",StringComparison.OrdinalIgnoreCase) && File.Exists(Paths.Child(root,relative))) continue;
                            Paths.Child(root,relative); cancel.ThrowIfCancellationRequested();
                        }
                        cancel.ThrowIfCancellationRequested();
                        report(96,"Finishing installation. Please keep this window open...");
                        // Commit cannot be cancelled mid-transaction; ordinary errors restore backups.
                        foreach(string f in files) {
                            string relative=f.Substring(payload.Length+1), target=Paths.Child(root,relative);
                            if(relative.Equals("Game\\resources\\boom_boom_rocket.toml",StringComparison.OrdinalIgnoreCase) && File.Exists(target)) continue;
                            Directory.CreateDirectory(Path.GetDirectoryName(target));
                            if(File.Exists(target)) {
                                string backup=Paths.Child(Path.Combine(stage,"backup"),relative);
                                Directory.CreateDirectory(Path.GetDirectoryName(backup)); File.Move(target,backup); backups.Add(relative);
                            }
                            File.Move(f,target); moved.Add(relative);
                        }
                        committed=true;
                        report(100,dlc!=null ? "Game and DLC are ready. Let the fireworks begin." : "You're ready to play. Add DLC any time with this setup.");
                        return Path.Combine(userRoot,"Game","Boom Boom Rocket.exe");
                    } catch {
                        if(!committed) {
                            try {
                                foreach(string relative in moved.AsEnumerable().Reverse()) File.Delete(Paths.Child(root,relative));
                                foreach(string relative in backups.AsEnumerable().Reverse()) File.Move(Paths.Child(Path.Combine(stage,"backup"),relative),Paths.Child(root,relative));
                            } catch(Exception recovery) { rollbackFailed=true; throw new IOException("Installation stopped and some files could not be restored. Recovery files were preserved in "+stage+". "+recovery.Message,recovery); }
                        }
                        throw;
                    } finally {
                        // Only remove this invocation's random staging directory, never the destination.
                        if(!rollbackFailed && stage.StartsWith(root+"\\.bbr-setup-",StringComparison.OrdinalIgnoreCase)) {
                            try { Paths.CheckParents(stage); Directory.Delete(stage,true); } catch { /* Preserve staging if locked; installed files remain valid. */ }
                        }
                    }
                }
            } finally { if(game!=null)game.Dispose(); if(dlc!=null)dlc.Dispose(); }
        }
        internal static void PatchLabels(string assets) {
            string block;
            using(var input=new StreamReader(Assembly.GetExecutingAssembly().GetManifestResourceStream("Bbr.PcStrings"),Encoding.UTF8)) block=input.ReadToEnd();
            foreach(string language in new[]{"English.ini","French.ini","german.ini","Italian.ini","spanish.ini"}) {
                string path=Path.Combine(assets,"UI","Text",language);
                if(!File.Exists(path)) continue;
                byte[] b=File.ReadAllBytes(path);
                if(b.Length<2 || b[0]!=255 || b[1]!=254 || b.Length%2!=0) throw new InvalidDataException("Unexpected text encoding: "+language);
                string text=Encoding.Unicode.GetString(b,2,b.Length-2);
                bool changed=false;
                foreach(string line in block.Split(new[]{'\r','\n'},StringSplitOptions.RemoveEmptyEntries)) {
                    int separator=line.IndexOf(" =",StringComparison.Ordinal);
                    if(separator<0) throw new InvalidDataException("Invalid PC label.");
                    if(text.Contains(line.Substring(0,separator)+" =")) continue;
                    text=text.TrimEnd('\0','\r','\n')+"\r\n"+line+"\r\n";
                    changed=true;
                }
                if(changed) File.WriteAllText(path,text,Encoding.Unicode);
            }
        }
    }
}
