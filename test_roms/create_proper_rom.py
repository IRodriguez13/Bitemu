#!/usr/bin/env python3
"""
Enhanced Genesis ROM Creator
Creates a proper Genesis ROM that will be detected correctly
"""

import struct

def create_proper_genesis_rom():
    """Create a proper Genesis test ROM with correct header"""
    
    # Proper Genesis ROM header (512 bytes for real detection)
    header = bytearray(512)
    
    # System info (16 bytes)
    header[0x100:0x110] = b'SEGA MEGA DRIVE'
    
    # Copyright/series info (16 bytes) 
    header[0x110:0x120] = b'2026 GENESIS TEST'
    
    # Domestic name (48 bytes)
    header[0x120:0x150] = b'BITEMU CORE TEST ROM          '  # Padded to 48 bytes
    
    # Overseas name (48 bytes)
    header[0x150:0x180] = b'BITEMU CORE TEST ROM          '  # Padded to 48 bytes
    
    # Product info
    header[0x180:0x18A] = b'GM 00000000-00'  # Product number + version
    header[0x18A:0x18B] = b'J'             # Region code
    header[0x18B:0x18C] = b'0'             # ROM type
    
    # Checksum fields (will be calculated later)
    header[0x18E:0x190] = b'\x00\x00\x00\x00'
    
    # I/O support
    header[0x190:0x192] = b'\x00\x00'  # No special I/O
    
    # Start of ROM data
    start_addr = 0x200
    
    # Simple test code at 0x200
    code = bytearray()
    
    # Padding to start address
    code += b'\x00' * (start_addr - 0x200)
    
    # Test code
    test_code = bytes([
        # Initialize work RAM with test pattern
        0x31, 0xFC, 0x00, 0xFF,  # move.l #$CAFEBABE, $FF0000
        0xCA, 0xFE, 0xBA, 0xBE,
        0xFF, 0x00, 0x00, 0x04,
        
        0x31, 0xFC, 0x00, 0xFF,  # move.l #$DEADBEEF, $FF0004
        0xDE, 0xAD, 0xBE, 0xEF,
        0xFF, 0x00, 0x00, 0x08,
        
        # Test VDP access - disable display
        0x31, 0xFC, 0x00, 0xC0,  # movea.l #$00008100, A0
        0x00, 0x00, 0x81, 0x00,
        0x33, 0xC8, 0x00, 0x04,  # movea.l A0, $C00004
        0x00, 0xC0, 0x00, 0x04,
        0x33, 0xC0, 0x00, 0x00,  # movea.l #$00000000, D0
        0x00, 0x00, 0x00, 0x00,
        0x30, 0xBC, 0x00, 0x04,  # movea.l D0, $C00000
        0x00, 0x00, 0x00, 0x00,
        
        # Store success marker
        0x31, 0xFC, 0x00, 0xFF,  # move.l #$12345678, $FF000C
        0x12, 0x34, 0x56, 0x78,
        0xFF, 0x00, 0x00, 0x0C,
        
        # Simple infinite loop
        0x60, 0xFE  # bra *
    ])
    
    code += test_code
    
    # Combine header and code
    rom_data = header + code
    
    # Pad to at least 8KB (minimum for Genesis)
    while len(rom_data) < 8192:
        rom_data += b'\x00'
    
    # Calculate and set checksum (simple sum for now)
    checksum = sum(rom_data[0x200:]) & 0xFFFF
    header[0x18E] = checksum & 0xFF
    header[0x18F] = (checksum >> 8) & 0xFF
    
    return rom_data

def main():
    print("Creating proper Genesis test ROM...")
    
    rom_data = create_proper_genesis_rom()
    
    with open('proper_genesis_test.bin', 'wb') as f:
        f.write(rom_data)
    
    print(f"Genesis test ROM created: proper_genesis_test.bin")
    print(f"Size: {len(rom_data)} bytes")
    print("This ROM should be detected as Genesis")

if __name__ == "__main__":
    main()
