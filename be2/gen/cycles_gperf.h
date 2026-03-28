/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -I -C -t -L ANSI-C --output-file=be2/gen/cycles_gperf.h be2/gen/cycles.gperf  */
/* Computed positions: -k'2' */

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

#line 8 "be2/gen/cycles.gperf"
struct gen_be2_line_sym { const char *name; int cycles_ref; };
#include <string.h>

#define TOTAL_KEYWORDS 16
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 2
#define MIN_HASH_VALUE 0
#define MAX_HASH_VALUE 30
/* maximum key range = 31, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
/*ARGSUSED*/
static unsigned int
gen_be2_line_cycles_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 11,  6,
       1, 28, 23, 18, 13,  8,  3, 30, 31, 31,
      31, 31, 31, 31, 31, 25, 20, 15, 10,  5,
       0, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
      31, 31, 31, 31, 31, 31
    };
  return asso_values[(unsigned char)str[1]];
}

static const struct gen_be2_line_sym wordlist[] =
  {
#line 25 "be2/gen/cycles.gperf"
    {"LF", 34},
#line 12 "be2/gen/cycles.gperf"
    {"L2", 8},
    {(char*)0},
#line 18 "be2/gen/cycles.gperf"
    {"L8", 8},
    {(char*)0},
#line 24 "be2/gen/cycles.gperf"
    {"LE", 6},
#line 11 "be2/gen/cycles.gperf"
    {"L1", 8},
    {(char*)0},
#line 17 "be2/gen/cycles.gperf"
    {"L7", 4},
    {(char*)0},
#line 23 "be2/gen/cycles.gperf"
    {"LD", 8},
#line 10 "be2/gen/cycles.gperf"
    {"L0", 8},
    {(char*)0},
#line 16 "be2/gen/cycles.gperf"
    {"L6", 10},
    {(char*)0},
#line 22 "be2/gen/cycles.gperf"
    {"LC", 8},
    {(char*)0}, {(char*)0},
#line 15 "be2/gen/cycles.gperf"
    {"L5", 4},
    {(char*)0},
#line 21 "be2/gen/cycles.gperf"
    {"LB", 8},
    {(char*)0}, {(char*)0},
#line 14 "be2/gen/cycles.gperf"
    {"L4", 4},
    {(char*)0},
#line 20 "be2/gen/cycles.gperf"
    {"LA", 34},
    {(char*)0}, {(char*)0},
#line 13 "be2/gen/cycles.gperf"
    {"L3", 8},
    {(char*)0},
#line 19 "be2/gen/cycles.gperf"
    {"L9", 8}
  };

const struct gen_be2_line_sym *
gen_be2_line_cycles_lookup (register const char *str, register size_t len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = gen_be2_line_cycles_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (s && *str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
