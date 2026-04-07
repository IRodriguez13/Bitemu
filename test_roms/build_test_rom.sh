#!/bin/bash

# Simple Genesis ROM builder
# Creates a basic Genesis ROM from assembly

echo "Building Genesis test ROM..."

# Create a simple binary with our test
cat > simple_test.asm << 'EOF
    org $00000000
    
    ; Genesis header
    dc.b "SEGA MEGA DRIVE "
    dc.b "2026 GENESIS TEST"
    dc.b "TEST ROM FOR BITEMU"
    dc.b "2026-04-05"
    dc.b "0"
    dc.b "JUE"
    dc.b "0"
    ds.b $20, 0
    
    ; Start of program
    org $00000100
start:
    move.l #$DEADBEEF, $FF0000  ; Test start marker
    
    ; Test VDP register access
    move.w #$8100, $C00004     ; VDP control port - select mode 1 register
    move.w #$0000, $C00000     ; VDP data port - disable display
    move.l #$12345678, $FF0004  ; VDP test complete
    
    ; Test YM2612 busy flag
    move.b #$2A, $A04000       ; YM2612 address port
    move.b #$00, $A04001       ; YM2612 data port
    move.b $A04000, D0         ; Read YM2612 status
    move.l D0, $FF0008          ; Store status
    
    ; Test HV counters
    move.w $C00008, D0          ; Read HV counters
    move.l D0, $FF000C          ; Store first reading
    
    ; Small delay loop
    move.w #$1000, D1
delay_loop:
    sub.w #1, D1
    bne delay_loop
    
    move.w $C00008, D0          ; Read HV counters again
    move.l D0, $FF0010          ; Store second reading
    
    ; Compare HV readings
    cmp.l $FF000C, D0
    beq hv_same
    move.l #$AAAAAAAA, $FF0014  ; Different - good
    bra memory_test
hv_same:
    move.l #$55555555, $FF0014  ; Same - might be issue
    
memory_test:
    ; Test memory access
    move.l #$CAFEBABE, $FF0020
    move.l #$DEADBEEF, $FF0024
    move.l $FF0020, D0
    move.l D0, $FF0028
    move.l $FF0024, D0
    move.l D0, $FF002C
    
    ; Verify memory
    cmp.l #$CAFEBABE, D0
    bne mem_fail1
    move.l #$11111111, $FF0030
    bra mem_test2
mem_fail1:
    move.l #$22222222, $FF0030
    
mem_test2:
    cmp.l #$DEADBEEF, D0
    bne mem_fail2
    move.l #$33333333, $FF0034
    bra final
mem_fail2:
    move.l #$44444444, $FF0034
    
final:
    move.l #$CAFEBABE, $FF003C  ; Final marker
    
    ; Infinite loop
loop:
    bra loop
EOF

# Try to assemble with as (if available)
if command -v as >/dev/null 2>&1; then
    as -o simple_test.o simple_test.asm
    ld -o simple_test.bin simple_test.o --oformat binary -Ttext 0x0
    echo "ROM assembled with as/ld"
else
    # Create binary manually with hexdump
    echo "Creating binary manually..."
    cat > simple_test.bin << 'BINEOF'
EOF

echo "Genesis test ROM created: simple_test.bin"
echo "Size: $(wc -c < simple_test.bin) bytes"
