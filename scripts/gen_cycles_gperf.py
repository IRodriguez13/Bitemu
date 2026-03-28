#!/usr/bin/env python3
"""
Genera be1/gen/cycles.gperf, be1/gen/cycles_cb.gperf y be2/gen/cycles.gperf.
T-ciclos de referencia (BE1) alineados a la convención del core (retorno de handlers).
BE2: una entrada por línea de opcode (nibble alto), ciclos orientativos + etiqueta.
"""
from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Nombres mnemónicos por opcode principal (0x00-0xFF) — informativos para perf/símbolos
GB_OPC_SYMS = [
    "NOP",
    "LD_BC_NN",
    "LD_BC_A",
    "INC_BC",
    "INC_B",
    "DEC_B",
    "LD_B_N",
    "RLCA",
    "LD_NN_SP",
    "ADD_HL_BC",
    "LD_A_BC",
    "DEC_BC",
    "INC_C",
    "DEC_C",
    "LD_C_N",
    "RRCA",
    "STOP",
    "LD_DE_NN",
    "LD_DE_A",
    "INC_DE",
    "INC_D",
    "DEC_D",
    "LD_D_N",
    "RLA",
    "JR_N",
    "ADD_HL_DE",
    "LD_A_DE",
    "DEC_DE",
    "INC_E",
    "DEC_E",
    "LD_E_N",
    "RRA",
    "JR_NZ_N",
    "LD_HL_NN",
    "LD_HLI_A",
    "INC_HL",
    "INC_H",
    "DEC_H",
    "LD_H_N",
    "DAA",
    "JR_Z_N",
    "ADD_HL_HL",
    "LD_A_HLI",
    "DEC_HL",
    "INC_L",
    "DEC_L",
    "LD_L_N",
    "CPL",
    "JR_NC_N",
    "LD_SP_NN",
    "LD_HLD_A",
    "INC_SP",
    "INC_HL_IND",
    "DEC_HL_IND",
    "LD_HL_N",
    "SCF",
    "JR_C_N",
    "ADD_HL_SP",
    "LD_A_HLD",
    "DEC_SP",
    "INC_A",
    "DEC_A",
    "LD_A_N",
    "CCF",
]


