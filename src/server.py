import socket

HOST = '127.0.0.1'  # (localhost)
PORT = 5003        

def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        
        print(f"Server is listening on {HOST}:{PORT}...")
        
        conn, addr = server_socket.accept()
        with conn:
            print(f"Connection established with {addr}")
            
            while True:
                # up to 1024 bytes
                data = conn.recv(1024)
                
                if not data:
                    print("empty input.")
                    continue
                
                client_message = data.decode('utf-8')
                print(f"Received from client: {client_message}")
                
                response = f"Received info: '{client_message}'"
                conn.sendall(response.encode('utf-8'))

if __name__ == "__main__":
    start_server()