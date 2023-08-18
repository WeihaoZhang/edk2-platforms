#include "FileRW.h"
#include "SPECLib/Common.h"
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
// #include <stdio.h>
// #include <atomic.h>
// #include <ctype.h>
int _tolower(int c);
int isdigit(int c);
#define KSTRTOX_OVERFLOW	(1U << 31)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
typedef UINT64 u64;
typedef UINT64 uint64_t;
typedef UINT32 uint32_t;
typedef UINT32 u32;
typedef UINT8 u8;
typedef INT16 s16;



#define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = do_div(dividend, divisor);
	return dividend;
}

static inline u64 div_u64(u64 dividend, u32 divisor)
{
	u32 remainder;
	return div_u64_rem(dividend, divisor, &remainder);
}

unsigned int _parse_integer_limit(const char *s, unsigned int base, unsigned long long *p,
				  size_t max_chars)
{
	unsigned long long res;
	unsigned int rv;

	res = 0;
	rv = 0;
	while (max_chars--) {
		unsigned int c = *s;
		unsigned int lc = c | 0x20; /* don't tolower() this line */
		unsigned int val;

		if ('0' <= c && c <= '9')
			val = c - '0';
		else if ('a' <= lc && lc <= 'f')
			val = lc - 'a' + 10;
		else
			break;

		if (val >= base)
			break;
		/*
		 * Check for overflow only if we are within range of
		 * it in the max base we support (16)
		 */
		if (unlikely(res & (~0ull << 60))) {
			if (res > div_u64(ULLONG_MAX - val, base))
				rv |= KSTRTOX_OVERFLOW;
		}
		res = res * base + val;
		rv++;
		s++;
	}
	*p = res;
	return rv;
}

int isxdigit(int c)
{
	return isdigit(c) || ((unsigned)c | 32) - 'a' < 6;
}

const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (_tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && _tolower(s[1]) == 'x')
		s += 2;
	return s;
}

static unsigned long long simple_strntoull(const char *startp, size_t max_chars,
					   char **endp, unsigned int base)
{
	const char *cp;
	unsigned long long result = 0ULL;
	size_t prefix_chars;
	unsigned int rv;

	cp = _parse_integer_fixup_radix(startp, &base);
	prefix_chars = cp - startp;
	if (prefix_chars < max_chars) {
		rv = _parse_integer_limit(cp, base, &result, max_chars - prefix_chars);
		/* FIXME */
		cp += (rv & ~KSTRTOX_OVERFLOW);
	} else {
		/* Field too short for prefix + digit, skip over without converting */
		cp = startp + max_chars;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

/**
 * simple_strtoull - convert a string to an unsigned long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 *
 * This function has caveats. Please use kstrtoull instead.
 */
unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
	return simple_strntoull(cp, INT_MAX, endp, base);
}

static long long simple_strntoll(const char *cp, size_t max_chars, char **endp,
				 unsigned int base)
{
	/*
	 * simple_strntoull() safely handles receiving max_chars==0 in the
	 * case cp[0] == '-' && max_chars == 1.
	 * If max_chars == 0 we can drop through and pass it to simple_strntoull()
	 * and the content of *cp is irrelevant.
	 */
	if (*cp == '-' && max_chars > 0)
		return -simple_strntoull(cp + 1, max_chars - 1, endp, base);

	return simple_strntoull(cp, max_chars, endp, base);
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	return simple_strtoull(cp, endp, base);
}

long long simple_strtoll(const char *cp, char **endp, unsigned int base)
{
	return simple_strntoll(cp, INT_MAX, endp, base);
}

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))  //向上取整


#define BITS_PER_BYTE		8

#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))


#define DECLARE_BITMAP(name,bits)  \
	unsigned long name[BITS_TO_LONGS(bits)]

/* Determines if a particular character is in upper case */
int
isupper (
  int  c
  )
{
  //
  // <uppercase letter> := [A-Z]
  //
  return (('A' <= (c)) && ((c) <= 'Z'));
}


int tolower(int c)
{
	if(isupper(c))
	{
		return c | 32;
	}

	return c;
}

int _tolower(int c)
{
  if (c>='A' && c<='Z') return c+32;
  else return c;
}


int isspace(int c)
{
	return c == ' ' || (unsigned)c - '\t' < 5;
}
/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

int isdigit(int c)
{
	return (unsigned)c - '0' < 10;
}

static 
int skip_atoi(const char **s)
{
	int i = 0;

	do {
		i = i*10 + *((*s)++) - '0';
	} while (isdigit(**s));

	return i;
}