def _gb_main_tdots(op: int) -> int:
    """T-ciclos de referencia (trabajo típico; ramas condicionales = mínimo documental)."""
    if op == 0xCB:
        return 0
    # 0x40-0x7F: LD r,r' / HALT
    if 0x40 <= op <= 0x7F:
        if op == 0x76:
            return 4
        dst, src = (op >> 3) & 7, op & 7
        return 8 if (dst == 6 or src == 6) else 4
    # 0x80-0xBF: ALU A,r
    if 0x80 <= op <= 0xBF:
        return 8 if (op & 7) == 6 else 4
    # Resto: tabla (Pan Docs / bitemu)
    tbl = {
        0x00: 4,
        0x01: 12,
        0x02: 8,
        0x03: 8,
        0x04: 4,
        0x05: 4,
        0x06: 8,
        0x07: 4,
        0x08: 20,
        0x09: 8,
        0x0A: 8,
        0x0B: 8,
        0x0C: 4,
        0x0D: 4,
        0x0E: 8,
        0x0F: 4,
        0x10: 4,
        0x11: 12,
        0x12: 8,
        0x13: 8,
        0x14: 4,
        0x15: 4,
        0x16: 8,
        0x17: 4,
        0x18: 12,
        0x19: 8,
        0x1A: 8,
        0x1B: 8,
        0x1C: 4,
        0x1D: 4,
        0x1E: 8,
        0x1F: 4,
        0x20: 8,  # branch min
        0x21: 12,
        0x22: 8,
        0x23: 8,
        0x24: 4,
        0x25: 4,
        0x26: 8,
        0x27: 4,
        0x28: 8,
        0x29: 8,
        0x2A: 8,
        0x2B: 8,
        0x2C: 4,
        0x2D: 4,
        0x2E: 8,
        0x2F: 4,
        0x30: 8,
        0x31: 12,
        0x32: 8,
        0x33: 8,
        0x34: 12,
        0x35: 12,
        0x36: 12,
        0x37: 4,
        0x38: 8,
        0x39: 8,
        0x3A: 8,
        0x3B: 8,
        0x3C: 4,
        0x3D: 4,
        0x3E: 8,
        0x3F: 4,
        0xC0: 8,
        0xC1: 12,
        0xC2: 12,
        0xC3: 16,
        0xC4: 12,
        0xC5: 16,
        0xC6: 8,
        0xC7: 16,
        0xC8: 8,
        0xC9: 16,
        0xCA: 12,
        0xCC: 12,
        0xCD: 24,
        0xCE: 8,
        0xCF: 16,
        0xD0: 8,
        0xD1: 12,
        0xD2: 12,
        0xD3: 4,
        0xD4: 12,
        0xD5: 16,
        0xD6: 8,
        0xD7: 16,
        0xD8: 8,
        0xD9: 16,
        0xDA: 12,
        0xDB: 4,
        0xDC: 12,
        0xDD: 4,
        0xDE: 8,
        0xDF: 16,
        0xE0: 12,
        0xE1: 12,
        0xE2: 8,
        0xE3: 4,
        0xE4: 4,
        0xE5: 16,
        0xE6: 8,
        0xE7: 16,
        0xE8: 16,
        0xE9: 4,
        0xEA: 16,
        0xEB: 4,
        0xEC: 4,
        0xED: 4,
        0xEE: 8,
        0xEF: 16,
        0xF0: 12,
        0xF1: 12,
        0xF2: 8,
        0xF3: 4,
        0xF4: 4,
        0xF5: 16,
        0xF6: 8,
        0xF7: 16,
        0xF8: 12,
        0xF9: 8,
        0xFA: 16,
        0xFB: 4,
        0xFC: 4,
        0xFD: 4,
        0xFE: 8,
        0xFF: 16,
    }
    if op not in tbl:
        raise KeyError(f"opcode 0x{op:02X} sin timing en tabla")
    return tbl[op]


def _gb_cb_tdots(cb: int) -> int:
    return 16 if (cb & 7) == 6 else 8


def _cb_sym(cb: int) -> str:
    g = (cb >> 3) & 0x1F
    groups = (
        "RLC",
        "RRC",
        "RL",
        "RR",
        "SLA",
        "SRA",
        "SWAP",
        "SRL",
        "BIT0",
        "BIT1",
        "BIT2",
        "BIT3",
        "BIT4",
        "BIT5",
        "BIT6",
        "BIT7",
        "RES0",
        "RES1",
        "RES2",
        "RES3",
        "RES4",
        "RES5",
        "RES6",
        "RES7",
        "SET0",
        "SET1",
        "SET2",
        "SET3",
        "SET4",
        "SET5",
        "SET6",
        "SET7",
    )
    base = groups[g] if g < len(groups) else "CB"
    return f"CB_{base}_R{cb & 7}"


def _gb_main_sym(op: int) -> str:
    if op == 0xCB:
        return "PREFIX_CB"
    if 0x40 <= op <= 0x7F:
        return "HALT" if op == 0x76 else "LD_r_r"
    if 0x80 <= op <= 0xBF:
        return "ALU_A_r"
    if op < len(GB_OPC_SYMS):
        return GB_OPC_SYMS[op]
    return f"OP_{op:02X}"


