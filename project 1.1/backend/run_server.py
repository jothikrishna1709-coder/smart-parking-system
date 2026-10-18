import os
import sys
import ctypes

def main():
    port = 8080
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            pass

    dll_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'parking_backend.dll'))
    if not os.path.exists(dll_path):
        print(f"Error: {dll_path} not found. Please build the project first.")
        sys.exit(1)

    print(f"Loading native C backend library: {dll_path}")
    lib = ctypes.CDLL(dll_path)
    print(f"Starting Smart Parking Management System on port {port}...")
    try:
        res = lib.start_c_server(port)
        sys.exit(res)
    except KeyboardInterrupt:
        print("\nServer stopped.")

if __name__ == '__main__':
    main()
