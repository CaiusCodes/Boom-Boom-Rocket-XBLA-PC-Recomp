using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace Bbr.Setup {
    // Original project-owned procedural artwork reused from the old setup.
    internal static class LauncherTheme {
        internal static readonly Color Cyan=Color.FromArgb(42,219,255), Pink=Color.FromArgb(246,66,212), Muted=Color.FromArgb(161,177,202);
        internal static GraphicsPath Rounded(RectangleF r,float radius) {
            float d=radius*2;var p=new GraphicsPath();
            p.AddArc(r.Left,r.Top,d,d,180,90);p.AddArc(r.Right-d,r.Top,d,d,270,90);
            p.AddArc(r.Right-d,r.Bottom-d,d,d,0,90);p.AddArc(r.Left,r.Bottom-d,d,d,90,90);p.CloseFigure();return p;
        }

        internal static void Paint(Graphics g,Size size,string subtitle) {
            g.SmoothingMode=SmoothingMode.AntiAlias;
            g.ScaleTransform(size.Width/960f,size.Height/686f);
            using(var b=new LinearGradientBrush(new Rectangle(0,0,960,686),Color.FromArgb(11,20,31),Color.FromArgb(6,15,23),35f))g.FillRectangle(b,0,0,960,686);
            GlowText(g,"BOOM BOOM",new PointF(41,28),51,Cyan);
            GlowText(g,"ROCKET",new PointF(40,76),73,Pink);
            if(!string.IsNullOrEmpty(subtitle))using(var b=new SolidBrush(Color.FromArgb(221,247,255)))
            using(var f=new Font("Segoe UI",18,FontStyle.Regular,GraphicsUnit.Pixel))g.DrawString(subtitle,f,b,46,147);
            MinimalFirework(g,853,94);
            using(var p=new Pen(Color.FromArgb(165,Cyan))){g.DrawLine(p,141,176,141,294);g.DrawLine(p,141,316,141,448);}
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

    }
    internal sealed class LauncherButton : Button {
        readonly Color accent;readonly bool primary;bool hover;
        internal LauncherButton(Color color,bool filled){accent=color;primary=filled;SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);FlatStyle=FlatStyle.Flat;FlatAppearance.BorderSize=0;Cursor=Cursors.Hand;Font=new Font("Segoe UI",10.5f,filled?FontStyle.Bold:FontStyle.Regular);BackColor=Color.FromArgb(8,16,25);ForeColor=Color.White;}
        protected override void OnMouseEnter(EventArgs e){hover=true;Invalidate();base.OnMouseEnter(e);}
        protected override void OnMouseLeave(EventArgs e){hover=false;Invalidate();base.OnMouseLeave(e);}
        protected override void OnPaint(PaintEventArgs e){
            var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;g.Clear(BackColor);
            using(var p=LauncherTheme.Rounded(new RectangleF(1,1,Width-3,Height-3),4)){
                Color fill=primary?Color.FromArgb(hover?255:239,hover?50:12,hover?211:180):Color.FromArgb(hover?27:17,hover?43:28,hover?61:42);
                if(!Enabled)fill=primary?Color.FromArgb(92,21,79):Color.FromArgb(17,24,34);
                using(var b=new LinearGradientBrush(ClientRectangle,fill,primary?fill:Color.FromArgb(13,23,35),90f))g.FillPath(b,p);
                using(var pen=new Pen(Enabled?accent:primary?Color.FromArgb(137,38,115):Color.FromArgb(54,60,76),Focused?2:1))g.DrawPath(pen,p);
            }
            TextRenderer.DrawText(g,Text,Font,ClientRectangle,Enabled?Color.White:Color.FromArgb(112,120,141),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
            if(Focused&&ShowFocusCues)ControlPaint.DrawFocusRectangle(g,new Rectangle(9,7,Width-18,Height-14),Color.White,BackColor);
        }
    }

    internal sealed class ImportProgress : Control {
        int value;
        internal int Value {get{return value;}set{this.value=value;Invalidate();}}
        internal ImportProgress(){SetStyle(ControlStyles.UserPaint|ControlStyles.AllPaintingInWmPaint|ControlStyles.OptimizedDoubleBuffer,true);BackColor=Color.FromArgb(8,16,25);}
        protected override void OnPaint(PaintEventArgs e){
            var g=e.Graphics;g.SmoothingMode=SmoothingMode.AntiAlias;
            using(var p=LauncherTheme.Rounded(new RectangleF(1,1,Width-3,Height-3),3))
            using(var border=new Pen(Color.FromArgb(68,106,131)))g.DrawPath(border,p);
            if(value>0)using(var b=new SolidBrush(LauncherTheme.Cyan))g.FillRectangle(b,3,3,(Width-6)*value/100f,Height-6);
        }
    }
}