def emit_be1_main(path: str) -> None:
    lines = [
        "%language=ANSI-C",
        "%define hash-function-name gb_be1_cycles_hash",
        "%define lookup-function-name gb_be1_cycles_lookup",
        "%readonly-tables",
        "%null-strings",
        "%global-table",
        "%struct-type",
        "struct gb_be1_cycle_sym { const char *name; int tdots; };",
        "%%",
    ]
    for op in range(256):
        td = _gb_main_tdots(op)
        key = f'"{op:02X}"'
        lines.append(f"{key}, {td}")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def emit_be1_mnemonic_h(path: str) -> None:
    """Include C: mnemónicos por opcode (símbolo humano para perf/logs)."""
    out = ['/* Generado por scripts/gen_cycles_gperf.py — no editar */', ""]
    out.append("static const char *const gb_be1_mnem_main[256] = {")
    for op in range(256):
        sym = _gb_main_sym(op)
        sep = "," if op < 255 else ""
        out.append(f'    "{sym}"{sep}')
    out.append("};")
    out.append("")
    out.append("static const char *const gb_be1_mnem_cb[256] = {")
    for cb in range(256):
        sym = _cb_sym(cb)
        sep = "," if cb < 255 else ""
        out.append(f'    "{sym}"{sep}')
    out.append("};")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")


def emit_be1_cb(path: str) -> None:
    lines = [
        "%language=ANSI-C",
        "%define hash-function-name gb_be1_cb_cycles_hash",
        "%define lookup-function-name gb_be1_cb_cycles_lookup",
        "%readonly-tables",
        "%null-strings",
        "%global-table",
        "%struct-type",
        "struct gb_be1_cb_cycle_sym { const char *name; int tdots; };",
        "%%",
    ]
    for cb in range(256):
        key = f'"{cb:02X}"'
        lines.append(f"{key}, {_gb_cb_tdots(cb)}")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


_BE2_LINE_ROWS = [
    ("L0", "line0_quick_dbcc_imm_bit", 8),
    ("L1", "line1_move_b", 8),
    ("L2", "line2_move_l", 8),
    ("L3", "line3_move_w", 8),
    ("L4", "line4_complex_misc", 4),
    ("L5", "line5_addq_subq", 4),
    ("L6", "line6_branch", 10),
    ("L7", "line7_moveq", 4),
    ("L8", "line8_or_div", 8),
    ("L9", "line9_sub_suba", 8),
    ("LA", "lineA_lineA_except", 34),
    ("LB", "lineB_cmpa_eor_cmp", 8),
    ("LC", "lineC_mul_and_exg", 8),
    ("LD", "lineD_add_adda", 8),
    ("LE", "lineE_shift_rotate", 6),
    ("LF", "lineF_lineF_coproc", 34),
]


def emit_be2(path: str) -> None:
    rows = _BE2_LINE_ROWS
    lines = [
        "%language=ANSI-C",
        "%define hash-function-name gen_be2_line_cycles_hash",
        "%define lookup-function-name gen_be2_line_cycles_lookup",
        "%readonly-tables",
        "%null-strings",
        "%global-table",
        "%struct-type",
        "struct gen_be2_line_sym { const char *name; int cycles_ref; };",
        "%%",
    ]
    for key, sym, cy in rows:
        lines.append(f'"{key}", {cy}')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main() -> int:
    emit_be1_main(os.path.join(ROOT, "be1/gen/cycles.gperf"))
    emit_be1_cb(os.path.join(ROOT, "be1/gen/cycles_cb.gperf"))
    emit_be1_mnemonic_h(os.path.join(ROOT, "be1/gen/cycle_mnemonic_tables.h"))
    emit_be2(os.path.join(ROOT, "be2/gen/cycles.gperf"))
    emit_be2_line_names(os.path.join(ROOT, "be2/gen/cycle_line_names.h"), _BE2_LINE_ROWS)
    print(
        "Generado: be1/gen/cycles*.gperf, be1/gen/cycle_mnemonic_tables.h, "
        "be2/gen/cycles.gperf, be2/gen/cycle_line_names.h"
    )
    return 0


def emit_be2_line_names(path: str, rows: list) -> None:
    out = ['/* Generado por scripts/gen_cycles_gperf.py */', ""]
    out.append("static const char *const gen_be2_line_sym[16] = {")
    # Orden L0..L9, LA..LF según valor numérico de línea
    line_to_sym = {i: rows[i][1] for i in range(16)}
    for i in range(16):
        sep = "," if i < 15 else ""
        out.append(f'    "{line_to_sym[i]}"{sep}')
    out.append("};")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    sys.exit(main())
