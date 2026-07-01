import os
import glob

def rebrand_translations():
    translations_dir = 'dist/windows/installer-translations'
    if not os.path.exists(translations_dir):
        print(f"Error: Translations directory not found at {translations_dir}")
        return

    nsh_pattern = os.path.join(translations_dir, '*.nsh')
    translation_files = glob.glob(nsh_pattern)

    print(f"Found {len(translation_files)} translation files to rebrand.")

    for file_path in translation_files:
        try:
            # Read file with UTF-8 encoding
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()

            # Perform case-sensitive replacements
            # Replace qBittorrent with Vortex
            # Replace qbittorrent with vortex
            new_content = content.replace('qBittorrent', 'Vortex').replace('qbittorrent', 'vortex')

            if new_content != content:
                # Write file back with UTF-8 encoding
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                print(f"Rebranded: {os.path.basename(file_path)}")
            else:
                pass # No changes needed

        except Exception as e:
            print(f"Failed to process {file_path}: {e}")

    print("Installer translation files rebranding completed!")

if __name__ == '__main__':
    rebrand_translations()
