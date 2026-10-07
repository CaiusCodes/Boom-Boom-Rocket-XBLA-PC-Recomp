using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;

namespace Bbr.Setup {
    // STFS addressing follows the existing Extract-STFS.ps1 / Velocity algorithm.
    // Unlike the old script, malformed paths, cycles and short reads fail closed.
    internal sealed class StfsPackage : IDisposable {
        readonly FileStream stream;
        readonly byte[] header;
        readonly long hashBase;
        readonly int shift;
        readonly List<Entry> entries = new List<Entry>();
        internal string DisplayName;
        internal string ContentName;
        internal string ContentId;
        internal uint LicenseMask;
        internal long TotalBytes;
        sealed class Entry {
            internal int Index, Parent, Block, Blocks;
            internal long Size;
            internal bool Directory, Consecutive;
            internal string Name, Path;
        }
        static uint BE(byte[] b, int p) { return ((uint)b[p] << 24) | ((uint)b[p+1] << 16) | ((uint)b[p+2] << 8) | b[p+3]; }
        static int LE24(byte[] b, int p) { return b[p] | (b[p+1] << 8) | (b[p+2] << 16); }
        static void PutBE(byte[] b, int p, uint v) { b[p]=(byte)(v>>24); b[p+1]=(byte)(v>>16); b[p+2]=(byte)(v>>8); b[p+3]=(byte)v; }
        internal StfsPackage(string path, bool dlc) {
            stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
            try {
                header = Read(0, 0xA000);
                string magic = Encoding.ASCII.GetString(header, 0, 4);
                if (magic != "LIVE" && magic != "PIRS" && magic != "CON ") throw new InvalidDataException("Select the original Xbox 360 package file, not a ZIP or an extracted XEX.");
                if (BE(header, 0x360) != 0x5841086A) throw new InvalidDataException("This package belongs to a different game. Select Boom Boom Rocket.");
                if (BE(header, 0x344) != (dlc ? 2u : 0xD0000u)) throw new InvalidDataException(dlc ? "Select the Boom Boom Rocket DLC package in the optional DLC field." : "Select the base-game package in the Game package field. DLC belongs in the optional field.");
                hashBase = ((long)BE(header, 0x340) + 0xFFF) & ~0xFFFL;
                if (hashBase < 0xA000 || hashBase >= stream.Length) throw new InvalidDataException("Invalid STFS header size.");
                shift = (header[0x37B] & 1) != 0 ? 0 : 1;
                DisplayName = Encoding.BigEndianUnicode.GetString(header, 0x411, 0x100).Split('\0')[0];
                // Header content ID is stable even when the user renames the file.
                ContentId = BitConverter.ToString(header, 0x32C, 20).Replace("-", "");
                ContentName = ContentId;
                string originalName = Path.GetFileName(path).ToUpperInvariant();
                if (originalName.StartsWith(ContentId) && (originalName.Length==40 || originalName.Length==42) && originalName.All(c=>"0123456789ABCDEF".Contains(c.ToString()))) ContentName=originalName;
                for (int i=0; i<16; i++) if (BE(header, 0x230 + i*16 + 12) != 0) LicenseMask |= BE(header, 0x230 + i*16 + 8);
                int tableCount = header[0x37C] | (header[0x37D] << 8);
                int block = LE24(header, 0x37E);
                if (tableCount < 1 || tableCount > 4096) throw new InvalidDataException("Invalid STFS file table.");
                var visited = new HashSet<int>();
                for (int t=0; t<tableCount; t++) {
                    if (!visited.Add(block)) throw new InvalidDataException("Cyclic STFS file table.");
                    byte[] table = Read(Address(block), 4096);
                    for (int i=0; i<64; i++) {
                        int p=i*64, length=table[p+0x28]&63;
                        if (length == 0) continue;
                        if (length > 40) throw new InvalidDataException("Invalid STFS filename length.");
                        string name = Encoding.ASCII.GetString(table, p, length);
                        Paths.CheckName(name);
                        var e = new Entry { Index=t*64+i, Name=name, Parent=(table[p+0x32]<<8)|table[p+0x33], Block=LE24(table,p+0x2F), Blocks=LE24(table,p+0x29), Size=BE(table,p+0x34), Directory=(table[p+0x28]&128)!=0, Consecutive=(table[p+0x28]&64)!=0 };
                        if (!e.Directory && (e.Size > stream.Length || e.Size > (long)e.Blocks*4096)) throw new InvalidDataException("Invalid size for " + name);
                        entries.Add(e);
                    }
                    if (t+1 < tableCount) block=Next(block);
                }
                var byIndex=entries.ToDictionary(e=>e.Index);
                var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (var e in entries) {
                    string relative=e.Name;
                    int parent=e.Parent;
                    visited.Clear();
                    while (parent != 65535) {
                        Entry dir;
                        if (!visited.Add(parent) || !byIndex.TryGetValue(parent,out dir) || !dir.Directory) throw new InvalidDataException("Invalid STFS directory tree.");
                        relative=dir.Name+"\\"+relative; parent=dir.Parent;
                        if (relative.Length > 180) throw new InvalidDataException("Package path is too long.");
                    }
                    if (!names.Add(relative)) throw new InvalidDataException("Duplicate package path: " + relative);
                    e.Path=relative;
                    if (!e.Directory) TotalBytes+=e.Size;
                }
                if (entries.Count==0 || TotalBytes==0 || TotalBytes > stream.Length*4) throw new InvalidDataException("The package contains no usable game data.");
            } catch { stream.Dispose(); throw; }
        }
        byte[] Read(long offset, int count) {
            if (offset < 0 || offset > stream.Length-count) throw new InvalidDataException("The package is incomplete or damaged (data lies outside the file).");
            byte[] b=new byte[count]; stream.Position=offset; int n=0;
            while(n<count) { int got=stream.Read(b,n,count-n); if(got==0) throw new EndOfStreamException("Package ended unexpectedly."); n+=got; }
            return b;
        }
        long Address(int block) {
            if(block<0 || block>=0xFFFFFE) throw new InvalidDataException("STFS block chain ended early.");
            long backing=block+(((long)block+0xAA)/0xAA << shift);
            if(block>=0xAA) backing+=(((long)block+0x70E4)/0x70E4 << shift);
            if(block>=0x70E4) backing+=1<<shift;
            return hashBase+backing*4096;
        }
        int Next(int block) {
            long hash=0;
            if(block>=0xAA) {
                hash=(long)(block/0xAA)*(shift==0?0xAB:0xAC)+((block/0x70E4+1)<<shift);
                if(block/0x70E4 != 0) hash+=1<<shift;
            }
            byte[] b=Read(hashBase+hash*4096+(block%0xAA)*24+21,3);
            int next=(b[0]<<16)|(b[1]<<8)|b[2];
            if(next>=0xFFFFFE) throw new InvalidDataException("STFS block chain ended early.");
            return next;
        }
        internal void Extract(string root, Action<int,string> progress, CancellationToken cancel) {
            Directory.CreateDirectory(root); long done=0; int last=-1;
            foreach(var e in entries) {
                cancel.ThrowIfCancellationRequested();
                string path=Paths.Child(root,e.Path);
                if(e.Directory) { Directory.CreateDirectory(path); continue; }
                Directory.CreateDirectory(Path.GetDirectoryName(path));
                using(var output=new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.None)) {
                    long remaining=e.Size; int block=e.Block; var visited=new HashSet<int>();
                    while(remaining>0) {
                        cancel.ThrowIfCancellationRequested();
                        if(!visited.Add(block)) throw new InvalidDataException("Cyclic STFS file chain: "+e.Path);
                        int count=(int)Math.Min(4096,remaining); byte[] bytes=Read(Address(block),count);
                        output.Write(bytes,0,count); remaining-=count; done+=count;
                        int percent=(int)(done*100/TotalBytes);
                        if(percent!=last) { last=percent; progress(percent,e.Path); }
                        if(remaining>0) block=e.Consecutive ? block+1 : Next(block);
                    }
                }
            }
        }
        internal void WriteDlcHeader(string file) {
            // SDK 0.9.0: XCONTENT_DATA 0x134, 4-byte alignment pad,
            // XUID at 0x138, title at 0x140, struct size 0x148; host LE license.
            var b=new byte[LicenseMask==0?0x148:0x14C];
            PutBE(b,0,1); PutBE(b,4,2); PutBE(b,0x140,0x5841086A);
            byte[] name=Encoding.BigEndianUnicode.GetBytes(DisplayName.Length>127?DisplayName.Substring(0,127):DisplayName);
            Buffer.BlockCopy(name,0,b,8,name.Length);
            byte[] id=Encoding.ASCII.GetBytes(ContentName); Buffer.BlockCopy(id,0,b,0x108,id.Length);
            if(LicenseMask!=0) Buffer.BlockCopy(BitConverter.GetBytes(LicenseMask),0,b,0x148,4);
            Directory.CreateDirectory(Path.GetDirectoryName(file)); File.WriteAllBytes(file,b);
        }
        public void Dispose() { stream.Dispose(); }
    }
    internal static class Paths {
        internal static string Extended(string path) {
            path=Path.GetFullPath(path);
            if(path.StartsWith(@"\\?\"))return path;
            return path.StartsWith(@"\\") ? @"\\?\UNC\"+path.Substring(2) : @"\\?\"+path;
        }
        internal static void CheckName(string name) {
            string stem=name.Split('.')[0].ToUpperInvariant();
            if(string.IsNullOrWhiteSpace(name) || name=="." || name==".." || name.EndsWith(".") || name.EndsWith(" ") || name.IndexOfAny(Path.GetInvalidFileNameChars())>=0 || name.Any(c=>c<32 || c>126) || new[]{"CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9","LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"}.Contains(stem)) throw new InvalidDataException("Unsafe package filename: "+name);
        }
        internal static string Child(string root,string relative) {
            root=Path.GetFullPath(root).TrimEnd('\\');
            // ZIP entries use '/', but extended Windows paths (\\?\) bypass
            // Win32 slash normalization. Normalize before combining, not only
            // while validating the components.
            relative=relative.Replace('/','\\');
            if(Path.IsPathRooted(relative)) throw new InvalidDataException("Absolute package path rejected.");
            foreach(string part in relative.Replace('/','\\').Split('\\')) CheckName(part);
            string path=Path.GetFullPath(Path.Combine(root,relative));
            if(!path.StartsWith(root+"\\",StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Path leaves the install folder.");
            CheckParents(path); return path;
        }
        internal static void CheckParents(string path) {
            for(string p=Path.GetFullPath(path); p!=null; p=Path.GetDirectoryName(p))
                if((Directory.Exists(p)||File.Exists(p)) && (File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0) throw new IOException("Choose a local folder without directory links: "+p);
        }
    }
}
