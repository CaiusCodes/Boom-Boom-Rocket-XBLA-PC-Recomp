using System;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]
namespace Bbr.Setup {
    internal static class SetupLayoutTests {
        [DllImport("user32.dll")] static extern bool SetProcessDPIAware();
        static void Check(bool ok,string message) { if(!ok)throw new Exception(message); }
        static bool Near(int actual,float expected) { return Math.Abs(actual-expected)<=2; }
        [STAThread] static int Main() {
            try {
                SetProcessDPIAware();Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
                foreach(bool isDlc in new[]{false,true})using(var picker=SetupForm.CreatePackagePicker(isDlc)) {
                    Check(picker.InitialDirectory==System.IO.Path.GetDirectoryName(Application.ExecutablePath),"Package browser must start beside setup, not the current working directory");
                    Check(picker.CheckFileExists&&picker.RestoreDirectory,"Package browser preserves validation and working directory");
                }
                Console.WriteLine("PASS: Both package browsers default to the executable folder");
                float dpi;using(var g=Graphics.FromHwnd(IntPtr.Zero))dpi=g.DpiX;
                foreach(float scale in new[]{1f,1.25f,1.5f,1.75f,2f}) {
                    // Exercise the real constructor's autoscaling pass without
                    // changing the user's Windows display settings. The design
                    // baseline selects the desired runtime/design DPI ratio.
                    using(var form=new SetupForm(dpi/scale)) {
                        var handle=form.Handle;
                        form.PerformLayout();
                        Check(Near(form.ClientSize.Width,960*scale)&&Near(form.ClientSize.Height,686*scale),"Client size at "+scale+": "+form.ClientSize+"; screen "+Screen.PrimaryScreen.Bounds);
                        var controls=form.Controls.Cast<Control>().ToArray();
                        foreach(var c in controls) {
                            Check(c.Left>=0&&c.Top>=0&&c.Right<=form.ClientSize.Width+2&&c.Bottom<=form.ClientSize.Height+2,"Control outside form: "+c.Text+" at "+scale);
                        }
                        var numbers=controls.OfType<Label>().Where(c=>c.Text=="01"||c.Text=="02"||c.Text=="03").OrderBy(c=>c.Top).ToArray();
                        Check(numbers.Length==3,"Three section numbers");
                        for(int i=0;i<3;i++) {
                            Check(Near(numbers[i].Left,83*scale)&&Near(numbers[i].Top,(204+i*111)*scale)&&Near(numbers[i].Width,57*scale),"Unscaled/clipped section number at "+scale);
                        }
                        var fields=controls.OfType<TextBox>().OrderBy(c=>c.Top).ToArray();
                        Check(fields.Length==3,"Three path controls");
                        for(int i=0;i<3;i++) {
                            // Native text must lie within its custom-drawn frame.
                            float top=(240+i*111)*scale;
                            float right=(i==2?898:762)*scale;
                            Check(Near(fields[i].Left,180*scale)&&Near(fields[i].Top,(251+i*111)*scale)&&fields[i].Right<=right&&fields[i].Bottom<=top+42*scale+2,"Path text/frame mismatch at "+scale);
                        }
                        var buttons=controls.OfType<Button>().Where(c=>c.Text=="Browse").OrderBy(c=>c.Top).ToArray();
                        Check(buttons.Length==2&&Near(buttons[0].Top,240*scale)&&Near(buttons[1].Top,351*scale),"Browse/frame alignment at "+scale);
                        var close=controls.OfType<Button>().Single(c=>c.Text=="Close");
                        Check(Near(close.Top,600*scale)&&Near(close.Left,685*scale),"Footer placement at "+scale);
                        // Layout must not progressively rescale on a second pass.
                        var before=controls.Select(c=>c.Bounds).ToArray();
                        form.PerformAutoScale();form.PerformLayout();
                        Check(before.SequenceEqual(controls.Select(c=>c.Bounds)),"Repeated scaling drift at "+scale);
                        Console.WriteLine("PASS: "+(scale*100)+"% startup scaling, field alignment, section numbers, footer and stable repeat layout");
                    }
                }
                return 0;
            } catch(Exception e) {Console.Error.WriteLine(e);return 1;}
        }
    }
}
