import struct
import os

def get_pe_imports(filename):
    with open(filename, 'rb') as f:
        data = f.read()

    # DOS Header
    if data[:2] != b'MZ':
        print("Not a valid PE file (no MZ signature)")
        return []

    # Offset to PE Header
    pe_offset = struct.unpack('<I', data[0x3c:0x40])[0]
    
    # PE Header Signature
    if data[pe_offset:pe_offset+4] != b'PE\x00\x00':
        print("Not a valid PE file (no PE signature)")
        return []

    # Optional Header
    opt_header_offset = pe_offset + 24
    magic = struct.unpack('<H', data[opt_header_offset:opt_header_offset+2])[0]
    
    if magic == 0x10b: # PE32
        import_dir_offset = opt_header_offset + 96 + 8
    elif magic == 0x20b: # PE32+ (64-bit)
        import_dir_offset = opt_header_offset + 112 + 8
    else:
        print(f"Unknown magic: {hex(magic)}")
        return []

    # Import Directory RVA and Size
    import_rva, import_size = struct.unpack('<II', data[import_dir_offset:import_dir_offset+8])
    if import_rva == 0 or import_size == 0:
        print("No imports found")
        return []

    # Section Headers
    num_sections = struct.unpack('<H', data[pe_offset+6:pe_offset+8])[0]
    sections_offset = opt_header_offset + struct.unpack('<H', data[pe_offset+20:pe_offset+22])[0]

    sections = []
    for i in range(num_sections):
        offset = sections_offset + i * 40
        name = data[offset:offset+8].rstrip(b'\x00').decode('latin-1')
        vsize, vaddr, raw_size, raw_ptr = struct.unpack('<IIII', data[offset+8:offset+24])
        sections.append({
            'name': name,
            'vaddr': vaddr,
            'vsize': vsize,
            'raw_ptr': raw_ptr,
            'raw_size': raw_size
        })

    def rva_to_offset(rva):
        for s in sections:
            if s['vaddr'] <= rva < s['vaddr'] + max(s['vsize'], s['raw_size']):
                return s['raw_ptr'] + (rva - s['vaddr'])
        return None

    import_offset = rva_to_offset(import_rva)
    if import_offset is None:
        print("Could not map Import RVA to file offset")
        return []

    dlls = []
    curr_offset = import_offset
    while True:
        # Import Descriptor is 20 bytes
        desc = data[curr_offset:curr_offset+20]
        if len(desc) < 20 or desc == b'\x00'*20:
            break
        
        characteristics, time_date, forwarder, name_rva, first_thunk = struct.unpack('<IIIII', desc)
        name_offset = rva_to_offset(name_rva)
        if name_offset is not None:
            dll_name = []
            while data[name_offset] != 0:
                dll_name.append(chr(data[name_offset]))
                name_offset += 1
            dlls.append("".join(dll_name))
        curr_offset += 20

    return dlls

if __name__ == '__main__':
    exe_path = r"build/Release/vortex.exe"
    if os.path.exists(exe_path):
        print(f"Imports of {exe_path}:")
        for dll in get_pe_imports(exe_path):
            print(f"  {dll}")
    else:
        print(f"{exe_path} not found")
