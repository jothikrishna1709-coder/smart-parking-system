import os
import sys
import ctypes

def main():
    dll_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'parking_engine.dll'))
    if not os.path.exists(dll_path):
        print(f"Error: {dll_path} not found. Please build the project first.")
        sys.exit(1)

    lib = ctypes.CDLL(dll_path)
    res = lib.run_all_c_tests()
    sys.exit(res)

if __name__ == '__main__':
    main()
