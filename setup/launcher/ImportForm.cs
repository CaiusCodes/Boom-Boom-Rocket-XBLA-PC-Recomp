using System;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Collections.Generic;

namespace Bbr.Setup {
    internal sealed class ImportForm : Form {
        sealed class PackageChoice {
            internal string Path;
            public override string ToString(){return System.IO.Path.GetFileName(Path)+" — "+System.IO.Path.GetDirectoryName(Path);}
        }
        readonly PortableGame game;
        readonly bool manage;
        readonly bool dlcOnly;
        readonly string initialInput;
        readonly Label status=new Label();
        readonly ImportProgress progress=new ImportProgress();
        readonly ListBox choices=new ListBox();
        readonly Button file=new LauncherButton(LauncherTheme.Cyan,false),dlc=new LauncherButton(LauncherTheme.Cyan,false),skipDlc=new LauncherButton(LauncherTheme.Muted,false),cancel=new LauncherButton(LauncherTheme.Muted,false);
        readonly Label selectedDlc=new Label();
        readonly Button play=new LauncherButton(LauncherTheme.Pink,true);
        readonly CheckBox fullscreen=new CheckBox(),desktopShortcut=new CheckBox();
        string pendingDlc;
        CancellationTokenSource cancellation;
        bool busy,closeRequested,choiceDlc;
        internal ImportForm(PortableGame game,bool manage,string input,bool dlcOnly=false) {
            this.game=game;this.manage=manage;this.dlcOnly=dlcOnly;initialInput=input;
            SuspendLayout();AutoScaleDimensions=new SizeF(96,96);AutoScaleMode=AutoScaleMode.Dpi;
            Text=dlcOnly?"Boom Boom Rocket — Import DLC":manage?"Boom Boom Rocket — Game Files":"Boom Boom Rocket";
            Font=new Font("Segoe UI",10);BackColor=Color.FromArgb(9,15,23);ForeColor=Color.White;
            ClientSize=new Size(960,686);FormBorderStyle=FormBorderStyle.FixedSingle;MaximizeBox=false;StartPosition=FormStartPosition.CenterScreen;
            DoubleBuffered=true;
            try {Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);}catch{}
            AddLabel("01",88,176,60,42,24,LauncherTheme.Cyan);
            AddLabel(dlcOnly?"Boom Boom Rock Pack DLC":manage?"Original game assets":"Your original game package",167,176,727,30,12,Color.White,true);
            AddLabel(dlcOnly?"Choose your legally obtained Rock Pack package to add the extra songs.":"Choose your legally obtained Xbox 360 package. Your original files stay untouched.",167,214,727,24,10,LauncherTheme.Muted);
            AddButton(file,dlcOnly?"Browse DLC package":manage?"Repair / re-import":"Browse package",167,250,280);file.Click+=async(s,e)=>await Browse(dlcOnly);
            AddButton(dlc,"Browse DLC (optional)",167,404,280);dlc.Visible=!dlcOnly;dlc.Click+=async(s,e)=>await Browse(true);
            AddButton(skipDlc,"Skip DLC",465,404,140);skipDlc.Visible=false;skipDlc.Click+=(s,e)=>{pendingDlc=null;selectedDlc.Text="";skipDlc.Visible=false;status.Text=game.Ready()?"DLC skipped. Click Play when you're ready.":"DLC skipped. Choose your game package to start.";SetBusy(false);};
            AddLabel("02",88,316,60,42,24,LauncherTheme.Cyan);
            AddLabel(dlcOnly?"Or drag and drop here":"Boom Boom Rock Pack DLC (Optional)",167,316,727,30,12,Color.White,true);
            AddLabel(dlcOnly?"Drop one DLC package or a folder containing it.\nImport completes straight into play. Your saves and settings are kept.":"Import optional DLC before or after your game package, then click Play.\nYou can also add it later from Download Content on the main menu.",167,354,727,36,10,LauncherTheme.Muted);
            selectedDlc.Location=new Point(167,460);selectedDlc.Size=new Size(727,26);selectedDlc.BackColor=Color.Transparent;selectedDlc.ForeColor=LauncherTheme.Cyan;selectedDlc.AutoEllipsis=true;selectedDlc.Text=dlcOnly?"":"You can also drag and drop a package or its folder anywhere in this window.";Controls.Add(selectedDlc);
            choices.Location=new Point(167,460);choices.Size=new Size(727,43);
            choices.BackColor=BackColor;choices.ForeColor=ForeColor;choices.Visible=false;choices.HorizontalScrollbar=true;
            choices.SelectedIndexChanged+=async(s,e)=>{if(!busy&&choices.SelectedItem!=null)await Import(((PackageChoice)choices.SelectedItem).Path,choiceDlc);};Controls.Add(choices);
            if(!manage){AddLabel("Extras",85,511,95,30,11,Color.White,true);AddExtra(fullscreen,"Start in fullscreen",187);AddExtra(desktopShortcut,"Create desktop shortcut",450);}
            status.BackColor=Color.Transparent;status.ForeColor=LauncherTheme.Muted;status.Location=new Point(85,550);status.Size=new Size(813,26);Controls.Add(status);
            progress.Location=new Point(85,584);progress.Size=new Size(813,13);Controls.Add(progress);
            AddButton(cancel,"Close",manage?798:653,609,128);cancel.Click+=(s,e)=>Close();CancelButton=cancel;
            AddButton(play,"Play",798,609,128);play.Visible=!manage;play.Enabled=false;
            play.Click+=(s,e)=>{if(busy||pendingDlc!=null||!game.Ready())return;try{game.ApplyExtras(fullscreen.Checked,desktopShortcut.Checked);game.Launch();Close();}catch(Exception error){MessageBox.Show(this,error.Message,"Could not start game",MessageBoxButtons.OK,MessageBoxIcon.Error);}};
            AllowDrop=true;
            DragEnter+=(s,e)=>{e.Effect=!busy&&e.Data.GetDataPresent(DataFormats.FileDrop)?DragDropEffects.Copy:DragDropEffects.None;};
            DragDrop+=async(s,e)=>{var paths=e.Data.GetData(DataFormats.FileDrop) as string[];if(paths==null)return;if(paths.Length!=1){status.Text="Drop one package or folder at a time.";return;}await SelectInput(paths[0],dlcOnly,true);};
            FormClosing+=(s,e)=>{if(busy){e.Cancel=true;closeRequested=true;cancellation.Cancel();status.Text="Cancelling safely...";}};
            Shown+=async(s,e)=>await Initialize();ResumeLayout(true);
        }
        protected override void OnPaint(PaintEventArgs e){base.OnPaint(e);LauncherTheme.Paint(e.Graphics,ClientSize,dlcOnly?"D L C   I M P O R T":manage?"G A M E   F I L E S":"");}
        void AddLabel(string text,int x,int y,int width,int height,float size,Color color,bool bold=false){Controls.Add(new Label{Text=text,Location=new Point(x,y),Size=new Size(width,height),BackColor=Color.Transparent,ForeColor=color,Font=new Font("Segoe UI",size,bold?FontStyle.Bold:FontStyle.Regular)});}
        void AddButton(Button button,string text,int x,int y,int width) {button.Text=text;button.Location=new Point(x,y);button.Size=new Size(width,44);button.FlatStyle=FlatStyle.Flat;button.ForeColor=Color.FromArgb(42,219,255);Controls.Add(button);}
        void AddExtra(CheckBox box,string text,int x){box.Text=text;box.Location=new Point(x,510);box.Size=new Size(255,30);box.BackColor=Color.Transparent;box.ForeColor=LauncherTheme.Muted;box.Cursor=Cursors.Hand;Controls.Add(box);}
        void SetBusy(bool value) {busy=value;bool ready=game.Ready();file.Enabled=!value&&(manage||!ready);choices.Enabled=dlc.Enabled=skipDlc.Enabled=fullscreen.Enabled=desktopShortcut.Enabled=!value;play.Enabled=!value&&pendingDlc==null&&ready;cancel.Text=value?"Cancel":"Close";}
        void Report(int value,string text) {if(IsDisposed)return;BeginInvoke(new Action(()=>{progress.Value=Math.Max(0,Math.Min(100,value));status.Text=text;}));}
        async Task Run(Action<CancellationToken> operation,bool launch) {
            if(busy)return;cancellation=new CancellationTokenSource();SetBusy(true);
            try {await Task.Run(()=>operation(cancellation.Token));if(closeRequested){SetBusy(false);Close();return;}if(launch){if(manage){game.Launch();SetBusy(false);Close();}else{choices.Visible=false;skipDlc.Visible=pendingDlc!=null;status.Text="Game ready. Add optional DLC, or click Play to start.";}}}
            catch(OperationCanceledException){status.Text="Import cancelled. Installed files are unchanged.";}
            catch(Exception error){status.Text="Import did not complete.";MessageBox.Show(this,error.Message,"Check your game files",MessageBoxButtons.OK,MessageBoxIcon.Information);}
            finally {SetBusy(false);cancellation.Dispose();if(closeRequested)Close();}
        }
        async Task Initialize() {
            await Run(token=>game.Prepare(Report,token),false);
            if(IsDisposed||closeRequested)return;
            if(!manage&&game.Ready()){status.Text="Game ready. Add optional DLC, or click Play to start.";return;}
            if(initialInput!=null){await SelectInput(initialInput,dlcOnly,true);return;}
            if(!manage)await Find(game.Root,false,false);
            else status.Text=dlcOnly?"Choose your Boom Boom Rock Pack DLC package. Import starts as soon as you select it.":"Import optional DLC, or repair assets. Saves and settings are preserved.";
        }
        async Task Browse(bool addon) {
            using(var picker=new OpenFileDialog{Title=addon?"Choose Boom Boom Rock Pack DLC":"Choose your original Boom Boom Rocket package",Filter="Xbox 360 package (all files)|*.*",CheckFileExists=true,RestoreDirectory=true,InitialDirectory=game.Root})
                if(picker.ShowDialog(this)==DialogResult.OK)await SelectInput(picker.FileName,addon);
        }
        internal async Task SelectInput(string path,bool addon,bool detect=false) {
            if(busy)return;
            if(Directory.Exists(path)) {
                string legacy=!addon&&!game.Ready()?game.LegacyResources(path):null;
                if(legacy!=null){await Run(token=>{game.Adopt(legacy,Report,token);if(pendingDlc!=null){game.Import(pendingDlc,true,Report,token);pendingDlc=null;}},true);return;}
                await Find(path,addon,true);
            }else {
                if(detect&&!addon) {
                    try {PortableGame.ValidatePackage(path,true);addon=true;}catch(InvalidDataException){}catch(IOException){}
                }
                await Import(path,addon);
            }
        }
        async Task Find(string path,bool addon,bool nested) {
            List<string> found=null;
            await Run(token=>{found=PortableGame.FindPackages(path,addon,nested,token);
                if(nested&&!addon&&game.Ready()&&found.Count==0){found=PortableGame.FindPackages(path,true,true,token);if(found.Count>0)addon=true;}
            },false);
            if(IsDisposed||closeRequested||found==null)return;
            if(found.Count==1){await Import(found[0],addon);return;}
            choices.Items.Clear();choiceDlc=addon;choices.Visible=found.Count>1;
            foreach(string candidate in found)choices.Items.Add(new PackageChoice{Path=candidate});
            status.Text=found.Count>1?"Multiple compatible packages found. Click the one to import.":addon?"No compatible Rock Pack DLC found. Browse for a DLC package or drop another folder.":"Choose your original package to start. DLC is optional and can be added later.";
        }
        async Task Import(string path,bool addon) {
            if(!addon&&!manage&&game.Ready()){status.Text="Game package already imported. Add optional DLC, or click Play.";return;}
            if(addon&&!game.Ready()) {
                bool valid=false;
                await Run(token=>{token.ThrowIfCancellationRequested();PortableGame.ValidatePackage(path,true);valid=true;},false);
                if(valid&&!IsDisposed&&!closeRequested){pendingDlc=path;selectedDlc.Text="Selected DLC: "+Path.GetFileName(path);skipDlc.Visible=true;choices.Visible=false;status.Text="DLC selected. Choose your game package to import both, then click Play.";}
                return;
            }
            await Run(token=>{
                game.Import(path,addon,Report,token);
                if(!addon&&pendingDlc!=null){game.Import(pendingDlc,true,Report,token);pendingDlc=null;}
            },true);
        }
    }
}
