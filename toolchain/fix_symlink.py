import os
import shutil

def replace_symlinks_with_files(root):
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            full_path = os.path.join(dirpath, name)
            if os.path.islink(full_path):
                target = os.readlink(full_path)
                if not os.path.isabs(target):
                    target = os.path.join(os.path.dirname(full_path), target)
                if os.path.exists(target):
                    print(f"Replacing symlink: {full_path} -> {target}")
                    os.remove(full_path)
                    shutil.copy2(target, full_path)
                else:
                    print(f"Warning: symlink target not found: {full_path} -> {target}")

replace_symlinks_with_files("toolchain/")
