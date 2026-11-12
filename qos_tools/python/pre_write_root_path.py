import os, sys, json, shutil, filecmp, configparser

error_occurred = False
# error process
def error_proc(msg, prompt='CommandError', errtype='EARGV'):
    global error_occurred
    if not error_occurred:
        print("! %s: [%s]: `%s'" % (prompt, errtype, msg))
        error_occurred = True

# help cmd process
def help_proc():
    print('Usage: pre_write_root_path.py [arch_path] [sdk_path]\n')

def write_plat_unirtos_root_path(plat, sdk_path):
    # 1. Check if the input path exists and is a directory.
    if not os.path.exists(plat):
        raise FileNotFoundError(f"[pre_write_root_path] Directory does not exist: {plat}")
    if not os.path.isdir(plat):
        raise NotADirectoryError(f"[pre_write_root_path] Path is not a directory: {plat}")

    # 2. Construct the JSON file path (write to the specified directory).
    json_path = os.path.join(plat, "sdk_update_config.json")

    # 3. Convert single backslashes to double backslashes in the passed-in sdk_path path (if needed).
    # sdk_path_normalized = sdk_path.replace("\\", "\\\\")

    # 4. Build JSON data
    config_data = {
        "SDK_PATH": os.path.abspath(sdk_path)
    }

    # 5. Write JSON file
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(config_data, f, indent=4, ensure_ascii=False)

    print(f"unirtos root write success: {json_path}")


def prepare_proc(build_arch_root, sdk_path):
    try:
        write_plat_unirtos_root_path(build_arch_root, sdk_path)
    except Exception as e:
        error_proc(f"[pre_write_root_path] write unirtos root path error: {e}", "CommandError", "EROOT")
        help_proc()
        sys.exit(1)

    print("=======================================")
    print("========prepare success================")
    print("=======================================")

# main entry:
if __name__ == '__main__':
    if len(sys.argv) < 3:
        error_proc("Missing arguments. Need both arch_path and sdk_path", "ArgumentError", "EARG")
        help_proc()
        sys.exit(1)

    print("=======================================")
    print("arch_path: " + sys.argv[1])
    print("sdk_path: " + sys.argv[2])
    prepare_proc(sys.argv[1], sys.argv[2])
