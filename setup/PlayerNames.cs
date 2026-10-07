using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace Bbr.Setup {
    internal static class PlayerNames {
        // Keep the supplied list intact. The native Xbox name buffer permits
        // 15 ASCII characters plus NUL; don't silently truncate longer names.
        internal static string[] Load() {
            using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream("Bbr.PlayerNames")) {
                if(stream==null) throw new InvalidDataException("Player name list is missing.");
                using(var reader=new StreamReader(stream,Encoding.UTF8)) {
                    var names=reader.ReadToEnd().Split(new[]{'\r','\n'},StringSplitOptions.RemoveEmptyEntries)
                        .Select(s=>s.Trim()).Where(s=>s.Length>0 && s.Length<=15 &&
                            s.All(c=>(c>='A'&&c<='Z') || (c>='a'&&c<='z') || (c>='0'&&c<='9') || c==' ' || c=='_' || c=='-'))
                        .Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
                    if(names.Length<2) throw new InvalidDataException("At least two supported player names are required.");
                    return names;
                }
            }
        }
        static int Pick(RandomNumberGenerator random,int count) {
            var bytes=new byte[4]; uint value;
            uint limit=uint.MaxValue-(uint.MaxValue%(uint)count);
            do { random.GetBytes(bytes); value=BitConverter.ToUInt32(bytes,0); } while(value>=limit);
            return (int)(value%(uint)count);
        }
        internal static string[] Choose() {
            string[] names=Load();
            using(var random=RandomNumberGenerator.Create()) {
                int one=Pick(random,names.Length),two=Pick(random,names.Length-1);
                if(two>=one)++two;
                return new[]{names[one],names[two]};
            }
        }
        internal static void AssignFreshConfig(string config) {
            var names=Choose();
            File.AppendAllText(config,"\r\n# Names chosen once for this portable installation.\r\n"+
                "bbr_player_one_name = \""+names[0]+"\"\r\n"+
                "bbr_player_two_name = \""+names[1]+"\"\r\n",new UTF8Encoding(false));
        }
    }
}
