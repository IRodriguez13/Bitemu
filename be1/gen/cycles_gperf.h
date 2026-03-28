/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -I -C -t -L ANSI-C --output-file=be1/gen/cycles_gperf.h be1/gen/cycles.gperf  */
/* Computed positions: -k'1-2' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 8 "be1/gen/cycles.gperf"
struct gb_be1_cycle_sym { const char *name; int tdots; };
#include <string.h>

#define TOTAL_KEYWORDS 256
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 2
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 933
/* maximum key range = 933, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
gb_be1_cycles_hash (register const char *str, register size_t len)
{
  static const unsigned short asso_values[] =
    {
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 481, 211,
      186,   1, 508, 213,  53,  28,   3, 390, 934, 934,
      934, 934, 934, 934, 934, 250, 120,  95,  70,  45,
       20, 321, 301,  46, 298, 253,  78, 425, 410, 405,
      934, 934, 934, 934, 934, 934, 934, 260, 125,  15,
       10,   5,   0, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934, 934, 934,
      934, 934, 934, 934, 934, 934, 934, 934
    };
  return asso_values[(unsigned char)str[1]+22] + asso_values[(unsigned char)str[0]];
}

static const struct gb_be1_cycle_sym wordlist[] =
  {
    {(char*)0},
#line 73 "be1/gen/cycles.gperf"
    {"3F", 4},
    {(char*)0},
#line 153 "be1/gen/cycles.gperf"
    {"8F", 4},
    {(char*)0}, {(char*)0},
#line 72 "be1/gen/cycles.gperf"
    {"3E", 8},
    {(char*)0},
#line 152 "be1/gen/cycles.gperf"
    {"8E", 8},
    {(char*)0}, {(char*)0},
#line 71 "be1/gen/cycles.gperf"
    {"3D", 4},
    {(char*)0},
#line 151 "be1/gen/cycles.gperf"
    {"8D", 4},
    {(char*)0}, {(char*)0},
#line 70 "be1/gen/cycles.gperf"
    {"3C", 4},
    {(char*)0},
#line 150 "be1/gen/cycles.gperf"
    {"8C", 4},
    {(char*)0},
#line 265 "be1/gen/cycles.gperf"
    {"FF", 16},
#line 58 "be1/gen/cycles.gperf"
    {"30", 8},
    {(char*)0},
#line 138 "be1/gen/cycles.gperf"
    {"80", 4},
    {(char*)0},
#line 264 "be1/gen/cycles.gperf"
    {"FE", 8},
    {(char*)0}, {(char*)0},
#line 137 "be1/gen/cycles.gperf"
    {"7F", 4},
    {(char*)0},
#line 263 "be1/gen/cycles.gperf"
    {"FD", 4},
    {(char*)0}, {(char*)0},
#line 136 "be1/gen/cycles.gperf"
    {"7E", 8},
    {(char*)0},
#line 262 "be1/gen/cycles.gperf"
    {"FC", 4},
    {(char*)0}, {(char*)0},
#line 135 "be1/gen/cycles.gperf"
    {"7D", 4},
    {(char*)0},
#line 250 "be1/gen/cycles.gperf"
    {"F0", 12},
    {(char*)0}, {(char*)0},
#line 134 "be1/gen/cycles.gperf"
    {"7C", 4},
    {(char*)0},
#line 249 "be1/gen/cycles.gperf"
    {"EF", 16},
    {(char*)0},
#line 61 "be1/gen/cycles.gperf"
    {"33", 8},
#line 122 "be1/gen/cycles.gperf"
    {"70", 8},
#line 141 "be1/gen/cycles.gperf"
    {"83", 4},
#line 248 "be1/gen/cycles.gperf"
    {"EE", 8},
    {(char*)0}, {(char*)0},
#line 121 "be1/gen/cycles.gperf"
    {"6F", 4},
    {(char*)0},
#line 247 "be1/gen/cycles.gperf"
    {"ED", 4},
    {(char*)0}, {(char*)0},
#line 120 "be1/gen/cycles.gperf"
    {"6E", 8},
    {(char*)0},
#line 246 "be1/gen/cycles.gperf"
    {"EC", 4},
    {(char*)0}, {(char*)0},
#line 119 "be1/gen/cycles.gperf"
    {"6D", 4},
    {(char*)0},
#line 234 "be1/gen/cycles.gperf"
    {"E0", 12},
#line 253 "be1/gen/cycles.gperf"
    {"F3", 4},
    {(char*)0},
#line 118 "be1/gen/cycles.gperf"
    {"6C", 4},
    {(char*)0},
#line 233 "be1/gen/cycles.gperf"
    {"DF", 16},
    {(char*)0}, {(char*)0},
#line 106 "be1/gen/cycles.gperf"
    {"60", 4},
#line 125 "be1/gen/cycles.gperf"
    {"73", 8},
#line 232 "be1/gen/cycles.gperf"
    {"DE", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 64 "be1/gen/cycles.gperf"
    {"36", 12},
#line 231 "be1/gen/cycles.gperf"
    {"DD", 4},
#line 144 "be1/gen/cycles.gperf"
    {"86", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 230 "be1/gen/cycles.gperf"
    {"DC", 12},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 218 "be1/gen/cycles.gperf"
    {"D0", 8},
#line 237 "be1/gen/cycles.gperf"
    {"E3", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 217 "be1/gen/cycles.gperf"
    {"CF", 16},
    {(char*)0}, {(char*)0},
#line 256 "be1/gen/cycles.gperf"
    {"F6", 8},
#line 109 "be1/gen/cycles.gperf"
    {"63", 4},
#line 216 "be1/gen/cycles.gperf"
    {"CE", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 215 "be1/gen/cycles.gperf"
    {"CD", 24},
#line 128 "be1/gen/cycles.gperf"
    {"76", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 214 "be1/gen/cycles.gperf"
    {"CC", 12},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 202 "be1/gen/cycles.gperf"
    {"C0", 8},
#line 221 "be1/gen/cycles.gperf"
    {"D3", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 201 "be1/gen/cycles.gperf"
    {"BF", 4},
    {(char*)0}, {(char*)0},
#line 240 "be1/gen/cycles.gperf"
    {"E6", 8},
    {(char*)0},
#line 200 "be1/gen/cycles.gperf"
    {"BE", 8},
#line 69 "be1/gen/cycles.gperf"
    {"3B", 8},
    {(char*)0},
#line 149 "be1/gen/cycles.gperf"
    {"8B", 4},
    {(char*)0},
#line 199 "be1/gen/cycles.gperf"
    {"BD", 4},
#line 112 "be1/gen/cycles.gperf"
    {"66", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 198 "be1/gen/cycles.gperf"
    {"BC", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 186 "be1/gen/cycles.gperf"
    {"B0", 4},
#line 205 "be1/gen/cycles.gperf"
    {"C3", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 261 "be1/gen/cycles.gperf"
    {"FB", 4},
    {(char*)0}, {(char*)0},
#line 224 "be1/gen/cycles.gperf"
    {"D6", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 133 "be1/gen/cycles.gperf"
    {"7B", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 189 "be1/gen/cycles.gperf"
    {"B3", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 245 "be1/gen/cycles.gperf"
    {"EB", 4},
    {(char*)0}, {(char*)0},
#line 208 "be1/gen/cycles.gperf"
    {"C6", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 117 "be1/gen/cycles.gperf"
    {"6B", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 57 "be1/gen/cycles.gperf"
    {"2F", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 56 "be1/gen/cycles.gperf"
    {"2E", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 229 "be1/gen/cycles.gperf"
    {"DB", 4},
#line 55 "be1/gen/cycles.gperf"
    {"2D", 4},
    {(char*)0},
#line 192 "be1/gen/cycles.gperf"
    {"B6", 8},
    {(char*)0}, {(char*)0},
#line 54 "be1/gen/cycles.gperf"
    {"2C", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 42 "be1/gen/cycles.gperf"
    {"20", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 41 "be1/gen/cycles.gperf"
    {"1F", 4},
    {(char*)0},
#line 105 "be1/gen/cycles.gperf"
    {"5F", 4},
    {(char*)0}, {(char*)0},
#line 40 "be1/gen/cycles.gperf"
    {"1E", 8},
    {(char*)0},
#line 104 "be1/gen/cycles.gperf"
    {"5E", 8},
    {(char*)0},
#line 213 "be1/gen/cycles.gperf"
    {"CB", 0},
#line 39 "be1/gen/cycles.gperf"
    {"1D", 4},
    {(char*)0},
#line 103 "be1/gen/cycles.gperf"
    {"5D", 4},
    {(char*)0}, {(char*)0},
#line 38 "be1/gen/cycles.gperf"
    {"1C", 4},
    {(char*)0},
#line 102 "be1/gen/cycles.gperf"
    {"5C", 4},
    {(char*)0}, {(char*)0},
#line 26 "be1/gen/cycles.gperf"
    {"10", 4},
#line 45 "be1/gen/cycles.gperf"
    {"23", 8},
#line 90 "be1/gen/cycles.gperf"
    {"50", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 197 "be1/gen/cycles.gperf"
    {"BB", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 185 "be1/gen/cycles.gperf"
    {"AF", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 63 "be1/gen/cycles.gperf"
    {"35", 12},
#line 184 "be1/gen/cycles.gperf"
    {"AE", 8},
#line 143 "be1/gen/cycles.gperf"
    {"85", 4},
#line 29 "be1/gen/cycles.gperf"
    {"13", 8},
    {(char*)0},
#line 93 "be1/gen/cycles.gperf"
    {"53", 4},
#line 183 "be1/gen/cycles.gperf"
    {"AD", 4},
#line 68 "be1/gen/cycles.gperf"
    {"3A", 8},
    {(char*)0},
#line 148 "be1/gen/cycles.gperf"
    {"8A", 4},
#line 48 "be1/gen/cycles.gperf"
    {"26", 8},
#line 182 "be1/gen/cycles.gperf"
    {"AC", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 170 "be1/gen/cycles.gperf"
    {"A0", 4},
    {(char*)0}, {(char*)0},
#line 255 "be1/gen/cycles.gperf"
    {"F5", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 260 "be1/gen/cycles.gperf"
    {"FA", 16},
#line 127 "be1/gen/cycles.gperf"
    {"75", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 132 "be1/gen/cycles.gperf"
    {"7A", 4},
#line 32 "be1/gen/cycles.gperf"
    {"16", 8},
    {(char*)0},
#line 96 "be1/gen/cycles.gperf"
    {"56", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 173 "be1/gen/cycles.gperf"
    {"A3", 4},
    {(char*)0},
#line 239 "be1/gen/cycles.gperf"
    {"E5", 16},
#line 62 "be1/gen/cycles.gperf"
    {"34", 12},
    {(char*)0},
#line 142 "be1/gen/cycles.gperf"
    {"84", 4},
#line 60 "be1/gen/cycles.gperf"
    {"32", 8},
    {(char*)0},
#line 140 "be1/gen/cycles.gperf"
    {"82", 4},
#line 244 "be1/gen/cycles.gperf"
    {"EA", 16},
#line 111 "be1/gen/cycles.gperf"
    {"65", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 53 "be1/gen/cycles.gperf"
    {"2B", 8},
    {(char*)0},
#line 116 "be1/gen/cycles.gperf"
    {"6A", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 254 "be1/gen/cycles.gperf"
    {"F4", 4},
    {(char*)0}, {(char*)0},
#line 252 "be1/gen/cycles.gperf"
    {"F2", 8},
#line 59 "be1/gen/cycles.gperf"
    {"31", 12},
#line 223 "be1/gen/cycles.gperf"
    {"D5", 16},
#line 139 "be1/gen/cycles.gperf"
    {"81", 4},
    {(char*)0},
#line 126 "be1/gen/cycles.gperf"
    {"74", 8},
    {(char*)0},
#line 176 "be1/gen/cycles.gperf"
    {"A6", 8},
#line 124 "be1/gen/cycles.gperf"
    {"72", 8},
#line 228 "be1/gen/cycles.gperf"
    {"DA", 12},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 37 "be1/gen/cycles.gperf"
    {"1B", 8},
    {(char*)0},
#line 101 "be1/gen/cycles.gperf"
    {"5B", 4},
    {(char*)0}, {(char*)0},
#line 251 "be1/gen/cycles.gperf"
    {"F1", 12},
    {(char*)0},
#line 238 "be1/gen/cycles.gperf"
    {"E4", 4},
    {(char*)0}, {(char*)0},
#line 236 "be1/gen/cycles.gperf"
    {"E2", 8},
    {(char*)0},
#line 207 "be1/gen/cycles.gperf"
    {"C5", 16},
#line 123 "be1/gen/cycles.gperf"
    {"71", 8},
    {(char*)0},
#line 110 "be1/gen/cycles.gperf"
    {"64", 4},
    {(char*)0}, {(char*)0},
#line 108 "be1/gen/cycles.gperf"
    {"62", 4},
#line 212 "be1/gen/cycles.gperf"
    {"CA", 12},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 235 "be1/gen/cycles.gperf"
    {"E1", 12},
    {(char*)0},
#line 222 "be1/gen/cycles.gperf"
    {"D4", 12},
    {(char*)0}, {(char*)0},
#line 220 "be1/gen/cycles.gperf"
    {"D2", 12},
    {(char*)0},
#line 191 "be1/gen/cycles.gperf"
    {"B5", 4},
#line 107 "be1/gen/cycles.gperf"
    {"61", 4},
#line 181 "be1/gen/cycles.gperf"
    {"AB", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 196 "be1/gen/cycles.gperf"
    {"BA", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 169 "be1/gen/cycles.gperf"
    {"9F", 4},
#line 219 "be1/gen/cycles.gperf"
    {"D1", 12},
    {(char*)0},
#line 206 "be1/gen/cycles.gperf"
    {"C4", 12},
    {(char*)0},
#line 168 "be1/gen/cycles.gperf"
    {"9E", 8},
#line 204 "be1/gen/cycles.gperf"
    {"C2", 12},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 167 "be1/gen/cycles.gperf"
    {"9D", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 166 "be1/gen/cycles.gperf"
    {"9C", 4},
#line 67 "be1/gen/cycles.gperf"
    {"39", 8},
    {(char*)0},
#line 147 "be1/gen/cycles.gperf"
    {"89", 4},
    {(char*)0},
#line 154 "be1/gen/cycles.gperf"
    {"90", 4},
#line 66 "be1/gen/cycles.gperf"
    {"38", 8},
    {(char*)0},
#line 146 "be1/gen/cycles.gperf"
    {"88", 4},
    {(char*)0}, {(char*)0},
#line 203 "be1/gen/cycles.gperf"
    {"C1", 12},
    {(char*)0},
#line 190 "be1/gen/cycles.gperf"
    {"B4", 4},
    {(char*)0}, {(char*)0},
#line 188 "be1/gen/cycles.gperf"
    {"B2", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 259 "be1/gen/cycles.gperf"
    {"F9", 8},
#line 65 "be1/gen/cycles.gperf"
    {"37", 4},
    {(char*)0},
#line 145 "be1/gen/cycles.gperf"
    {"87", 4},
    {(char*)0},
#line 258 "be1/gen/cycles.gperf"
    {"F8", 12},
    {(char*)0}, {(char*)0},
#line 131 "be1/gen/cycles.gperf"
    {"79", 4},
    {(char*)0}, {(char*)0},
#line 157 "be1/gen/cycles.gperf"
    {"93", 4},
    {(char*)0},
#line 130 "be1/gen/cycles.gperf"
    {"78", 4},
#line 47 "be1/gen/cycles.gperf"
    {"25", 4},
    {(char*)0},
#line 187 "be1/gen/cycles.gperf"
    {"B1", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 257 "be1/gen/cycles.gperf"
    {"F7", 16},
#line 52 "be1/gen/cycles.gperf"
    {"2A", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 243 "be1/gen/cycles.gperf"
    {"E9", 4},
    {(char*)0}, {(char*)0},
#line 129 "be1/gen/cycles.gperf"
    {"77", 8},
    {(char*)0},
#line 242 "be1/gen/cycles.gperf"
    {"E8", 16},
    {(char*)0}, {(char*)0},
#line 115 "be1/gen/cycles.gperf"
    {"69", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 114 "be1/gen/cycles.gperf"
    {"68", 4},
#line 31 "be1/gen/cycles.gperf"
    {"15", 4},
    {(char*)0},
#line 95 "be1/gen/cycles.gperf"
    {"55", 4},
    {(char*)0},
#line 160 "be1/gen/cycles.gperf"
    {"96", 8},
    {(char*)0},
#line 241 "be1/gen/cycles.gperf"
    {"E7", 16},
#line 36 "be1/gen/cycles.gperf"
    {"1A", 8},
    {(char*)0},
#line 100 "be1/gen/cycles.gperf"
    {"5A", 4},
    {(char*)0},
#line 227 "be1/gen/cycles.gperf"
    {"D9", 16},
    {(char*)0}, {(char*)0},
#line 113 "be1/gen/cycles.gperf"
    {"67", 4},
    {(char*)0},
#line 226 "be1/gen/cycles.gperf"
    {"D8", 8},
#line 25 "be1/gen/cycles.gperf"
    {"0F", 4},
    {(char*)0}, {(char*)0},
#line 46 "be1/gen/cycles.gperf"
    {"24", 4},
    {(char*)0},
#line 24 "be1/gen/cycles.gperf"
    {"0E", 8},
#line 44 "be1/gen/cycles.gperf"
    {"22", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 23 "be1/gen/cycles.gperf"
    {"0D", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 225 "be1/gen/cycles.gperf"
    {"D7", 16},
#line 22 "be1/gen/cycles.gperf"
    {"0C", 4},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 211 "be1/gen/cycles.gperf"
    {"C9", 16},
#line 10 "be1/gen/cycles.gperf"
    {"00", 4},
    {(char*)0},
#line 175 "be1/gen/cycles.gperf"
    {"A5", 4},
    {(char*)0},
#line 210 "be1/gen/cycles.gperf"
    {"C8", 8},
    {(char*)0},
#line 43 "be1/gen/cycles.gperf"
    {"21", 12},
#line 89 "be1/gen/cycles.gperf"
    {"4F", 4},
#line 30 "be1/gen/cycles.gperf"
    {"14", 4},
#line 180 "be1/gen/cycles.gperf"
    {"AA", 4},
#line 94 "be1/gen/cycles.gperf"
    {"54", 4},
#line 28 "be1/gen/cycles.gperf"
    {"12", 8},
#line 88 "be1/gen/cycles.gperf"
    {"4E", 8},
#line 92 "be1/gen/cycles.gperf"
    {"52", 4},
#line 165 "be1/gen/cycles.gperf"
    {"9B", 4},
    {(char*)0}, {(char*)0},
#line 87 "be1/gen/cycles.gperf"
    {"4D", 4},
    {(char*)0},
#line 209 "be1/gen/cycles.gperf"
    {"C7", 16},
    {(char*)0}, {(char*)0},
#line 86 "be1/gen/cycles.gperf"
    {"4C", 4},
    {(char*)0},
#line 195 "be1/gen/cycles.gperf"
    {"B9", 4},
    {(char*)0},
#line 13 "be1/gen/cycles.gperf"
    {"03", 8},
#line 74 "be1/gen/cycles.gperf"
    {"40", 4},
    {(char*)0},
#line 194 "be1/gen/cycles.gperf"
    {"B8", 4},
    {(char*)0},
#line 27 "be1/gen/cycles.gperf"
    {"11", 12},
    {(char*)0},
#line 91 "be1/gen/cycles.gperf"
    {"51", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 193 "be1/gen/cycles.gperf"
    {"B7", 4},
    {(char*)0}, {(char*)0},
#line 174 "be1/gen/cycles.gperf"
    {"A4", 4},
    {(char*)0}, {(char*)0},
#line 172 "be1/gen/cycles.gperf"
    {"A2", 4},
    {(char*)0}, {(char*)0},
#line 77 "be1/gen/cycles.gperf"
    {"43", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 16 "be1/gen/cycles.gperf"
    {"06", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 171 "be1/gen/cycles.gperf"
    {"A1", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 80 "be1/gen/cycles.gperf"
    {"46", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 51 "be1/gen/cycles.gperf"
    {"29", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 50 "be1/gen/cycles.gperf"
    {"28", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 21 "be1/gen/cycles.gperf"
    {"0B", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 49 "be1/gen/cycles.gperf"
    {"27", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 35 "be1/gen/cycles.gperf"
    {"19", 8},
    {(char*)0},
#line 99 "be1/gen/cycles.gperf"
    {"59", 4},
    {(char*)0}, {(char*)0},
#line 34 "be1/gen/cycles.gperf"
    {"18", 12},
    {(char*)0},
#line 98 "be1/gen/cycles.gperf"
    {"58", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 85 "be1/gen/cycles.gperf"
    {"4B", 4},
    {(char*)0}, {(char*)0},
#line 33 "be1/gen/cycles.gperf"
    {"17", 4},
    {(char*)0},
#line 97 "be1/gen/cycles.gperf"
    {"57", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 159 "be1/gen/cycles.gperf"
    {"95", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 164 "be1/gen/cycles.gperf"
    {"9A", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 179 "be1/gen/cycles.gperf"
    {"A9", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 178 "be1/gen/cycles.gperf"
    {"A8", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 177 "be1/gen/cycles.gperf"
    {"A7", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 158 "be1/gen/cycles.gperf"
    {"94", 4},
    {(char*)0}, {(char*)0},
#line 156 "be1/gen/cycles.gperf"
    {"92", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 155 "be1/gen/cycles.gperf"
    {"91", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 15 "be1/gen/cycles.gperf"
    {"05", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 20 "be1/gen/cycles.gperf"
    {"0A", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 79 "be1/gen/cycles.gperf"
    {"45", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 84 "be1/gen/cycles.gperf"
    {"4A", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 14 "be1/gen/cycles.gperf"
    {"04", 4},
    {(char*)0}, {(char*)0},
#line 12 "be1/gen/cycles.gperf"
    {"02", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 163 "be1/gen/cycles.gperf"
    {"99", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 162 "be1/gen/cycles.gperf"
    {"98", 4},
    {(char*)0},
#line 11 "be1/gen/cycles.gperf"
    {"01", 12},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 78 "be1/gen/cycles.gperf"
    {"44", 4},
    {(char*)0}, {(char*)0},
#line 76 "be1/gen/cycles.gperf"
    {"42", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 161 "be1/gen/cycles.gperf"
    {"97", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 75 "be1/gen/cycles.gperf"
    {"41", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 19 "be1/gen/cycles.gperf"
    {"09", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 18 "be1/gen/cycles.gperf"
    {"08", 20},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 17 "be1/gen/cycles.gperf"
    {"07", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 83 "be1/gen/cycles.gperf"
    {"49", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 82 "be1/gen/cycles.gperf"
    {"48", 4},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 81 "be1/gen/cycles.gperf"
    {"47", 4}
  };

const struct gb_be1_cycle_sym *
gb_be1_cycles_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = gb_be1_cycles_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (s && *str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
