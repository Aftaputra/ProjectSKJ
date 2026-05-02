import socket

HOST = '127.0.0.1'  # (localhost)
PORT = 5003

def start_client():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        try:
            # Connect to the server
            client_socket.connect((HOST, PORT))
            print(f"Connected to server at {HOST}:{PORT}")
            
            while True:
                # Get input from user
                message = input("Enter message to send (or 'quit' to exit): ")
                
                # Check if user wants to quit
                if message.lower() == 'quit':
                    print("Closing connection...")
                    break
                
                # Send message to server
                client_socket.sendall(message.encode('utf-8'))
                
                # Receive response from server
                response = client_socket.recv(1024)
                response_message = response.decode('utf-8')
                print(f"Server response: {response_message}")
                
        except ConnectionRefusedError:
            print("Error: Could not connect to server. Make sure the server is running.")
        except Exception as e:
            print(f"An error occurred: {e}")

if __name__ == "__main__":
    start_client()
