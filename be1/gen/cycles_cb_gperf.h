/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -I -C -t -L ANSI-C --output-file=be1/gen/cycles_cb_gperf.h be1/gen/cycles_cb.gperf  */
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

#line 8 "be1/gen/cycles_cb.gperf"
struct gb_be1_cb_cycle_sym { const char *name; int tdots; };
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
gb_be1_cb_cycles_hash (register const char *str, register size_t len)
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

static const struct gb_be1_cb_cycle_sym wordlist[] =
  {
    {(char*)0},
#line 73 "be1/gen/cycles_cb.gperf"
    {"3F", 8},
    {(char*)0},
#line 153 "be1/gen/cycles_cb.gperf"
    {"8F", 8},
    {(char*)0}, {(char*)0},
#line 72 "be1/gen/cycles_cb.gperf"
    {"3E", 16},
    {(char*)0},
#line 152 "be1/gen/cycles_cb.gperf"
    {"8E", 16},
    {(char*)0}, {(char*)0},
#line 71 "be1/gen/cycles_cb.gperf"
    {"3D", 8},
    {(char*)0},
#line 151 "be1/gen/cycles_cb.gperf"
    {"8D", 8},
    {(char*)0}, {(char*)0},
#line 70 "be1/gen/cycles_cb.gperf"
    {"3C", 8},
    {(char*)0},
#line 150 "be1/gen/cycles_cb.gperf"
    {"8C", 8},
    {(char*)0},
#line 265 "be1/gen/cycles_cb.gperf"
    {"FF", 8},
#line 58 "be1/gen/cycles_cb.gperf"
    {"30", 8},
    {(char*)0},
#line 138 "be1/gen/cycles_cb.gperf"
    {"80", 8},
    {(char*)0},
#line 264 "be1/gen/cycles_cb.gperf"
    {"FE", 16},
    {(char*)0}, {(char*)0},
#line 137 "be1/gen/cycles_cb.gperf"
    {"7F", 8},
    {(char*)0},
#line 263 "be1/gen/cycles_cb.gperf"
    {"FD", 8},
    {(char*)0}, {(char*)0},
#line 136 "be1/gen/cycles_cb.gperf"
    {"7E", 16},
    {(char*)0},
#line 262 "be1/gen/cycles_cb.gperf"
    {"FC", 8},
    {(char*)0}, {(char*)0},
#line 135 "be1/gen/cycles_cb.gperf"
    {"7D", 8},
    {(char*)0},
#line 250 "be1/gen/cycles_cb.gperf"
    {"F0", 8},
    {(char*)0}, {(char*)0},
#line 134 "be1/gen/cycles_cb.gperf"
    {"7C", 8},
    {(char*)0},
#line 249 "be1/gen/cycles_cb.gperf"
    {"EF", 8},
    {(char*)0},
#line 61 "be1/gen/cycles_cb.gperf"
    {"33", 8},
#line 122 "be1/gen/cycles_cb.gperf"
    {"70", 8},
#line 141 "be1/gen/cycles_cb.gperf"
    {"83", 8},
#line 248 "be1/gen/cycles_cb.gperf"
    {"EE", 16},
    {(char*)0}, {(char*)0},
#line 121 "be1/gen/cycles_cb.gperf"
    {"6F", 8},
    {(char*)0},
#line 247 "be1/gen/cycles_cb.gperf"
    {"ED", 8},
    {(char*)0}, {(char*)0},
#line 120 "be1/gen/cycles_cb.gperf"
    {"6E", 16},
    {(char*)0},
#line 246 "be1/gen/cycles_cb.gperf"
    {"EC", 8},
    {(char*)0}, {(char*)0},
#line 119 "be1/gen/cycles_cb.gperf"
    {"6D", 8},
    {(char*)0},
#line 234 "be1/gen/cycles_cb.gperf"
    {"E0", 8},
#line 253 "be1/gen/cycles_cb.gperf"
    {"F3", 8},
    {(char*)0},
#line 118 "be1/gen/cycles_cb.gperf"
    {"6C", 8},
    {(char*)0},
#line 233 "be1/gen/cycles_cb.gperf"
    {"DF", 8},
    {(char*)0}, {(char*)0},
#line 106 "be1/gen/cycles_cb.gperf"
    {"60", 8},
#line 125 "be1/gen/cycles_cb.gperf"
    {"73", 8},
#line 232 "be1/gen/cycles_cb.gperf"
    {"DE", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 64 "be1/gen/cycles_cb.gperf"
    {"36", 16},
#line 231 "be1/gen/cycles_cb.gperf"
    {"DD", 8},
#line 144 "be1/gen/cycles_cb.gperf"
    {"86", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 230 "be1/gen/cycles_cb.gperf"
    {"DC", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 218 "be1/gen/cycles_cb.gperf"
    {"D0", 8},
#line 237 "be1/gen/cycles_cb.gperf"
    {"E3", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 217 "be1/gen/cycles_cb.gperf"
    {"CF", 8},
    {(char*)0}, {(char*)0},
#line 256 "be1/gen/cycles_cb.gperf"
    {"F6", 16},
#line 109 "be1/gen/cycles_cb.gperf"
    {"63", 8},
#line 216 "be1/gen/cycles_cb.gperf"
    {"CE", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 215 "be1/gen/cycles_cb.gperf"
    {"CD", 8},
#line 128 "be1/gen/cycles_cb.gperf"
    {"76", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 214 "be1/gen/cycles_cb.gperf"
    {"CC", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 202 "be1/gen/cycles_cb.gperf"
    {"C0", 8},
#line 221 "be1/gen/cycles_cb.gperf"
    {"D3", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 201 "be1/gen/cycles_cb.gperf"
    {"BF", 8},
    {(char*)0}, {(char*)0},
#line 240 "be1/gen/cycles_cb.gperf"
    {"E6", 16},
    {(char*)0},
#line 200 "be1/gen/cycles_cb.gperf"
    {"BE", 16},
#line 69 "be1/gen/cycles_cb.gperf"
    {"3B", 8},
    {(char*)0},
#line 149 "be1/gen/cycles_cb.gperf"
    {"8B", 8},
    {(char*)0},
#line 199 "be1/gen/cycles_cb.gperf"
    {"BD", 8},
#line 112 "be1/gen/cycles_cb.gperf"
    {"66", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 198 "be1/gen/cycles_cb.gperf"
    {"BC", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 186 "be1/gen/cycles_cb.gperf"
    {"B0", 8},
#line 205 "be1/gen/cycles_cb.gperf"
    {"C3", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 261 "be1/gen/cycles_cb.gperf"
    {"FB", 8},
    {(char*)0}, {(char*)0},
#line 224 "be1/gen/cycles_cb.gperf"
    {"D6", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 133 "be1/gen/cycles_cb.gperf"
    {"7B", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 189 "be1/gen/cycles_cb.gperf"
    {"B3", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 245 "be1/gen/cycles_cb.gperf"
    {"EB", 8},
    {(char*)0}, {(char*)0},
#line 208 "be1/gen/cycles_cb.gperf"
    {"C6", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 117 "be1/gen/cycles_cb.gperf"
    {"6B", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 57 "be1/gen/cycles_cb.gperf"
    {"2F", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 56 "be1/gen/cycles_cb.gperf"
    {"2E", 16},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 229 "be1/gen/cycles_cb.gperf"
    {"DB", 8},
#line 55 "be1/gen/cycles_cb.gperf"
    {"2D", 8},
    {(char*)0},
#line 192 "be1/gen/cycles_cb.gperf"
    {"B6", 16},
    {(char*)0}, {(char*)0},
#line 54 "be1/gen/cycles_cb.gperf"
    {"2C", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 42 "be1/gen/cycles_cb.gperf"
    {"20", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 41 "be1/gen/cycles_cb.gperf"
    {"1F", 8},
    {(char*)0},
#line 105 "be1/gen/cycles_cb.gperf"
    {"5F", 8},
    {(char*)0}, {(char*)0},
#line 40 "be1/gen/cycles_cb.gperf"
    {"1E", 16},
    {(char*)0},
#line 104 "be1/gen/cycles_cb.gperf"
    {"5E", 16},
    {(char*)0},
#line 213 "be1/gen/cycles_cb.gperf"
    {"CB", 8},
#line 39 "be1/gen/cycles_cb.gperf"
    {"1D", 8},
    {(char*)0},
#line 103 "be1/gen/cycles_cb.gperf"
    {"5D", 8},
    {(char*)0}, {(char*)0},
#line 38 "be1/gen/cycles_cb.gperf"
    {"1C", 8},
    {(char*)0},
#line 102 "be1/gen/cycles_cb.gperf"
    {"5C", 8},
    {(char*)0}, {(char*)0},
#line 26 "be1/gen/cycles_cb.gperf"
    {"10", 8},
#line 45 "be1/gen/cycles_cb.gperf"
    {"23", 8},
#line 90 "be1/gen/cycles_cb.gperf"
    {"50", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 197 "be1/gen/cycles_cb.gperf"
    {"BB", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 185 "be1/gen/cycles_cb.gperf"
    {"AF", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 63 "be1/gen/cycles_cb.gperf"
    {"35", 8},
#line 184 "be1/gen/cycles_cb.gperf"
    {"AE", 16},
#line 143 "be1/gen/cycles_cb.gperf"
    {"85", 8},
#line 29 "be1/gen/cycles_cb.gperf"
    {"13", 8},
    {(char*)0},
#line 93 "be1/gen/cycles_cb.gperf"
    {"53", 8},
#line 183 "be1/gen/cycles_cb.gperf"
    {"AD", 8},
#line 68 "be1/gen/cycles_cb.gperf"
    {"3A", 8},
    {(char*)0},
#line 148 "be1/gen/cycles_cb.gperf"
    {"8A", 8},
#line 48 "be1/gen/cycles_cb.gperf"
    {"26", 16},
#line 182 "be1/gen/cycles_cb.gperf"
    {"AC", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 170 "be1/gen/cycles_cb.gperf"
    {"A0", 8},
    {(char*)0}, {(char*)0},
#line 255 "be1/gen/cycles_cb.gperf"
    {"F5", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 260 "be1/gen/cycles_cb.gperf"
    {"FA", 8},
#line 127 "be1/gen/cycles_cb.gperf"
    {"75", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 132 "be1/gen/cycles_cb.gperf"
    {"7A", 8},
#line 32 "be1/gen/cycles_cb.gperf"
    {"16", 16},
    {(char*)0},
#line 96 "be1/gen/cycles_cb.gperf"
    {"56", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 173 "be1/gen/cycles_cb.gperf"
    {"A3", 8},
    {(char*)0},
#line 239 "be1/gen/cycles_cb.gperf"
    {"E5", 8},
#line 62 "be1/gen/cycles_cb.gperf"
    {"34", 8},
    {(char*)0},
#line 142 "be1/gen/cycles_cb.gperf"
    {"84", 8},
#line 60 "be1/gen/cycles_cb.gperf"
    {"32", 8},
    {(char*)0},
#line 140 "be1/gen/cycles_cb.gperf"
    {"82", 8},
#line 244 "be1/gen/cycles_cb.gperf"
    {"EA", 8},
#line 111 "be1/gen/cycles_cb.gperf"
    {"65", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 53 "be1/gen/cycles_cb.gperf"
    {"2B", 8},
    {(char*)0},
#line 116 "be1/gen/cycles_cb.gperf"
    {"6A", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 254 "be1/gen/cycles_cb.gperf"
    {"F4", 8},
    {(char*)0}, {(char*)0},
#line 252 "be1/gen/cycles_cb.gperf"
    {"F2", 8},
#line 59 "be1/gen/cycles_cb.gperf"
    {"31", 8},
#line 223 "be1/gen/cycles_cb.gperf"
    {"D5", 8},
#line 139 "be1/gen/cycles_cb.gperf"
    {"81", 8},
    {(char*)0},
#line 126 "be1/gen/cycles_cb.gperf"
    {"74", 8},
    {(char*)0},
#line 176 "be1/gen/cycles_cb.gperf"
    {"A6", 16},
#line 124 "be1/gen/cycles_cb.gperf"
    {"72", 8},
#line 228 "be1/gen/cycles_cb.gperf"
    {"DA", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 37 "be1/gen/cycles_cb.gperf"
    {"1B", 8},
    {(char*)0},
#line 101 "be1/gen/cycles_cb.gperf"
    {"5B", 8},
    {(char*)0}, {(char*)0},
#line 251 "be1/gen/cycles_cb.gperf"
    {"F1", 8},
    {(char*)0},
#line 238 "be1/gen/cycles_cb.gperf"
    {"E4", 8},
    {(char*)0}, {(char*)0},
#line 236 "be1/gen/cycles_cb.gperf"
    {"E2", 8},
    {(char*)0},
#line 207 "be1/gen/cycles_cb.gperf"
    {"C5", 8},
#line 123 "be1/gen/cycles_cb.gperf"
    {"71", 8},
    {(char*)0},
#line 110 "be1/gen/cycles_cb.gperf"
    {"64", 8},
    {(char*)0}, {(char*)0},
#line 108 "be1/gen/cycles_cb.gperf"
    {"62", 8},
#line 212 "be1/gen/cycles_cb.gperf"
    {"CA", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 235 "be1/gen/cycles_cb.gperf"
    {"E1", 8},
    {(char*)0},
#line 222 "be1/gen/cycles_cb.gperf"
    {"D4", 8},
    {(char*)0}, {(char*)0},
#line 220 "be1/gen/cycles_cb.gperf"
    {"D2", 8},
    {(char*)0},
#line 191 "be1/gen/cycles_cb.gperf"
    {"B5", 8},
#line 107 "be1/gen/cycles_cb.gperf"
    {"61", 8},
#line 181 "be1/gen/cycles_cb.gperf"
    {"AB", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 196 "be1/gen/cycles_cb.gperf"
    {"BA", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 169 "be1/gen/cycles_cb.gperf"
    {"9F", 8},
#line 219 "be1/gen/cycles_cb.gperf"
    {"D1", 8},
    {(char*)0},
#line 206 "be1/gen/cycles_cb.gperf"
    {"C4", 8},
    {(char*)0},
#line 168 "be1/gen/cycles_cb.gperf"
    {"9E", 16},
#line 204 "be1/gen/cycles_cb.gperf"
    {"C2", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 167 "be1/gen/cycles_cb.gperf"
    {"9D", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 166 "be1/gen/cycles_cb.gperf"
    {"9C", 8},
#line 67 "be1/gen/cycles_cb.gperf"
    {"39", 8},
    {(char*)0},
#line 147 "be1/gen/cycles_cb.gperf"
    {"89", 8},
    {(char*)0},
#line 154 "be1/gen/cycles_cb.gperf"
    {"90", 8},
#line 66 "be1/gen/cycles_cb.gperf"
    {"38", 8},
    {(char*)0},
#line 146 "be1/gen/cycles_cb.gperf"
    {"88", 8},
    {(char*)0}, {(char*)0},
#line 203 "be1/gen/cycles_cb.gperf"
    {"C1", 8},
    {(char*)0},
#line 190 "be1/gen/cycles_cb.gperf"
    {"B4", 8},
    {(char*)0}, {(char*)0},
#line 188 "be1/gen/cycles_cb.gperf"
    {"B2", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 259 "be1/gen/cycles_cb.gperf"
    {"F9", 8},
#line 65 "be1/gen/cycles_cb.gperf"
    {"37", 8},
    {(char*)0},
#line 145 "be1/gen/cycles_cb.gperf"
    {"87", 8},
    {(char*)0},
#line 258 "be1/gen/cycles_cb.gperf"
    {"F8", 8},
    {(char*)0}, {(char*)0},
#line 131 "be1/gen/cycles_cb.gperf"
    {"79", 8},
    {(char*)0}, {(char*)0},
#line 157 "be1/gen/cycles_cb.gperf"
    {"93", 8},
    {(char*)0},
#line 130 "be1/gen/cycles_cb.gperf"
    {"78", 8},
#line 47 "be1/gen/cycles_cb.gperf"
    {"25", 8},
    {(char*)0},
#line 187 "be1/gen/cycles_cb.gperf"
    {"B1", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 257 "be1/gen/cycles_cb.gperf"
    {"F7", 8},
#line 52 "be1/gen/cycles_cb.gperf"
    {"2A", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 243 "be1/gen/cycles_cb.gperf"
    {"E9", 8},
    {(char*)0}, {(char*)0},
#line 129 "be1/gen/cycles_cb.gperf"
    {"77", 8},
    {(char*)0},
#line 242 "be1/gen/cycles_cb.gperf"
    {"E8", 8},
    {(char*)0}, {(char*)0},
#line 115 "be1/gen/cycles_cb.gperf"
    {"69", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 114 "be1/gen/cycles_cb.gperf"
    {"68", 8},
#line 31 "be1/gen/cycles_cb.gperf"
    {"15", 8},
    {(char*)0},
#line 95 "be1/gen/cycles_cb.gperf"
    {"55", 8},
    {(char*)0},
#line 160 "be1/gen/cycles_cb.gperf"
    {"96", 16},
    {(char*)0},
#line 241 "be1/gen/cycles_cb.gperf"
    {"E7", 8},
#line 36 "be1/gen/cycles_cb.gperf"
    {"1A", 8},
    {(char*)0},
#line 100 "be1/gen/cycles_cb.gperf"
    {"5A", 8},
    {(char*)0},
#line 227 "be1/gen/cycles_cb.gperf"
    {"D9", 8},
    {(char*)0}, {(char*)0},
#line 113 "be1/gen/cycles_cb.gperf"
    {"67", 8},
    {(char*)0},
#line 226 "be1/gen/cycles_cb.gperf"
    {"D8", 8},
#line 25 "be1/gen/cycles_cb.gperf"
    {"0F", 8},
    {(char*)0}, {(char*)0},
#line 46 "be1/gen/cycles_cb.gperf"
    {"24", 8},
    {(char*)0},
#line 24 "be1/gen/cycles_cb.gperf"
    {"0E", 16},
#line 44 "be1/gen/cycles_cb.gperf"
    {"22", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 23 "be1/gen/cycles_cb.gperf"
    {"0D", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 225 "be1/gen/cycles_cb.gperf"
    {"D7", 8},
#line 22 "be1/gen/cycles_cb.gperf"
    {"0C", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 211 "be1/gen/cycles_cb.gperf"
    {"C9", 8},
#line 10 "be1/gen/cycles_cb.gperf"
    {"00", 8},
    {(char*)0},
#line 175 "be1/gen/cycles_cb.gperf"
    {"A5", 8},
    {(char*)0},
#line 210 "be1/gen/cycles_cb.gperf"
    {"C8", 8},
    {(char*)0},
#line 43 "be1/gen/cycles_cb.gperf"
    {"21", 8},
#line 89 "be1/gen/cycles_cb.gperf"
    {"4F", 8},
#line 30 "be1/gen/cycles_cb.gperf"
    {"14", 8},
#line 180 "be1/gen/cycles_cb.gperf"
    {"AA", 8},
#line 94 "be1/gen/cycles_cb.gperf"
    {"54", 8},
#line 28 "be1/gen/cycles_cb.gperf"
    {"12", 8},
#line 88 "be1/gen/cycles_cb.gperf"
    {"4E", 16},
#line 92 "be1/gen/cycles_cb.gperf"
    {"52", 8},
#line 165 "be1/gen/cycles_cb.gperf"
    {"9B", 8},
    {(char*)0}, {(char*)0},
#line 87 "be1/gen/cycles_cb.gperf"
    {"4D", 8},
    {(char*)0},
#line 209 "be1/gen/cycles_cb.gperf"
    {"C7", 8},
    {(char*)0}, {(char*)0},
#line 86 "be1/gen/cycles_cb.gperf"
    {"4C", 8},
    {(char*)0},
#line 195 "be1/gen/cycles_cb.gperf"
    {"B9", 8},
    {(char*)0},
#line 13 "be1/gen/cycles_cb.gperf"
    {"03", 8},
#line 74 "be1/gen/cycles_cb.gperf"
    {"40", 8},
    {(char*)0},
#line 194 "be1/gen/cycles_cb.gperf"
    {"B8", 8},
    {(char*)0},
#line 27 "be1/gen/cycles_cb.gperf"
    {"11", 8},
    {(char*)0},
#line 91 "be1/gen/cycles_cb.gperf"
    {"51", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 193 "be1/gen/cycles_cb.gperf"
    {"B7", 8},
    {(char*)0}, {(char*)0},
#line 174 "be1/gen/cycles_cb.gperf"
    {"A4", 8},
    {(char*)0}, {(char*)0},
#line 172 "be1/gen/cycles_cb.gperf"
    {"A2", 8},
    {(char*)0}, {(char*)0},
#line 77 "be1/gen/cycles_cb.gperf"
    {"43", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 16 "be1/gen/cycles_cb.gperf"
    {"06", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 171 "be1/gen/cycles_cb.gperf"
    {"A1", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 80 "be1/gen/cycles_cb.gperf"
    {"46", 16},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 51 "be1/gen/cycles_cb.gperf"
    {"29", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 50 "be1/gen/cycles_cb.gperf"
    {"28", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 21 "be1/gen/cycles_cb.gperf"
    {"0B", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 49 "be1/gen/cycles_cb.gperf"
    {"27", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 35 "be1/gen/cycles_cb.gperf"
    {"19", 8},
    {(char*)0},
#line 99 "be1/gen/cycles_cb.gperf"
    {"59", 8},
    {(char*)0}, {(char*)0},
#line 34 "be1/gen/cycles_cb.gperf"
    {"18", 8},
    {(char*)0},
#line 98 "be1/gen/cycles_cb.gperf"
    {"58", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 85 "be1/gen/cycles_cb.gperf"
    {"4B", 8},
    {(char*)0}, {(char*)0},
#line 33 "be1/gen/cycles_cb.gperf"
    {"17", 8},
    {(char*)0},
#line 97 "be1/gen/cycles_cb.gperf"
    {"57", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 159 "be1/gen/cycles_cb.gperf"
    {"95", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 164 "be1/gen/cycles_cb.gperf"
    {"9A", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 179 "be1/gen/cycles_cb.gperf"
    {"A9", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 178 "be1/gen/cycles_cb.gperf"
    {"A8", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 177 "be1/gen/cycles_cb.gperf"
    {"A7", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 158 "be1/gen/cycles_cb.gperf"
    {"94", 8},
    {(char*)0}, {(char*)0},
#line 156 "be1/gen/cycles_cb.gperf"
    {"92", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 155 "be1/gen/cycles_cb.gperf"
    {"91", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 15 "be1/gen/cycles_cb.gperf"
    {"05", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 20 "be1/gen/cycles_cb.gperf"
    {"0A", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 79 "be1/gen/cycles_cb.gperf"
    {"45", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 84 "be1/gen/cycles_cb.gperf"
    {"4A", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 14 "be1/gen/cycles_cb.gperf"
    {"04", 8},
    {(char*)0}, {(char*)0},
#line 12 "be1/gen/cycles_cb.gperf"
    {"02", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 163 "be1/gen/cycles_cb.gperf"
    {"99", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 162 "be1/gen/cycles_cb.gperf"
    {"98", 8},
    {(char*)0},
#line 11 "be1/gen/cycles_cb.gperf"
    {"01", 8},
    {(char*)0}, {(char*)0}, {(char*)0},
#line 78 "be1/gen/cycles_cb.gperf"
    {"44", 8},
    {(char*)0}, {(char*)0},
#line 76 "be1/gen/cycles_cb.gperf"
    {"42", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 161 "be1/gen/cycles_cb.gperf"
    {"97", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0},
#line 75 "be1/gen/cycles_cb.gperf"
    {"41", 8},
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
#line 19 "be1/gen/cycles_cb.gperf"
    {"09", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 18 "be1/gen/cycles_cb.gperf"
    {"08", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 17 "be1/gen/cycles_cb.gperf"
    {"07", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 83 "be1/gen/cycles_cb.gperf"
    {"49", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
#line 82 "be1/gen/cycles_cb.gperf"
    {"48", 8},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0}, {(char*)0}, {(char*)0},
    {(char*)0}, {(char*)0},
#line 81 "be1/gen/cycles_cb.gperf"
    {"47", 8}
  };

const struct gb_be1_cb_cycle_sym *
gb_be1_cb_cycles_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = gb_be1_cb_cycles_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (s && *str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
