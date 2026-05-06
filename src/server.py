import socket

HOST = '0.0.0.0'  # Listen on all available interfaces
PORT = 5003        

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        
        # Allow immediate reuse of the port after restarting the script
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        
        print(f"Server is listening on {HOST}:{PORT}...")
        
        while True:
            try:
                conn, addr = server_socket.accept()
                with conn:
                    print(f"\nConnection established with {addr}")
                    
                    while True:
                        # up to 1024 bytes
                        data = conn.recv(1024)
                        
                        if not data:
                            print(f"Connection closed by {addr}.")
                            break
                        
                        client_message = data.decode('utf-8').strip()
                        if client_message:
                            print(f"Received from {addr[0]}: {client_message}")
                            
                            response = f"Received info: '{client_message}'\n"
                            conn.sendall(response.encode('utf-8'))
            except KeyboardInterrupt:
                print("\nServer shutting down.")
                break
            except Exception as e:
                print(f"An error occurred: {e}")

if __name__ == "__main__":
    start_server()