/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
int vsscanf(const char *buf, const char *fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int num = 0;
	UINT8 qualifier;
	unsigned int base;
	union {
		long long s;
		unsigned long long u;
	} val;
	INT16 field_width;
	bool is_sign;

	while (*fmt) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			fmt = skip_spaces(++fmt);
			str = skip_spaces(str);
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;

		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			if (!*str)
				break;
			while (!isspace(*fmt) && *fmt != '%' && *fmt) {
				/* '%*[' not yet supported, invalid format */
				if (*fmt == '[')
					return num;
				fmt++;
			}
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt)) {
			field_width = skip_atoi(&fmt);
			if (field_width <= 0)
				break;
		}

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || _tolower(*fmt) == 'l' ||
		    *fmt == 'z') {
			qualifier = *fmt++;
			if (unlikely(qualifier == *fmt)) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}

		if (!*fmt)
			break;

		if (*fmt == 'n') {
			/* return number of characters read so far */
			*va_arg(args, int *) = str - buf;
			++fmt;
			continue;
		}

		if (!*str)
			break;

		base = 10;
		is_sign = false;

		switch (*fmt++) {
		case 'c':
		{
			char *s = (char *)va_arg(args, char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *)va_arg(args, char *);
			if (field_width == -1)
				field_width = SHRT_MAX;
			/* first, skip leading white space in buffer */
			str = skip_spaces(str);

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--)
				*s++ = *str++;
			*s = '\0';
			num++;
		}
		continue;
		/*
		 * Warning: This implementation of the '[' conversion specifier
		 * deviates from its glibc counterpart in the following ways:
		 * (1) It does NOT support ranges i.e. '-' is NOT a special
		 *     character
		 * (2) It cannot match the closing bracket ']' itself
		 * (3) A field width is required
		 * (4) '%*[' (discard matching input) is currently not supported
		 *
		 * Example usage:
		 * ret = sscanf("00:0a:95","%2[^:]:%2[^:]:%2[^:]",
		 *		buf1, buf2, buf3);
		 * if (ret < 3)
		 *    // etc..
		 */
		// case '[':
		// {
		// 	char *s = (char *)va_arg(args, char *);
		// 	DECLARE_BITMAP(set, 256) = {0};
		// 	unsigned int len = 0;
		// 	bool negate = (*fmt == '^');

		// 	/* field width is required */
		// 	if (field_width == -1)
		// 		return num;

		// 	if (negate)
		// 		++fmt;

		// 	for ( ; *fmt && *fmt != ']'; ++fmt, ++len)
		// 		set_bit((UINT8)*fmt, set);

		// 	/* no ']' or no character set found */
		// 	if (!*fmt || !len)
		// 		return num;
		// 	++fmt;

		// 	if (negate) {
		// 		bitmap_complement(set, set, 256);
		// 		/* exclude null '\0' byte */
		// 		clear_bit(0, set);
		// 	}

		// 	/* match must be non-empty */
		// 	if (!test_bit((UINT8)*str, set))
		// 		return num;

		// 	while (test_bit((UINT8)*str, set) && field_width--)
		// 		*s++ = *str++;
		// 	*s = '\0';
		// 	++num;
		// }
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
			base = 0;
			/* fall through */
		case 'd':
			is_sign = true;
			/* fall through */
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%')
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		str = skip_spaces(str);

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
		    || (base == 16 && !isxdigit(digit))
		    || (base == 10 && !isdigit(digit))
		    || (base == 8 && (!isdigit(digit) || digit > '7'))
		    || (base == 0 && !isdigit(digit)))
			break;

		if (is_sign)
			val.s = simple_strntoll(str,
						field_width >= 0 ? field_width : INT_MAX,
						&next, base);
		else
			val.u = simple_strntoull(str,
						 field_width >= 0 ? field_width : INT_MAX,
						 &next, base);

		switch (qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign)
				*va_arg(args, signed char *) = val.s;
			else
				*va_arg(args, unsigned char *) = val.u;
			break;
		case 'h':
			if (is_sign)
				*va_arg(args, short *) = val.s;
			else
				*va_arg(args, unsigned short *) = val.u;
			break;
		case 'l':
			if (is_sign)
				*va_arg(args, long *) = val.s;
			else
				*va_arg(args, unsigned long *) = val.u;
			break;
		case 'L':
			if (is_sign)
				*va_arg(args, long long *) = val.s;
			else
				*va_arg(args, unsigned long long *) = val.u;
			break;
		case 'z':
			*va_arg(args, size_t *) = val.u;
			break;
		default:
			if (is_sign)
				*va_arg(args, int *) = val.s;
			else
				*va_arg(args, unsigned int *) = val.u;
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}

	return num;
}

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int sscanf(const char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsscanf(buf, fmt, args);
	va_end(args);

	return i;
}