using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Threading;
using System.Windows.Forms;
using Bbr.Setup;

// Private integration fixtures are generated ONLY from the user's local input.
// No package, XEX, retail file or translated source is stored in this test.
internal static class PortableLauncherTests {
    static int checks;
    static void Check(bool condition,string name){if(!condition)throw new Exception(name);checks++;Console.WriteLine("PASS: "+name);}
    static void Reject(Action action,string name){bool failed=false;try{action();}catch{failed=true;}Check(failed,name);}
    static string Hash(string file){using(var f=File.OpenRead(file))using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(f));}
    static void BE(byte[] bytes,int o,uint value){bytes[o]=(byte)(value>>24);bytes[o+1]=(byte)(value>>16);bytes[o+2]=(byte)(value>>8);bytes[o+3]=(byte)value;}
    static void LE24(byte[] bytes,int o,int value){bytes[o]=(byte)value;bytes[o+1]=(byte)(value>>8);bytes[o+2]=(byte)(value>>16);}
    static long Address(int block){long backing=block+((long)(block+0xAA)/0xAA);if(block>=0xAA)backing+=(block+0x70E4L)/0x70E4;if(block>=0x70E4)backing++;return 0xA000+backing*4096;}
    internal static void MakePackage(string source,string target,bool dlc) {
        var directories=Directory.GetDirectories(source,"*",SearchOption.AllDirectories).OrderBy(p=>p.Count(c=>c=='\\')).ThenBy(p=>p).ToArray();
        var files=Directory.GetFiles(source,"*",SearchOption.AllDirectories).OrderBy(p=>p).ToArray();
        var paths=directories.Concat(files).ToArray();int tables=(paths.Length+63)/64;
        var indices=new Dictionary<string,int>(StringComparer.OrdinalIgnoreCase);for(int i=0;i<paths.Length;i++)indices.Add(paths[i],i);
        byte[] header=new byte[0xA000];System.Text.Encoding.ASCII.GetBytes("PIRS").CopyTo(header,0);BE(header,0x340,0xA000);BE(header,0x344,dlc?2u:0xD0000u);BE(header,0x360,0x5841086A);header[0x37B]=1;header[0x37C]=(byte)tables;header[0x37D]=(byte)(tables>>8);
        for(int i=0;i<20;i++)header[0x32C+i]=(byte)(i+1);
        BE(header,0x238,1);BE(header,0x23C,1);
        System.Text.Encoding.BigEndianUnicode.GetBytes(dlc?"Boom Boom Rock Pack":"Boom Boom Rocket").CopyTo(header,0x411);
        using(var output=new FileStream(target,FileMode.CreateNew)) {
            output.Write(header,0,header.Length);int nextBlock=tables;
            for(int i=0;i<paths.Length;i++) {
                string p=paths[i];bool dir=Directory.Exists(p);long size=dir?0:new FileInfo(p).Length;int blocks=(int)((size+4095)/4096);
                byte[] entry=new byte[64];byte[] name=System.Text.Encoding.ASCII.GetBytes(Path.GetFileName(p));name.CopyTo(entry,0);entry[0x28]=(byte)(name.Length|(dir?128:64));
                int parent;string parentPath=Path.GetDirectoryName(p);if(!indices.TryGetValue(parentPath,out parent))parent=0xFFFF;
                entry[0x32]=(byte)(parent>>8);entry[0x33]=(byte)parent;LE24(entry,0x29,blocks);LE24(entry,0x2F,nextBlock);BE(entry,0x34,(uint)size);
                output.Position=Address(i/64)+(i%64)*64;output.Write(entry,0,64);
                if(!dir)using(var input=File.OpenRead(p)) {byte[] bytes=new byte[4096];while(input.Position<input.Length){int count=input.Read(bytes,0,bytes.Length);output.Position=Address(nextBlock++);output.Write(bytes,0,count);}}
            }
            for(int t=0;t<tables-1;t++){output.Position=0xA000+t*24+21;output.WriteByte(0);output.WriteByte(0);output.WriteByte((byte)(t+1));}
            output.SetLength(Math.Max(output.Length,Address(nextBlock)+4096));
        }
    }
    static void Defaults(string root){Directory.CreateDirectory(Path.Combine(root,"resources"));File.WriteAllText(Path.Combine(root,"resources","defaults.toml"),"vsync = true\n");}
    static void UiCase(string work,string package,string probe,string mode) {
        string root=Path.Combine(work,"ui-"+mode);Defaults(root);
        File.Copy(probe,Path.Combine(root,"resources","boom_boom_rocket.exe"));
        bool addon=mode.StartsWith("dlc-");
        string single=Path.Combine(work,"single-folder-"+mode);Directory.CreateDirectory(single);
        if(!File.Exists(Path.Combine(single,"package")))File.Copy(package,Path.Combine(single,"package"));
        if(mode=="auto"||mode=="multiple")File.Copy(package,Path.Combine(root,"first-package"));
        if(mode=="multiple")File.Copy(package,Path.Combine(root,"second-package"));
        var game=new PortableGame(root);
        if(addon)game.Import(Path.Combine(work,"packages","owned-game-package"),false,(p,s)=>{},CancellationToken.None);
        var form=new ImportForm(game,addon,null,addon);
        bool selected=false;Exception error=null;int ticks=0;
        using(var timer=new System.Windows.Forms.Timer{Interval=30}) {
            timer.Tick+=async(s,e)=>{
                if(++ticks>600){error=new Exception("UI test timed out: "+mode);form.Close();return;}
                if(selected||mode=="auto") {
                    var start=form.Controls.OfType<Button>().First(b=>b.Text=="Play");
                    if(!addon&&start.Enabled){Check(!File.Exists(Path.Combine(root,"resources","launch-probe.txt")),"first setup waits for Play instead of launching early");Check(!form.Controls.OfType<Button>().First(b=>b.Text=="Browse package").Enabled,"Browse Package stays greyed out after successful import");start.PerformClick();}
                    return;
                }
                var buttons=form.Controls.OfType<Button>().ToArray();
                if(!buttons.First(b=>b.Text==(addon?"Browse DLC package":"Browse package")).Enabled)return;
                if(mode=="file") {
                    Check(form.Controls.OfType<CheckBox>().Count()==2&&form.Controls.OfType<CheckBox>().All(b=>!b.Checked),"setup extras are both opt-in by default");
                    using(var preview=new System.Drawing.Bitmap(form.Width,form.Height)){form.DrawToBitmap(preview,new System.Drawing.Rectangle(0,0,form.Width,form.Height));preview.Save(Path.Combine(work,"first-launch-extras.png"),System.Drawing.Imaging.ImageFormat.Png);}
                }
                selected=true;
                try {
                    if(mode=="game-then-dlc") {
                        form.Controls.OfType<CheckBox>().First(b=>b.Text=="Start in fullscreen").Checked=true;
                        await form.SelectInput(package,false);
                        Check(game.Ready()&&!File.Exists(Path.Combine(root,"resources","launch-probe.txt")),"game-first import leaves setup open for optional DLC");
                        await form.SelectInput(Path.Combine(work,"dlc-package"),true);
                    }else if(mode.StartsWith("first-dlc")) {
                        Check(!buttons.Any(b=>b.Text=="Select folder"),"first launch has no Select Folder button");
                        string firstAddon=Path.Combine(work,"dlc-package");
                        await form.SelectInput(firstAddon,true);
                        Check(!game.Ready()&&!File.Exists(Path.Combine(root,"resources","launch-probe.txt")),"first-launch DLC selection queues without launching or requiring base assets");
                        if(mode=="first-dlc-skip")buttons.First(b=>b.Text=="Skip DLC").PerformClick();
                        await form.SelectInput(package,false);
                    }else if(mode=="multiple") {
                        var list=form.Controls.OfType<ListBox>().Single();
                        if(list.Items.Count!=2){selected=false;return;}
                        Check(!game.Ready(),"UI multiple matches wait for user choice");list.SelectedIndex=1;
                    }else if(mode=="file"||mode=="dlc-file")await form.SelectInput(package,addon);
                    else if(mode=="folder"||mode=="dlc-folder")await form.SelectInput(single,addon);
                    else {
                        var data=new DataObject(DataFormats.FileDrop,new[]{mode=="drop-folder"||mode=="dlc-drop-folder"?single:package});
                        var drop=new DragEventArgs(data,0,0,0,DragDropEffects.Copy,DragDropEffects.None);
                        typeof(Control).GetMethod("OnDragEnter",System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic).Invoke(form,new object[]{drop});
                        Check(drop.Effect==DragDropEffects.Copy,"file/folder drop accepted by launcher event");
                        typeof(Control).GetMethod("OnDragDrop",System.Reflection.BindingFlags.Instance|System.Reflection.BindingFlags.NonPublic).Invoke(form,new object[]{drop});
                    }
                }catch(Exception ex){error=ex;form.Close();}
            };
            timer.Start();Application.Run(form);
        }
        if(error!=null)throw error;
        string launched=Path.Combine(root,"resources","launch-probe.txt");
        for(int n=0;n<100&&!File.Exists(launched);n++)Thread.Sleep(10);
        Check(game.Ready()&&File.Exists(launched),"UI "+mode+" imports and launches "+(addon?"automatically after DLC import":"only after clicking Play"));
        Check(File.ReadAllLines(launched)[0].Equals(game.Resources,StringComparison.OrdinalIgnoreCase),"runtime launch uses resources as working directory");
        if(mode=="game-then-dlc")Check(File.ReadAllText(Path.Combine(game.Resources,"boom_boom_rocket.toml")).Contains("fullscreen = true"),"Play applies the selected fullscreen extra");
        if(mode.StartsWith("first-dlc")||mode=="game-then-dlc") {
            string content=Path.Combine(game.Resources,"userdata","0000000000000000","5841086A","00000002");
            Check(mode=="first-dlc-skip"?!Directory.Exists(content):Directory.GetFiles(content,"fixture.txt",SearchOption.AllDirectories).Length==1,"first-launch optional DLC is imported before launch, or cleanly skipped");
        }
    }
    [STAThread] static int Main(string[] args) {
        try {
            if(args[0]=="--crash") {
                var interrupted=new PortableGame(args[1]);interrupted.AfterMoveForTest=i=>Environment.Exit(77);
                interrupted.Import(args[2],false,(p,s)=>{},CancellationToken.None);return 99;
            }
            if(args[0]=="--pack") {MakePackage(args[1],args[2],args[3]=="dlc");return 0;}
            string work=Path.GetFullPath(args[0]),assets=Path.GetFullPath(args[1]);Directory.CreateDirectory(work);
            string packages=Path.Combine(work,"packages");Directory.CreateDirectory(packages);
            string original=Path.Combine(packages,"owned-game-package");MakePackage(assets,original,false);string originalHash=Hash(original);
            Check(PortableGame.FindPackages(packages,false,false).Count==1,"supported revision detected beside launcher, without extension");
            File.Copy(original,Path.Combine(packages,"second-package"));Check(PortableGame.FindPackages(packages,false,false).Count==2,"multiple packages returned for user choice, not guessed");
            Directory.CreateDirectory(Path.Combine(work,"nested","folder"));File.Copy(original,Path.Combine(work,"nested","folder","package"));
            Check(PortableGame.FindPackages(Path.Combine(work,"nested"),false,false).Count==0&&PortableGame.FindPackages(Path.Combine(work,"nested"),false,true).Count==1,"automatic scan is shallow; explicit folder selection finds nested package");
            string root=Path.Combine(work,"installed");Defaults(root);var game=new PortableGame(root);
            game.Import(original,false,(p,s)=>{},CancellationToken.None);Check(game.Ready(),"first import creates ready marker and valid assets");
            string marker=Path.Combine(game.Resources,"import-manifest.json"),savedMarker=File.ReadAllText(marker);
            File.WriteAllText(marker,"{}");Check(!game.Ready(),"incomplete manifest is not accepted as a validated import");File.WriteAllText(marker,savedMarker);
            Check(!Directory.Exists(Path.Combine(root,"Game"))&&File.Exists(Path.Combine(root,"resources","assets","default.xex")),"flat portable layout, with assets in resources");
            Check(File.ReadAllText(Path.Combine(root,"resources","boom_boom_rocket.toml")).Contains("bbr_player_one_name"),"fresh profile names assigned once");
            string config=Path.Combine(root,"resources","boom_boom_rocket.toml");File.AppendAllText(config,"\n# Settings preservation sentinel\n");
            string saves=Path.Combine(root,"resources","userdata","5841086A","profile","User");Directory.CreateDirectory(saves);File.WriteAllText(Path.Combine(saves,"save"),"save sentinel");string configHash=Hash(config);
            string xexHash=Hash(Path.Combine(game.Assets,"default.xex"));DateTime xexTime=File.GetLastWriteTimeUtc(Path.Combine(game.Assets,"default.xex"));
            File.Move(original,original+".relocated");game.Prepare((p,s)=>{},CancellationToken.None);Check(game.Ready()&&File.GetLastWriteTimeUtc(Path.Combine(game.Assets,"default.xex"))==xexTime,"repeat launch needs no original package or re-extraction");File.Move(original+".relocated",original);
            game.Import(original,false,(p,s)=>{},CancellationToken.None);Check(Hash(config)==configHash&&File.ReadAllText(Path.Combine(saves,"save"))=="save sentinel","repair keeps settings and saves");
            var cancellation=new CancellationTokenSource();cancellation.Cancel();Reject(()=>game.Import(original,false,(p,s)=>{},cancellation.Token),"cancelled import rejected before replacing working data");
            Check(game.Ready()&&Hash(config)==configHash,"cancel preserves ready installation");
            game.AfterMoveForTest=i=>{if(i==1)throw new IOException("injected commit failure");};Reject(()=>game.Import(original,false,(p,s)=>{},CancellationToken.None),"commit failure is recoverable");game.AfterMoveForTest=null;
            Check(game.Ready()&&Hash(Path.Combine(game.Assets,"default.xex"))==xexHash&&Hash(config)==configHash,"transaction rollback restores assets, settings and manifest");
            var crash=System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(System.Reflection.Assembly.GetExecutingAssembly().Location,"--crash \""+root+"\" \""+original+"\""){UseShellExecute=false});crash.WaitForExit();
            Check(crash.ExitCode==77&&!game.Ready(),"hard process interruption leaves a recoverable journal");
            game.Recover();Check(game.Ready()&&Hash(config)==configHash&&Hash(Path.Combine(game.Assets,"default.xex"))==xexHash,"next launch recovers hard-interrupted asset replacement");
            var midCancel=new CancellationTokenSource();Reject(()=>game.Import(original,false,(p,s)=>{if(p>5)midCancel.Cancel();},midCancel.Token),"cancellation during extraction leaves working installation intact");
            Check(game.Ready()&&Hash(config)==configHash,"mid-extraction cancellation preserves settings");
            string unsupportedSource=Path.Combine(work,"unsupported-input");Directory.CreateDirectory(unsupportedSource);File.Copy(Path.Combine(assets,"default.xex"),Path.Combine(unsupportedSource,"default.xex"));
            using(var changed=new FileStream(Path.Combine(unsupportedSource,"default.xex"),FileMode.Open,FileAccess.ReadWrite)){changed.Position=100;int old=changed.ReadByte();changed.Position=100;changed.WriteByte((byte)(old^1));}
            string unsupported=Path.Combine(work,"unsupported-package");MakePackage(unsupportedSource,unsupported,false);
            Reject(()=>game.Import(unsupported,false,(p,s)=>{},CancellationToken.None),"same Title ID with unsupported XEX revision rejected");
            string dlcSource=Path.Combine(work,"dlc-input");Directory.CreateDirectory(dlcSource);File.WriteAllText(Path.Combine(dlcSource,"fixture.txt"),"synthetic DLC extraction fixture");string addon=Path.Combine(work,"dlc-package");MakePackage(dlcSource,addon,true);
            game.Import(addon,true,(p,s)=>{},CancellationToken.None);Check(Directory.GetFiles(Path.Combine(game.Resources,"userdata","0000000000000000","5841086A","Headers","00000002")).Length==1,"DLC import writes runtime content header");
            game.Import(addon,true,(p,s)=>{},CancellationToken.None);Check(Hash(config)==configHash&&File.ReadAllText(Path.Combine(saves,"save"))=="save sentinel","repeat DLC import keeps settings and saves");
            Reject(()=>game.Import(addon,false,(p,s)=>{},CancellationToken.None),"DLC rejected as base-game input");
            Reject(()=>game.Import(Path.Combine(dlcSource,"fixture.txt"),false,(p,s)=>{},CancellationToken.None),"invalid package rejected without disturbing installation");
            string legacy=Path.Combine(work,"legacy","Game","resources");Directory.CreateDirectory(legacy);PortableGame.CopyTree(game.Assets,Path.Combine(legacy,"assets"),CancellationToken.None);PortableGame.CopyTree(Path.Combine(game.Resources,"userdata"),Path.Combine(legacy,"userdata"),CancellationToken.None);File.Copy(config,Path.Combine(legacy,"boom_boom_rocket.toml"));
            string migratedRoot=Path.Combine(work,"migrated");Defaults(migratedRoot);var migrated=new PortableGame(migratedRoot);migrated.Adopt(migrated.LegacyResources(Path.Combine(work,"legacy")),(p,s)=>{},CancellationToken.None);
            Check(migrated.Ready()&&Hash(Path.Combine(migrated.Resources,"boom_boom_rocket.toml"))==configHash&&File.Exists(Path.Combine(legacy,"assets","default.xex")),"legacy migration copies assets, DLC and settings; leaves old installation untouched");
            Check(Hash(original)==originalHash,"original game package never modified");
            game.ApplyExtras(false,false);Check(Hash(config)==configHash,"unselected extras leave all existing settings untouched");
            game.ApplyExtras(true,false);Check(File.ReadAllText(config).Contains("fullscreen = true")&&File.ReadAllText(config).Contains("Settings preservation sentinel"),"fullscreen extra persists without replacing unrelated settings");
            string desktop=Path.Combine(work,"test-desktop");game.ApplyExtras(false,true,desktop);
            string shortcutPath=Path.Combine(desktop,"Boom Boom Rocket.lnk");Check(File.Exists(shortcutPath),"desktop shortcut extra creates a conventional Windows link in an isolated test folder");
            object shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell"));object link=null;
            try{link=shell.GetType().InvokeMember("CreateShortcut",System.Reflection.BindingFlags.InvokeMethod,null,shell,new object[]{shortcutPath});string target=(string)link.GetType().InvokeMember("TargetPath",System.Reflection.BindingFlags.GetProperty,null,link,null);Check(string.Equals(target,Path.Combine(root,"Boom Boom Rocket.exe"),StringComparison.OrdinalIgnoreCase),"desktop shortcut targets the root portable launcher");}
            finally{if(link!=null)System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);System.Runtime.InteropServices.Marshal.FinalReleaseComObject(shell);}
            string shortcutHash=Hash(shortcutPath);Reject(()=>game.ApplyExtras(false,true,desktop),"existing desktop shortcut is never silently overwritten");Check(Hash(shortcutPath)==shortcutHash,"existing shortcut remains unchanged");
            if(args.Length>2)foreach(string mode in new[]{"file","folder","drop-file","drop-folder","auto","multiple"})UiCase(work,original,args[2],mode);
            if(args.Length>2)foreach(string mode in new[]{"dlc-file","dlc-folder","dlc-drop-file","dlc-drop-folder"})UiCase(work,addon,args[2],mode);
            if(args.Length>2)foreach(string mode in new[]{"first-dlc","first-dlc-skip","game-then-dlc"})UiCase(work,original,args[2],mode);
            Console.WriteLine("SUCCESS: "+checks+" portable launcher checks");return 0;
        }catch(Exception e){Console.Error.WriteLine(e);return 1;}
    }
}
