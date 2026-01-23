#!/usr/bin/python3

import os
import sys
import platform
import subprocess

try:
    import ctypes
    import winreg
except ImportError:
    pass

SCRIPT_NAME = "vulcli.py"
COMMAND_NAME = "vulcli"

def is_admin():
    # check if script has admin priviledges.
    try:
        if platform.system() == "Windows":
            if hasattr(ctypes, 'windll'):
                return ctypes.windll.shell32.IsUserAnAdmin()
            return False
        else:
            return os.geteuid() == 0
    except:
        return False


def get_script_path():
    # returns the absolute path to vulcli.py
    current_dir = os.path.dirname(os.path.abspath(__file__))
    script_path = os.path.join(current_dir, SCRIPT_NAME)

    if not os.path.exists(script_path):
        print(f"Error: Could not find {SCRIPT_NAME} in {current_dir}")
        sys.exit(1)
    return script_path, current_dir


def install_unix():
    # install logic for linux and macos

    src, _ = get_script_path()
    dst = f"/usr/local/bin/{COMMAND_NAME}"

    print(f"Detected Unix-like OS")
    print(f" Target {dst}")

    try:
        os.chmod(src, 0o755)
        print("Made Script executable")
    except Exception as e:
        print(f"Failed to chmod: {e}")

    try:
        if os.path.exists(dst) or os.path.islink(dst):
            os.remove(dst)

        os.symlink(src, dst)
        print(f" Symlink created: {dst} -> {src}")
        print(f"Installation complete! You can use '{COMMAND_NAME}' now.")

    except PermissionError:
        print("\n Permission Denied\n")
        print("please run this installation with sudo\n")
        print("command: sudo python3 install.py\n")
    except Exception as e:
        print(f"Installation Failed: {e}")

def install_windows():
    # installation logic for windows platform
    src, script_dir = get_script_path()
    bat_path = os.path.join(script_dir, f"{COMMAND_NAME}.bat")

    print("Detected Windows")

    try:
        with open(bat_path, "w") as f:
            f.write('@echo off\n')
            f.write(f'python "{src}" %*\n')
        print(f"Wrapper created: {bat_path}")
    except Exception as e:
        print(f"Failed to create batch file: {e}")
        return

    print("... Updating User Path in Registry ...")
    try:
        key = winreg.OpenKey(
            winreg.HKEY_CURRENT_USER,
            "Environment",
            0,
            winreg.KEY_ALL_ACCESS
        )
        try:
            current_path, _ = winreg.QueryValueEx(key, "path")
        except FileNotFoundError:
            current_path = ""

        if script_dir.lower() in current_path.lower():
            print("Directory is already in PATH.")
        else:
            new_path = current_path
            if new_path and not new_path.endswith(";"):
                new_path += ";"
            new_path += script_dir

            winreg.SetValueEx(key, "Path", 0, winreg.REG_EXPAND_SZ, new_path)
            print("Added to PATH successfully.")
            print("IMPORTANT: Close and Reopen your terminal to use the command.")

        winreg.CloseKey(key)

    except Exception as e:
        print(f"Failed to update Registry: {e}")
        print(" You may need to add the folder to your PATH manually.")


def main():
    print(f"--- Installing {COMMAND_NAME} Tool ---")
    os_type = platform.system()

    if os_type == "Linux" or os_type == "Darwin":
        install_unix()
    elif os_type == "Windows":
        install_windows()
    else:
        print(f"Unsupported OS: {os_type}")


if __name__ == "__main__":
    main()


