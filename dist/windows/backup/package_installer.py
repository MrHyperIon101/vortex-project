import os
import shutil
import subprocess  # nosec B404

def stage_and_package():
    build_dir = 'build'
    dist_dir = 'dist/windows'
    staging_dir = os.path.join(dist_dir, 'Vortex')

    if not os.path.exists(build_dir):
        print(f"Error: Build directory {build_dir} not found. Please build the project first.")
        return

    # Clean previous staging directory if exists
    if os.path.exists(staging_dir):
        print(f"Cleaning existing staging directory: {staging_dir}")
        shutil.rmtree(staging_dir)

    print(f"Creating staging directory: {staging_dir}")
    os.makedirs(staging_dir)

    # 1. Copy vortex.exe and vortex.pdb
    exe_src = os.path.join(build_dir, 'vortex.exe')
    pdb_src = os.path.join(build_dir, 'vortex.pdb')
    
    if os.path.exists(exe_src):
        print(f"Copying {exe_src} to staging...")
        shutil.copy2(exe_src, os.path.join(staging_dir, 'vortex.exe'))
    else:
        print(f"Warning: {exe_src} not found!")

    if os.path.exists(pdb_src):
        print(f"Copying {pdb_src} to staging...")
        shutil.copy2(pdb_src, os.path.join(staging_dir, 'vortex.pdb'))
    else:
        print(f"Warning: {pdb_src} not found!")

    # 2. Copy all DLLs from build
    for item in os.listdir(build_dir):
        if item.lower().endswith('.dll'):
            src_file = os.path.join(build_dir, item)
            print(f"Copying DLL: {item}")
            shutil.copy2(src_file, staging_dir)

    # 3. Copy subfolders
    subfolders = ['platforms', 'iconengines', 'imageformats', 'styles', 'tls', 'translations']
    for folder in subfolders:
        src_folder = os.path.join(build_dir, folder)
        if os.path.exists(src_folder):
            dest_folder = os.path.join(staging_dir, folder)
            print(f"Copying subfolder: {folder}")
            shutil.copytree(src_folder, dest_folder)
        else:
            print(f"Subfolder {folder} not found, skipping...")

    # 4. Invoke makensis to compile the installer
    # makensis is in Program Files (x86)\NSIS
    nsis_compiler = r"C:\Program Files (x86)\NSIS\makensis.exe"
    nsi_script = 'qbittorrent.nsi'

    if not os.path.exists(nsis_compiler):
        print(f"Error: makensis.exe not found at {nsis_compiler}")
        return

    print("Compiling installer using NSIS...")
    try:
        result = subprocess.run(  # nosec B603
            [nsis_compiler, nsi_script],
            cwd=dist_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=True
        )
        print("NSIS Compilation Output:")
        print(result.stdout)
        print("Installer compiled successfully!")
    except subprocess.CalledProcessError as e:
        print("NSIS Compilation Failed!")
        print("Exit Code:", e.returncode)
        print("Output:\n", e.output)

if __name__ == '__main__':
    stage_and_package()
