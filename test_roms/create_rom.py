#!/usr/bin/env python3
"""
Simple Genesis ROM Creator
Creates a basic test ROM for Genesis emulator testing
"""

import struct

def create_genesis_rom():
    """Create a simple Genesis test ROM"""
    
    # ROM header (64 bytes)
    header = b'SEGA MEGA DRIVE '  # 16 bytes
    header += b'2026 GENESIS TEST'  # 16 bytes  
    header += b'BITEMU TEST ROM  '  # 16 bytes
    header += b'2026-04-05'        # 10 bytes
    header += b'\x00'              # Domestic name
    header += b'JUE'              # Country codes
    header += b'\x00' * 5         # Padding
    
    # Simple test code
    code = bytearray()
    
    # Test pattern at 0x100
    code += b'\x00' * 0x100  # Padding to 0x100
    
    # Move test marker to RAM
    code += bytes([
        0x22, 0x7C, 0x00, 0x00,  # move.l #$DEADBEEF, $FF0000
        0xDE, 0xAD, 0xBE, 0xEF,
        0xFF, 0x00, 0x00, 0x04,
        
        # Test VDP - disable display
        0x31, 0xFC, 0x00, 0xC0,  # move.w #$8100, $C00004
        0x81, 0x00,
        0x32, 0xFC, 0x00, 0xC0,  # move.w #$0000, $C00000  
        0x00, 0x00,
        
        # Store success marker
        0x22, 0x7C, 0x00, 0x00,  # move.l #$12345678, $FF0004
        0x12, 0x34, 0x56, 0x78,
        0xFF, 0x00, 0x00, 0x08,
        
        # Infinite loop
        0x60, 0xFE  # bra *
    ])
    
    # Combine header and code
    rom_data = header + code
    
    # Pad to 64KB
    while len(rom_data) < 65536:
        rom_data += b'\x00'
    
    return rom_data[:65536]

def main():
    print("Creating Genesis test ROM...")
    
    rom_data = create_genesis_rom()
    
    with open('simple_test.bin', 'wb') as f:
        f.write(rom_data)
    
    print(f"Genesis test ROM created: simple_test.bin")
    print(f"Size: {len(rom_data)} bytes")

if __name__ == "__main__":
    main()
