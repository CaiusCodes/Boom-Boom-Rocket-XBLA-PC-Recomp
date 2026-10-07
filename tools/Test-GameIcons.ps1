param([Parameter(Mandatory)][string]$GameExe,[Parameter(Mandatory)][string]$LauncherExe,[Parameter(Mandatory)][string]$SetupExe)
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
public static class BbrIconResources {
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr LoadLibraryExW(string path,IntPtr file,uint flags);
 [DllImport("kernel32",SetLastError=true)] static extern IntPtr FindResourceW(IntPtr module,IntPtr name,IntPtr type);
 [DllImport("kernel32")] static extern IntPtr LoadResource(IntPtr module,IntPtr resource);
 [DllImport("kernel32")] static extern IntPtr LockResource(IntPtr resource);
 [DllImport("kernel32")] static extern uint SizeofResource(IntPtr module,IntPtr resource);
 [DllImport("kernel32")] static extern bool FreeLibrary(IntPtr module);
 static byte[] Resource(IntPtr module,int name,int type) {
  var r=FindResourceW(module,(IntPtr)name,(IntPtr)type);
  if(r==IntPtr.Zero)throw new Win32Exception(Marshal.GetLastWin32Error());
  var b=new byte[SizeofResource(module,r)];Marshal.Copy(LockResource(LoadResource(module,r)),b,0,b.Length);return b;
 }
 public static string[] Frames(string path,int group) {
  var m=LoadLibraryExW(path,IntPtr.Zero,2);if(m==IntPtr.Zero)throw new Win32Exception(Marshal.GetLastWin32Error());
  try {
   var g=Resource(m,group,14);int count=BitConverter.ToUInt16(g,4);
   if(count!=7)throw new Exception("Expected seven icon sizes: "+path);
   var hashes=new string[count];
   using(var sha=SHA256.Create())for(int i=0;i<count;i++)hashes[i]=BitConverter.ToString(sha.ComputeHash(Resource(m,BitConverter.ToUInt16(g,6+14*i+12),3)));
   return hashes;
  }finally{FreeLibrary(m);}
 }
}
'@
$game=[BbrIconResources]::Frames([IO.Path]::GetFullPath($GameExe),101)
$launcher=[BbrIconResources]::Frames([IO.Path]::GetFullPath($LauncherExe),32512)
$setup=[BbrIconResources]::Frames([IO.Path]::GetFullPath($SetupExe),32512)
for($i=0;$i -lt 7;$i++) {
 if($game[$i] -ne $launcher[$i]) { throw "Game/launcher icon mismatch at frame $i" }
 if($game[$i] -eq $setup[$i]) { throw "Game/setup icons are not distinct at frame $i" }
}
Write-Output 'PASS: seven game/launcher icon sizes match; all differ from setup.'
