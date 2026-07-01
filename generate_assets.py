import os
from PIL import Image

def generate_all_assets():
    logo_path = r'c:\Users\mrhyp\OneDrive\Documents\vortexweb\public\logo.png'
    if not os.path.exists(logo_path):
        print(f"Error: Logo file not found at {logo_path}")
        return

    print(f"Loading base logo from {logo_path}...")
    img = Image.open(logo_path)

    # 1. Generate App Icon (src/icons/qbittorrent.ico)
    ico_path = 'src/icons/qbittorrent.ico'
    print(f"Generating multi-resolution app icon: {ico_path}")
    ico_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    img.save(ico_path, format='ICO', sizes=ico_sizes)

    # 2. Generate File Icon (src/icons/qbittorrent_file.ico)
    ico_file_path = 'src/icons/qbittorrent_file.ico'
    print(f"Generating multi-resolution file association icon: {ico_file_path}")
    img.save(ico_file_path, format='ICO', sizes=ico_sizes)

    # 3. Generate Mascot (src/icons/mascot.png) - 94x162, transparent canvas with centered logo scaled to fit
    mascot_path = 'src/icons/mascot.png'
    print(f"Generating mascot image: {mascot_path}")
    mascot_canvas = Image.new("RGBA", (94, 162), (0, 0, 0, 0))
    logo_mascot = img.resize((94, 94), Image.Resampling.LANCZOS)
    mascot_canvas.paste(logo_mascot, (0, 34), logo_mascot)
    mascot_canvas.save(mascot_path, format='PNG')

    # 4. Generate Splash Image (src/icons/splash.png) - 350x300, background #16161A (RGB: 22, 22, 26)
    splash_path = 'src/icons/splash.png'
    print(f"Generating splash image: {splash_path}")
    splash_canvas = Image.new("RGB", (350, 300), (22, 22, 26))
    logo_splash = img.resize((180, 180), Image.Resampling.LANCZOS)
    if logo_splash.mode == 'RGBA':
        splash_canvas.paste(logo_splash, (85, 60), logo_splash)
    else:
        splash_canvas.paste(logo_splash, (85, 60))
    splash_canvas.save(splash_path, format='PNG')

    # 5. Generate Installer Welcome/Finish Banner (dist/windows/welcome.bmp) - 164x314, background #16161A
    welcome_path = 'dist/windows/welcome.bmp'
    print(f"Generating welcome banner: {welcome_path}")
    welcome_canvas = Image.new("RGB", (164, 314), (22, 22, 26))
    logo_welcome = img.resize((120, 120), Image.Resampling.LANCZOS)
    if logo_welcome.mode == 'RGBA':
        welcome_canvas.paste(logo_welcome, (22, 97), logo_welcome)
    else:
        welcome_canvas.paste(logo_welcome, (22, 97))
    welcome_canvas.save(welcome_path, format='BMP')

    # 6. Generate Installer Header Image (dist/windows/header.bmp) - 150x57, background #16161A
    header_path = 'dist/windows/header.bmp'
    print(f"Generating header image: {header_path}")
    header_canvas = Image.new("RGB", (150, 57), (22, 22, 26))
    logo_header = img.resize((48, 48), Image.Resampling.LANCZOS)
    if logo_header.mode == 'RGBA':
        header_canvas.paste(logo_header, (96, 4), logo_header)
    else:
        header_canvas.paste(logo_header, (96, 4))
    header_canvas.save(header_path, format='BMP')

    # 7. Generate PNG Tray and Window Icons (32x32 transparent PNGs)
    tray_size = (32, 32)
    tray_files = [
        'src/icons/qbittorrent-tray.png',
        'src/icons/qbittorrent-tray-light.png',
        'src/icons/qbittorrent-tray-dark.png'
    ]
    for tray_file in tray_files:
        print(f"Generating tray icon: {tray_file}")
        tray_canvas = Image.new("RGBA", tray_size, (0, 0, 0, 0))
        logo_tray = img.resize(tray_size, Image.Resampling.LANCZOS)
        tray_canvas.paste(logo_tray, (0, 0), logo_tray)
        tray_canvas.save(tray_file, format='PNG')

    print("All visual assets generated successfully!")

if __name__ == '__main__':
    generate_all_assets()
