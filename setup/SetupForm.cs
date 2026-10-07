using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Bbr.Setup {
    internal sealed class SetupForm : Form {
        internal static readonly Color Cyan=Color.FromArgb(42,219,255), Pink=Color.FromArgb(246,66,212), Muted=Color.FromArgb(161,177,202);
        readonly TextBox game=new TextBox(),dlc=new TextBox(),folder=new TextBox();
        readonly NeonButton browseGame,browseDlc,clearDlc,install,close;
        readonly Label gameHint,dlcHint,status,percent,heading;
        readonly Panel progressFill;
        readonly ToolTip tips=new ToolTip();
        CancellationTokenSource cancellation;
        string executable;
        bool busy;
        [DllImport("dwmapi.dll")] static extern int DwmSetWindowAttribute(IntPtr h,int attr,ref int value,int size);
        internal SetupForm(float designDpi=96f) {
            // Defer WinForms' first DPI scaling pass until ALL controls exist.
            // Otherwise setting Font consumes the 96-DPI baseline before the
            // controls are added, leaving their bounds unscaled at high DPI.
            SuspendLayout();
            Text="Boom Boom Rocket — Setup";
            AutoScaleDimensions=new SizeF(designDpi,designDpi); AutoScaleMode=AutoScaleMode.Dpi;
            FormBorderStyle=FormBorderStyle.FixedSingle;
            MaximizeBox=false; StartPosition=FormStartPosition.CenterScreen;
            BackColor=Color.FromArgb(8,16,25); ForeColor=Color.White; Font=new Font("Segoe UI",10); DoubleBuffered=true;
            ClientSize=new Size(960,686);
            Icon=MakeIcon();
            heading=LabelAt("Game package",167,207,560,29,Color.White,12,FontStyle.Bold);
            LabelAt("01",83,204,57,43,Cyan,23);
            PathBox(game,180,251,567,"Game package");
            gameHint=LabelAt("",167,283,730,22,Muted,9.5f);
            browseGame=ButtonAt("Browse",777,240,121,42,Cyan,false);
            browseGame.Click+=(s,e)=>SelectPackage(game,false);
            LabelAt("02",83,315,57,43,Cyan,23);
            LabelAt("Boom Boom Rock Pack DLC (Optional)",167,318,560,29,Color.White,12,FontStyle.Bold);
            PathBox(dlc,180,362,539,"Optional Boom Boom Rock Pack DLC package");
            dlcHint=LabelAt("You can add DLC later.",167,400,730,22,Muted,9.5f);
            browseDlc=ButtonAt("Browse",777,351,121,42,Cyan,false);
            browseDlc.Click+=(s,e)=>SelectPackage(dlc,true);
            clearDlc=ButtonAt("×",724,355,32,32,Muted,false); clearDlc.AccessibleName="Remove selected DLC";
            clearDlc.Click+=(s,e)=>{dlc.Text="Skip or select a DLC package";UpdateState();};
            LabelAt("03",83,426,57,43,Cyan,23);
            LabelAt("Portable folder",167,429,500,29,Color.White,12,FontStyle.Bold);
            PathBox(folder,180,473,703,"Portable folder beside this installer");
            LabelAt("Play from Game. Runtime, assets, DLC and saves stay in Game/resources.",167,512,730,22,Muted,9.5f);
            status=LabelAt("",85,538,735,21,Muted,9);
            percent=LabelAt("",834,538,65,21,Cyan,9.5f,FontStyle.Bold); percent.TextAlign=ContentAlignment.TopRight;
            var track=new Panel { Location=new Point(86,560),Size=new Size(811,9),BackColor=Color.FromArgb(16,30,44) };
            progressFill=new Panel { Location=Point.Empty,Size=new Size(0,9),BackColor=Cyan };track.Controls.Add(progressFill);Controls.Add(track);
            close=ButtonAt("Close",685,600,97,44,Muted,false); close.Click+=(s,e)=>{ if(busy){cancellation.Cancel();status.Text="Cancelling safely...";}else Close();};
            install=ButtonAt("Install game",798,600,131,44,Pink,true); install.Click+=async(s,e)=>await InstallAsync();
            AcceptButton=install; CancelButton=close;
            folder.TextChanged+=(s,e)=>UpdateState(); folder.Text=InstallEngine.DefaultDestination();
            game.TextChanged+=(s,e)=>UpdateState(); dlc.TextChanged+=(s,e)=>UpdateState();
            game.Text="No package selected";dlc.Text="Skip or select a DLC package";
            Shown+=(s,e)=>{try{int dark=1;DwmSetWindowAttribute(Handle,20,ref dark,4);}catch{} UpdateState();};
            FormClosing+=(s,e)=>{if(busy){e.Cancel=true;cancellation.Cancel();status.Text="Cancelling safely...";}};
            ResumeLayout(true);
        }
        internal static OpenFileDialog CreatePackagePicker(bool isDlc) {
            return new OpenFileDialog { Title=isDlc?"Choose Boom Boom Rock Pack DLC":"Choose your Boom Boom Rocket game package", Filter="Xbox 360 package (all files)|*.*", CheckFileExists=true, RestoreDirectory=true, InitialDirectory=Path.GetDirectoryName(Application.ExecutablePath) };
        }
        void SelectPackage(TextBox box,bool isDlc) {
            using(var picker=CreatePackagePicker(isDlc)) {
                if(picker.ShowDialog(this)!=DialogResult.OK)return;
                try { using(var package=new StfsPackage(picker.FileName,isDlc)) { box.Text=picker.FileName; tips.SetToolTip(box,picker.FileName); } }
                catch(Exception error){MessageBox.Show(this,error.Message,"Check your package",MessageBoxButtons.OK,MessageBoxIcon.Information);}
            }
        }
        void UpdateState() {
            if(install==null || busy)return;
            bool hasGame=false,hasDlc=false,validFolder=false;
            try{validFolder=Path.IsPathRooted(folder.Text)&&folder.Text.Length>3;hasGame=InstallEngine.HasGame(folder.Text);hasDlc=InstallEngine.HasDlc(folder.Text);}catch{}
            browseGame.Enabled=!hasGame&&executable==null;
            gameHint.Text=hasGame?"Installed game detected. Your game data and saves will be kept.":"";
            gameHint.ForeColor=hasGame?Cyan:Muted;
            if(hasGame && !File.Exists(game.Text) && game.Text!="Existing game — no package needed")game.Text="Existing game — no package needed";
            if(!hasGame && game.Text=="Existing game — no package needed")game.Text="No package selected";
            dlcHint.Text=File.Exists(dlc.Text)?"Boom Boom Rock Pack DLC selected.":hasDlc?"DLC is already installed in this folder.":"You can add DLC later.";
            clearDlc.Visible=File.Exists(dlc.Text);
            install.Enabled=executable!=null || (validFolder&&(hasGame||File.Exists(game.Text)));
            install.Text=executable!=null?"Play now":hasGame?(File.Exists(dlc.Text)?"Install DLC":"Update / play"):"Install game";
            if(executable==null)status.Text=hasGame?"Existing installation found. Add DLC or update the PC files.":File.Exists(game.Text)?"Ready to install. Your original packages stay untouched.":"";
        }
        async Task InstallAsync() {
            if(executable!=null) {
                try { Process.Start(new ProcessStartInfo(executable){UseShellExecute=true,WorkingDirectory=Path.GetDirectoryName(executable)});Close(); }
                catch(Exception error){MessageBox.Show(this,error.Message,"Could not start the game");}
                return;
            }
            string source=game.Text,addon=File.Exists(dlc.Text)?dlc.Text:null,destination=folder.Text;
            cancellation=new CancellationTokenSource(); busy=true; install.Enabled=false;close.Text="Cancel";
            foreach(Control c in new Control[]{browseGame,browseDlc,clearDlc,folder})c.Enabled=false;
            var progress=new Progress<InstallProgress>(p=>{progressFill.Width=progressFill.Parent.ClientSize.Width*p.Percent/100;percent.Text=p.Percent+"%";status.Text=p.Text;});
            try {
                executable=await Task.Run(()=>new InstallEngine().Install(source,addon,destination,(p,t)=>((IProgress<InstallProgress>)progress).Report(new InstallProgress{Percent=p,Text=t}),cancellation.Token));
                heading.Text="Game ready to play";heading.ForeColor=Cyan;install.Text="Play now";
            } catch(OperationCanceledException){status.Text="Cancelled. Your packages and previous installation are safe.";}
            catch(Exception error){status.Text="Installation stopped. Choose a package or folder and try again.";MessageBox.Show(this,error.Message,"Installation stopped",MessageBoxButtons.OK,MessageBoxIcon.Error);}
            finally {
                cancellation.Dispose();cancellation=null;busy=false;close.Text="Close";install.Enabled=true;
                foreach(Control c in new Control[]{browseGame,browseDlc,clearDlc,folder})c.Enabled=executable==null;
                if(executable!=null){install.Focus();}else{install.Text="Try again";}
            }
        }
        sealed class InstallProgress { internal int Percent;internal string Text; }
        Label LabelAt(string text,int x,int y,int w,int h,Color color,float size,FontStyle style=FontStyle.Regular) {
            var label=new Label{Text=text,Location=new Point(x,y),Size=new Size(w,h),ForeColor=color,BackColor=Color.Transparent,Font=new Font("Segoe UI",size,style),AutoEllipsis=true}; Controls.Add(label);return label;
        }
        void PathBox(TextBox box,int x,int y,int w,string name) {
            box.Location=new Point(x,y);box.Size=new Size(w,25);box.BorderStyle=BorderStyle.None;
            box.BackColor=Color.FromArgb(15,26,39);box.ForeColor=Color.FromArgb(195,210,228);box.Font=new Font("Segoe UI",10.5f);
            box.ReadOnly=true;box.TabStop=false;box.AccessibleName=name;Controls.Add(box);
        }
        NeonButton ButtonAt(string text,int x,int y,int w,int h,Color accent,bool primary) {
            var button=new NeonButton(accent,primary){Text=text,Location=new Point(x,y),Size=new Size(w,h)};Controls.Add(button);return button;
        }
        internal static GraphicsPath Rounded(RectangleF r,float radius) {
            float d=radius*2;var p=new GraphicsPath();
            p.AddArc(r.Left,r.Top,d,d,180,90);p.AddArc(r.Right-d,r.Top,d,d,270,90);
            p.AddArc(r.Right-d,r.Bottom-d,d,d,0,90);p.AddArc(r.Left,r.Bottom-d,d,d,90,90);p.CloseFigure();return p;
        }
        protected override void OnPaint(PaintEventArgs e) {
            base.OnPaint(e);var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;
            // Share the actual scaled client bounds with the native controls.
            // The form is fixed-size, with a 960 x 686 logical design canvas.
            g.ScaleTransform(ClientSize.Width/960f,ClientSize.Height/686f);
            using(var background=new LinearGradientBrush(new Rectangle(0,0,960,686),Color.FromArgb(11,20,31),Color.FromArgb(6,15,23),35f))g.FillRectangle(background,0,0,960,686);
            GlowText(g,"BOOM BOOM",new PointF(41,28),51,Cyan);
            GlowText(g,"ROCKET",new PointF(40,76),73,Pink);
            // Pixel font avoids applying device DPI again inside the transform.
            using(var b=new SolidBrush(Color.FromArgb(221,247,255)))using(var f=new Font("Segoe UI",24,FontStyle.Regular,GraphicsUnit.Pixel))g.DrawString("S E T U P",f,b,46,151);
            MinimalFirework(g,853,94);
            foreach(int y in new[]{207,318,429}) {
                using(var pen=new Pen(Color.FromArgb(165,Cyan),1))g.DrawLine(pen,141,y,141,y+76);
                DrawField(g,new RectangleF(167,y+33,y==429?731:595,42));
            }
            using(var p=Rounded(new RectangleF(85,559,813,11),3))using(var pen=new Pen(Color.FromArgb(68,106,131)))g.DrawPath(pen,p);
            DrawSkyline(g);
        }
        static void DrawField(Graphics g,RectangleF r) {
            using(var p=Rounded(r,4))using(var b=new SolidBrush(Color.FromArgb(15,26,39)))using(var pen=new Pen(Color.FromArgb(62,99,125))){g.FillPath(b,p);g.DrawPath(pen,p);}
        }
        static void MinimalFirework(Graphics g,float x,float y) {
            for(int i=0;i<20;i++) {
                double a=i*Math.PI*2/20;float r=i%3==0?70:49+i%4*5;
                Color color=i%3==0?Pink:Cyan;
                float sx=x+(float)Math.Cos(a)*12,sy=y+(float)Math.Sin(a)*12;
                float ex=x+(float)Math.Cos(a)*r,ey=y+(float)Math.Sin(a)*r;
                using(var pen=new Pen(Color.FromArgb(165,color),1.2f))g.DrawBezier(pen,sx,sy,x+(float)Math.Cos(a+.2)*r*.5f,y+(float)Math.Sin(a+.2)*r*.5f,ex,ey-5,ex,ey);
                if(i%3!=0)using(var b=new SolidBrush(color))g.FillEllipse(b,ex-1.5f,ey-1.5f,3,3);
            }
            Star(g,x-79,y-43,6,Cyan);Star(g,x+58,y-56,9,Pink);Star(g,x+73,y+60,8,Pink);Star(g,x+68,y-22,3,Cyan);
        }
        static void Star(Graphics g,float x,float y,float radius,Color color) {
            using(var b=new SolidBrush(color))g.FillPolygon(b,new[]{new PointF(x,y-radius),new PointF(x+2,y-2),new PointF(x+radius,y),new PointF(x+2,y+2),new PointF(x,y+radius),new PointF(x-2,y+2),new PointF(x-radius,y),new PointF(x-2,y-2)});
        }
        static void DrawSkyline(Graphics g) {
            // Lightweight original line artwork matching the selected Minimal Neon concept.
            var random=new Random(67);const int baseline=663;
            using(var outline=new Pen(Color.FromArgb(158,Cyan),.8f)) {
                for(int x=12;x<538;) {
                    int w=random.Next(15,29),h=random.Next(12,55);
                    if(x>430)h=random.Next(9,28);
                    g.DrawLines(outline,new[]{new Point(x,baseline),new Point(x,baseline-h),new Point(x+w,baseline-h),new Point(x+w,baseline)});
                    for(int wx=x+5;wx<x+w-3;wx+=7)for(int wy=baseline-h+6;wy<baseline-2;wy+=9)if(random.Next(3)==0)using(var b=new SolidBrush(random.Next(4)==0?Pink:Color.FromArgb(130,Cyan)))g.FillRectangle(b,wx,wy,1.5f,2);
                    if(random.Next(4)==0){g.DrawLine(outline,x+w/2,baseline-h,x+w/2,baseline-h-8);using(var b=new SolidBrush(Pink))g.FillEllipse(b,x+w/2-1,baseline-h-10,2,2);}
                    x+=w+5;
                }
                // Tall landmark on the left, observation wheel and suspension bridge.
                g.DrawLines(outline,new[]{new Point(44,663),new Point(44,574),new Point(49,574),new Point(49,553),new Point(51,553),new Point(51,530),new Point(52,553),new Point(54,553),new Point(54,574),new Point(59,574),new Point(59,663)});
                g.DrawEllipse(outline,45,565,12,12);g.DrawEllipse(outline,546,620,39,39);
                for(int i=0;i<12;i++){double a=i*Math.PI/6;g.DrawLine(outline,565.5f,639.5f,565.5f+(float)Math.Cos(a)*19.5f,639.5f+(float)Math.Sin(a)*19.5f);}
                g.DrawLine(outline,565,639,557,663);g.DrawLine(outline,566,639,574,663);
                g.DrawLine(outline,620,663,620,631);g.DrawLine(outline,673,663,673,643);
                g.DrawBezier(outline,594,661,606,658,615,643,620,633);
                g.DrawBezier(outline,620,633,638,650,655,657,673,645);
                g.DrawBezier(outline,673,645,695,661,718,663,744,663);
                for(int x=625;x<670;x+=7)g.DrawLine(outline,x,650+(int)(5*Math.Sin((x-625)*Math.PI/45)),x,663);
                g.DrawLine(outline,0,baseline+2,800,baseline+2);
            }
            for(int i=0;i<170;i++) {int x=random.Next(790),y=random.Next(669,686);using(var p=new Pen(Color.FromArgb(random.Next(20,95),i%6==0?Pink:Cyan),.7f))g.DrawLine(p,x,y,x+random.Next(2,11),y);}
        }
        static void GlowText(Graphics g,string text,PointF point,float size,Color color) {
            using(var p=new GraphicsPath()) {
                using(var family=new FontFamily("Arial"))p.AddString(text,family,(int)(FontStyle.Bold|FontStyle.Italic),size,point,StringFormat.GenericDefault);
                // Equal logo widths keep the stacked wordmark close to the concept.
                RectangleF bounds=p.GetBounds();using(var m=new Matrix()){m.Translate(-bounds.Left,-bounds.Top);p.Transform(m);}
                using(var m=new Matrix()){m.Scale(415/bounds.Width,1);p.Transform(m);}
                using(var m=new Matrix()){m.Translate(point.X,point.Y+12);p.Transform(m);}
                for(int i=7;i>=3;i-=2)using(var pen=new Pen(Color.FromArgb(9,color),i)){pen.LineJoin=LineJoin.Round;g.DrawPath(pen,p);}
                using(var b=new SolidBrush(Color.FromArgb(10,22,32)))g.FillPath(b,p);
                using(var pen=new Pen(color,1.7f))g.DrawPath(pen,p);
            }
        }
        static void Firework(Graphics g,float x,float y,float radius,Color color,int seed) {
            var random=new Random(seed);
            for(int i=0;i<34;i++){
                double angle=i*Math.PI*2/34;float length=radius*(0.68f+(float)random.NextDouble()*0.32f);
                float ax=x+(float)Math.Cos(angle)*length*0.28f,ay=y+(float)Math.Sin(angle)*length*0.28f;
                float bx=x+(float)Math.Cos(angle)*length,by=y+(float)Math.Sin(angle)*length;
                using(var glow=new Pen(Color.FromArgb(22,color),5))g.DrawLine(glow,ax,ay,bx,by);
                using(var pen=new Pen(Color.FromArgb(150+random.Next(105),color),1.1f))g.DrawLine(pen,ax,ay,bx,by);
                using(var b=new SolidBrush(Color.FromArgb(220,color)))g.FillEllipse(b,bx-1,by-1,2.5f,2.5f);
            }
        }
        static Icon MakeIcon() {
            using(var bmp=new Bitmap(32,32)) {
                using(var g=Graphics.FromImage(bmp)){g.Clear(Color.FromArgb(8,12,25));g.SmoothingMode=SmoothingMode.AntiAlias;Firework(g,16,16,13,Pink,2);}
                IntPtr handle=bmp.GetHicon();try{return (Icon)Icon.FromHandle(handle).Clone();}finally{DestroyIcon(handle);}
            }
        }
        [DllImport("user32.dll")]static extern bool DestroyIcon(IntPtr h);
        protected override void Dispose(bool disposing){if(disposing)tips.Dispose();base.Dispose(disposing);}
    }
    internal sealed class NeonButton : Button {
        readonly Color accent;readonly bool primary;bool hover;
        internal NeonButton(Color color,bool filled){accent=color;primary=filled;SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);FlatStyle=FlatStyle.Flat;FlatAppearance.BorderSize=0;Cursor=Cursors.Hand;Font=new Font("Segoe UI",10.5f,filled?FontStyle.Bold:FontStyle.Regular);BackColor=Color.FromArgb(8,16,25);ForeColor=Color.White;}
        protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
        protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
        protected override void OnPaint(PaintEventArgs e){
            var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;g.Clear(BackColor);
            using(var p=SetupForm.Rounded(new RectangleF(1,1,Width-3,Height-3),4)){
                Color fill=primary?Color.FromArgb(hover?255:239,hover?50:12,hover?211:180):Color.FromArgb(hover?27:17,hover?43:28,hover?61:42);
                if(!Enabled)fill=primary?Color.FromArgb(92,21,79):Color.FromArgb(17,24,34);
                using(var b=new LinearGradientBrush(ClientRectangle,fill,primary?fill:Color.FromArgb(13,23,35),90f))g.FillPath(b,p);
                using(var pen=new Pen(Enabled?accent:primary?Color.FromArgb(137,38,115):Color.FromArgb(54,60,76),Focused?2:1))g.DrawPath(pen,p);
            }
            TextRenderer.DrawText(g,Text,Font,ClientRectangle,Enabled?Color.White:Color.FromArgb(112,120,141),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
            if(Focused&&ShowFocusCues)ControlPaint.DrawFocusRectangle(g,new Rectangle(9,7,Width-18,Height-14),Color.White,BackColor);
        }
    }
}
