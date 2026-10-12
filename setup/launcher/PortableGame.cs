using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Runtime.InteropServices;
using System.Web.Script.Serialization;

namespace Bbr.Setup {
    // Project-owned importer shared by first launch and management. Runtime
    // binaries are ordinary files in resources, never an embedded EXE payload.
    internal sealed class ImportManifest {
        public int schema;
        public string xexSha256;
        public int assetPatchVersion;
        public string[] dlc=new string[0];
    }
    internal sealed class ImportMove {
        public string source,target,backup;
        public bool existed;
    }
    internal sealed class ImportJournal {
        public string stage;
        public bool committed;
        public List<ImportMove> moves=new List<ImportMove>();
    }
    internal sealed class PortableGame {
        internal const int AssetPatchVersion=1;
        internal readonly string Root,Resources,Assets;
        readonly JavaScriptSerializer json=new JavaScriptSerializer();
        [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
        static extern bool MoveFileEx(string source,string target,uint flags);
#if BBR_LAUNCHER_TESTS
        internal Action<int> AfterMoveForTest;
#endif
        internal PortableGame(string root) {
            Root=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar);
            if(Root.Length<4)throw new IOException("Extract the launcher into its own portable folder, not a drive root.");
            Resources=Path.Combine(Root,"resources");Assets=Path.Combine(Resources,"assets");
            Paths.CheckParents(Resources);
        }
        string ManifestPath {get{return Path.Combine(Resources,"import-manifest.json");}}
        string JournalPath {get{return Path.Combine(Resources,".import-journal.json");}}
        static bool Exists(string path) {return File.Exists(path)||Directory.Exists(path);}
        static void Move(string source,string target) {
            Directory.CreateDirectory(Path.GetDirectoryName(target));
            if(Directory.Exists(source))Directory.Move(source,target);else File.Move(source,target);
        }
        static void Remove(string path) {
            Paths.CheckParents(path);
            if(Directory.Exists(path))Directory.Delete(path,true);else if(File.Exists(path))File.Delete(path);
        }
        void WriteJson(string path,object value) {
            string temp=path+".tmp";Paths.CheckParents(temp);
            using(var file=new FileStream(temp,FileMode.Create,FileAccess.Write,FileShare.None)) {
                byte[] bytes=Encoding.UTF8.GetBytes(json.Serialize(value));file.Write(bytes,0,bytes.Length);file.Flush(true);
            }
            // ReplaceFile can fail to remove OneDrive-backed targets even when
            // writable. Same-directory MoveFileEx is atomic, with write-through.
            for(int attempt=0;;attempt++) {
                if(MoveFileEx(temp,path,1|8))break;
                int error=Marshal.GetLastWin32Error();
                if(attempt>=4||(error!=32&&error!=33))throw new IOException("Could not save import journal/manifest.",new System.ComponentModel.Win32Exception(error));
                Thread.Sleep(20);
            }
        }
        FileStream Lock() {
            Directory.CreateDirectory(Resources);Paths.CheckParents(Resources);
            return new FileStream(Path.Combine(Resources,".import.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None,1,FileOptions.DeleteOnClose);
        }
        internal bool Ready() {
            try {
                if(File.Exists(JournalPath))return false;
                var manifest=json.Deserialize<ImportManifest>(File.ReadAllText(ManifestPath));
                return manifest!=null && manifest.schema==1 && manifest.xexSha256==InstallEngine.SupportedXexSha256 &&
                    InstallEngine.RequiredAssets.All(p=>File.Exists(Path.Combine(Assets,p)));
            } catch(IOException){return false;} catch(ArgumentException){return false;} catch(InvalidOperationException){return false;}
        }
        internal static void ValidatePackage(string path,bool dlc) {
            using(var package=new StfsPackage(path,dlc)) {
                if(!dlc) using(var sha=SHA256.Create()) {
                    string hash=BitConverter.ToString(sha.ComputeHash(package.ReadFile("default.xex"))).Replace("-","");
                    if(hash!=InstallEngine.SupportedXexSha256)throw new InvalidDataException("Unsupported Boom Boom Rocket executable revision. Your installed game has not been changed.");
                }
            }
        }
        // Automatic lookup is top-level only. Explicit folder selection permits
        // bounded nesting for normal Xbox content directories; never scan drives.
        internal static List<string> FindPackages(string folder,bool dlc,bool nested,CancellationToken cancel=default(CancellationToken)) {
            var result=new List<string>();int inspected=0;
            var queue=new Queue<Tuple<string,int>>();queue.Enqueue(Tuple.Create(Path.GetFullPath(folder),0));
            while(queue.Count>0) {
                cancel.ThrowIfCancellationRequested();
                var current=queue.Dequeue();Paths.CheckParents(current.Item1);
                foreach(string path in Directory.EnumerateFiles(current.Item1)) {
                    cancel.ThrowIfCancellationRequested();
                    if(++inspected>2048)throw new IOException("Too many files. Choose a smaller package folder.");
                    try {
                        using(var file=File.OpenRead(path)) {
                            byte[] magic=new byte[4];if(file.Read(magic,0,4)!=4)continue;
                            string text=Encoding.ASCII.GetString(magic);
                            if(text!="LIVE"&&text!="PIRS"&&text!="CON ")continue;
                        }
                        ValidatePackage(path,dlc);result.Add(path);
                    }catch(InvalidDataException){}catch(EndOfStreamException){}catch(UnauthorizedAccessException){}catch(IOException){}
                }
                if(nested&&current.Item2<4)foreach(string child in Directory.EnumerateDirectories(current.Item1)) {
                    if((File.GetAttributes(child)&FileAttributes.ReparsePoint)!=0 ||
                       Path.GetFileName(child).Equals("resources",StringComparison.OrdinalIgnoreCase))continue;
                    queue.Enqueue(Tuple.Create(child,current.Item2+1));
                    if(queue.Count>256)throw new IOException("Too many folders. Choose the package folder directly.");
                }
            }
            return result.OrderBy(p=>p,StringComparer.OrdinalIgnoreCase).ToList();
        }
        string NewStage() {string stage=Path.Combine(Resources,".import-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);return stage;}
        ImportManifest CurrentManifest() {
            if(File.Exists(ManifestPath)) {
                try {var m=json.Deserialize<ImportManifest>(File.ReadAllText(ManifestPath));if(m!=null&&m.schema==1&&m.xexSha256==InstallEngine.SupportedXexSha256)return m;}catch(ArgumentException){}catch(InvalidOperationException){}
            }
            return NewManifest();
        }
        static ImportManifest NewManifest() {return new ImportManifest{schema=1,xexSha256=InstallEngine.SupportedXexSha256,assetPatchVersion=AssetPatchVersion};}
        internal static void CopyTree(string source,string target,CancellationToken cancel) {
            Paths.CheckParents(source);Paths.CheckParents(target);Directory.CreateDirectory(target);
            foreach(string path in Directory.EnumerateFiles(source)) {cancel.ThrowIfCancellationRequested();Paths.CheckParents(path);File.Copy(path,Path.Combine(target,Path.GetFileName(path)),false);}
            foreach(string path in Directory.EnumerateDirectories(source)) {cancel.ThrowIfCancellationRequested();CopyTree(path,Path.Combine(target,Path.GetFileName(path)),cancel);}
        }
        void Commit(string stage,Dictionary<string,string> replacements,CancellationToken cancel) {
            var journal=new ImportJournal{stage=Path.GetFileName(stage)};
            int index=0;
            foreach(var pair in replacements) {
                string target=Paths.Child(Resources,pair.Key);
                journal.moves.Add(new ImportMove{source=pair.Value.Substring(stage.Length+1),target=pair.Key,backup="backup\\"+(index++),existed=Exists(target)});
            }
            cancel.ThrowIfCancellationRequested();WriteJson(JournalPath,journal);
            // Cancellation stops before commit only. Journal is durable before
            // every rename, allowing recovery even if power/process is lost.
            try {
                index=0;
                foreach(var op in journal.moves) {
                    string target=Paths.Child(Resources,op.target),source=Paths.Child(stage,op.source),backup=Paths.Child(stage,op.backup);
                    if(op.existed)Move(target,backup);
                    Move(source,target);
#if BBR_LAUNCHER_TESTS
                    if(AfterMoveForTest!=null)AfterMoveForTest(++index);
#endif
                }
                journal.committed=true;WriteJson(JournalPath,journal);
            }catch {RecoverLocked();throw;}
            RecoverLocked();
        }
        void RecoverLocked() {
            if(!File.Exists(JournalPath))return;
            var journal=json.Deserialize<ImportJournal>(File.ReadAllText(JournalPath));
            if(journal==null||journal.stage==null||!journal.stage.StartsWith(".import-")||journal.stage.Length!=40)throw new IOException("Invalid import journal; keep resources intact and report this error.");
            string stage=Paths.Child(Resources,journal.stage);
            foreach(var op in journal.moves) {Paths.Child(Resources,op.target);Paths.Child(stage,op.source);Paths.Child(stage,op.backup);}
            if(!journal.committed) foreach(var op in journal.moves.AsEnumerable().Reverse()) {
                string target=Paths.Child(Resources,op.target),source=Paths.Child(stage,op.source),backup=Paths.Child(stage,op.backup);
                if(Exists(backup)) {if(Exists(target))Remove(target);Move(backup,target);}
                else if(!op.existed&&!Exists(source)&&Exists(target))Remove(target);
            }
            if(Directory.Exists(stage))Remove(stage);
            File.Delete(JournalPath);
        }
        internal void Recover() {
            using(Lock()) {
                RecoverLocked();
                foreach(string stage in Directory.GetDirectories(Resources,".import-*")) {
                    string name=Path.GetFileName(stage);Guid id;
                    if(name.Length==40&&Guid.TryParseExact(name.Substring(8),"N",out id))Remove(stage);
                }
            }
        }
        internal void CheckNotRunning() {
            foreach(var process in Process.GetProcessesByName("boom_boom_rocket"))using(process) {
                try {if(string.Equals(Path.GetDirectoryName(process.MainModule.FileName),Resources,StringComparison.OrdinalIgnoreCase))throw new IOException("Close Boom Boom Rocket before managing game files.");}
                catch(System.ComponentModel.Win32Exception){throw new IOException("Could not check the running game. Close it before importing.");}
            }
        }
        void FreshConfig(string stage,Dictionary<string,string> replacements) {
            if(File.Exists(Path.Combine(Resources,"boom_boom_rocket.toml")))return;
            string config=Path.Combine(stage,"config.toml");File.Copy(Path.Combine(Resources,"defaults.toml"),config);
            PlayerNames.AssignFreshConfig(config);replacements.Add("boom_boom_rocket.toml",config);
        }
        void AddManifest(string stage,Dictionary<string,string> replacements,ImportManifest manifest) {
            manifest.assetPatchVersion=AssetPatchVersion;
            string path=Path.Combine(stage,"manifest.json");WriteJson(path,manifest);replacements.Add("import-manifest.json",path);
        }
        internal void Import(string packagePath,bool dlc,Action<int,string> report,CancellationToken cancel) {
            using(Lock()) {
                CheckNotRunning();
                RecoverLocked();if(dlc&&!Ready())throw new IOException("Import the base game before DLC.");
                ValidatePackage(packagePath,dlc);cancel.ThrowIfCancellationRequested();
                string stage=NewStage();
                try {
                    var replacements=new Dictionary<string,string>();var manifest=CurrentManifest();
                    using(var package=new StfsPackage(packagePath,dlc)) {
                        string data=Path.Combine(stage,"data");
                        package.Extract(data,(p,name)=>report(p*90/100,"Importing "+name),cancel);
                        if(!dlc) {
                            if(!InstallEngine.RequiredAssets.All(p=>File.Exists(Path.Combine(data,p))))throw new InvalidDataException("The package is missing required game files.");
                            InstallEngine.ValidateSupportedXex(Path.Combine(data,"default.xex"));InstallEngine.PatchLabels(data);
                            replacements.Add("assets",data);
                        }else {
                            string content="userdata\\0000000000000000\\5841086A\\";
                            string installed=Path.Combine(Resources,content,"00000002");
                            if(Directory.Exists(installed)) {
                                string existing=Directory.GetDirectories(installed).Select(Path.GetFileName).FirstOrDefault(n=>n.StartsWith(package.ContentId,StringComparison.OrdinalIgnoreCase)&&(n.Length==40||n.Length==42));
                                if(existing!=null)package.ContentName=existing;
                            }
                            string header=Path.Combine(stage,"dlc.header");package.WriteDlcHeader(header);
                            replacements.Add(content+"00000002\\"+package.ContentName,data);
                            replacements.Add(content+"Headers\\00000002\\"+package.ContentName+".header",header);
                            manifest.dlc=(manifest.dlc??new string[0]).Concat(new[]{package.ContentId}).Distinct().ToArray();
                        }
                    }
                    FreshConfig(stage,replacements);AddManifest(stage,replacements,manifest);
                    report(95,"Finishing import...");Commit(stage,replacements,cancel);report(100,"Ready to play.");
                }finally {if(!File.Exists(JournalPath)&&Directory.Exists(stage))Remove(stage);}
            }
        }
        internal string LegacyResources(string selectedRoot) {
            foreach(string candidate in new[]{Path.Combine(selectedRoot,"resources"),Path.Combine(selectedRoot,"Game","resources"),Path.Combine(selectedRoot,"Game"),selectedRoot})
                if(InstallEngine.RequiredAssets.All(p=>File.Exists(Path.Combine(candidate,"assets",p))))return Path.GetFullPath(candidate);
            return null;
        }
        // Copies instead of moving: old portable releases remain recoverable.
        internal bool Adopt(string sourceResources,Action<int,string> report,CancellationToken cancel) {
            using(Lock()) {
                CheckNotRunning();
                RecoverLocked();if(Ready())return true;
                if(sourceResources==null)return false;Paths.CheckParents(sourceResources);
                InstallEngine.ValidateSupportedXex(Path.Combine(sourceResources,"assets","default.xex"));
                string stage=NewStage();
                try {
                    var replacements=new Dictionary<string,string>();string data=Path.Combine(stage,"assets");
                    report(10,"Reusing existing game assets...");CopyTree(Path.Combine(sourceResources,"assets"),data,cancel);InstallEngine.PatchLabels(data);replacements.Add("assets",data);
                    if(!string.Equals(Resources,sourceResources,StringComparison.OrdinalIgnoreCase)) {
                        foreach(string name in new[]{"userdata","boom_boom_rocket.toml"}) {
                            string source=Path.Combine(sourceResources,name),target=Path.Combine(Resources,name),copy=Path.Combine(stage,name);
                            if(Exists(source)&&!Exists(target)) {if(Directory.Exists(source))CopyTree(source,copy,cancel);else File.Copy(source,copy);replacements.Add(name,copy);}
                            else if(Exists(source)&&Exists(target))throw new IOException("Both locations contain "+name+". Choose a fresh launcher folder to migrate without overwriting data.");
                        }
                    }
                    if(!replacements.ContainsKey("boom_boom_rocket.toml"))FreshConfig(stage,replacements);
                    var manifest=NewManifest();string dlcRoot=Path.Combine(sourceResources,"userdata","0000000000000000","5841086A","00000002");
                    if(Directory.Exists(dlcRoot))manifest.dlc=Directory.GetDirectories(dlcRoot).Select(Path.GetFileName).Select(n=>n.Length>=40?n.Substring(0,40):n).ToArray();
                    AddManifest(stage,replacements,manifest);Commit(stage,replacements,cancel);report(100,"Ready to play.");return true;
                }finally {if(!File.Exists(JournalPath)&&Directory.Exists(stage))Remove(stage);}
            }
        }
        internal void Prepare(Action<int,string> report,CancellationToken cancel) {
            Recover();
            if(!Ready()) {string legacy=LegacyResources(Root);if(legacy!=null)Adopt(legacy,report,cancel);return;}
            var manifest=CurrentManifest();
            if(manifest.assetPatchVersion!=AssetPatchVersion)AdoptUpgrade(report,cancel);
        }
        void AdoptUpgrade(Action<int,string> report,CancellationToken cancel) {
            using(Lock()) {
                CheckNotRunning();
                RecoverLocked();string stage=NewStage();
                try {
                    var replacements=new Dictionary<string,string>();string data=Path.Combine(stage,"assets");
                    report(10,"Updating imported text patches...");CopyTree(Assets,data,cancel);InstallEngine.PatchLabels(data);replacements.Add("assets",data);
                    AddManifest(stage,replacements,CurrentManifest());Commit(stage,replacements,cancel);report(100,"Ready to play.");
                }finally {if(!File.Exists(JournalPath)&&Directory.Exists(stage))Remove(stage);}
            }
        }
        internal void ApplyExtras(bool fullscreen,bool shortcut,string shortcutDirectory=null) {
            if(!fullscreen&&!shortcut)return;
            using(Lock()) {
                CheckNotRunning();RecoverLocked();
                if(!Ready())throw new IOException("Import your game package before applying extras.");
                if(fullscreen) {
                    string config=Path.Combine(Resources,"boom_boom_rocket.toml");
                    string text=File.ReadAllText(config);
                    var pattern=new System.Text.RegularExpressions.Regex(@"(?m)^[ \t]*fullscreen[ \t]*=[ \t]*(true|false)[^\r\n]*");
                    text=pattern.IsMatch(text)?pattern.Replace(text,"fullscreen = true"):"fullscreen = true\r\n"+text;
                    string stage=NewStage();
                    try{string replacement=Path.Combine(stage,"config.toml");File.WriteAllText(replacement,text,new UTF8Encoding(false));Commit(stage,new Dictionary<string,string>{{"boom_boom_rocket.toml",replacement}},CancellationToken.None);}
                    finally{if(!File.Exists(JournalPath)&&Directory.Exists(stage))Remove(stage);}
                }
                if(shortcut)CreateShortcut(shortcutDirectory??Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory));
            }
        }
        void CreateShortcut(string directory) {
            // Use Windows' built-in shortcut API, targeting the portable root
            // launcher (not the internal runtime). Never replace an existing link.
            string link=Path.Combine(directory,"Boom Boom Rocket.lnk");
            Paths.CheckParents(link);
            if(File.Exists(link))throw new IOException("A Boom Boom Rocket desktop shortcut already exists and was not overwritten. Uncheck Create desktop shortcut to play.");
            Directory.CreateDirectory(directory);
            object shell=null,shortcut=null;
            try {
                Type type=Type.GetTypeFromProgID("WScript.Shell");
                if(type==null)throw new IOException("Windows shortcut creation is unavailable. Uncheck Create desktop shortcut to play.");
                shell=Activator.CreateInstance(type);
                shortcut=type.InvokeMember("CreateShortcut",System.Reflection.BindingFlags.InvokeMethod,null,shell,new object[]{link});
                Type shortcutType=shortcut.GetType();
                string launcher=Path.Combine(Root,"Boom Boom Rocket.exe");
                foreach(var value in new Dictionary<string,string>{{"TargetPath",launcher},{"WorkingDirectory",Root},{"IconLocation",launcher+",0"},{"Description","Boom Boom Rocket"}})
                    shortcutType.InvokeMember(value.Key,System.Reflection.BindingFlags.SetProperty,null,shortcut,new object[]{value.Value});
                shortcutType.InvokeMember("Save",System.Reflection.BindingFlags.InvokeMethod,null,shortcut,null);
            }finally{if(shortcut!=null)Marshal.FinalReleaseComObject(shortcut);if(shell!=null)Marshal.FinalReleaseComObject(shell);}
        }
        internal void Launch() {
            using(Lock()) {
                RecoverLocked();
                if(!Ready())throw new IOException("Select your original game package first.");
                string runtime=Path.Combine(Resources,"boom_boom_rocket.exe");
                if(!File.Exists(runtime))throw new IOException("Runtime is missing. Extract the complete download, including resources.");
                Process.Start(new ProcessStartInfo(runtime){WorkingDirectory=Resources,UseShellExecute=false});
            }
        }
    }
}
