/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

#ifndef jsstr_h___
#define jsstr_h___
/*
 * JS string type implementation.
 *
 * A JS string is a counted array of unicode characters.  To support handoff
 * of API client memory, the chars are allocated separately from the length,
 * necessitating a pointer after the count, to form a separately allocated
 * string descriptor.  String descriptors are GC'ed, while their chars are
 * allocated from the malloc heap.
 *
 * When a string is treated as an object (by following it with . or []), the
 * runtime wraps it with a JSObject whose valueOf method returns the unwrapped
 * string descriptor.
 */
#include <ctype.h>
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jshash.h"

JS_BEGIN_EXTERN_C

/*
 * The original GC-thing "string" type, a flat character string owned by its
 * GC-thing descriptor.  The chars member points to a vector having byte size
 * (length + 1) * sizeof(jschar), terminated at index length by a zero jschar.
 * The terminator is purely a backstop, in case the chars pointer flows out to
 * native code that requires \u0000 termination.
 *
 * NB: Always use the JSSTRING_LENGTH and JSSTRING_CHARS accessor macros,
 * unless you guard str->member uses with !JSSTRING_IS_DEPENDENT(str).
 */
struct JSString {
    size_t          length;
    jschar          *chars;
};

/*
 * Overlay structure for a string that depends on another string's characters.
 * Distinguished by the JSSTRFLAG_DEPENDENT bit being set in length.  The base
 * member may point to another dependent string if JSSTRING_CHARS has not been
 * called yet.  The length chars in a dependent string are stored starting at
 * base->chars + start, and are not necessarily zero-terminated.  If start is
 * 0, it is not stored, length is a full size_t (minus the JSSTRFLAG_* bits in
 * the high two positions), and the JSSTRFLAG_PREFIX flag is set.
 */
struct JSDependentString {
    size_t          length;
    JSString        *base;
};

/* Definitions for flags stored in the high order bits of JSString.length. */
#define JSSTRFLAG_BITS              2
#define JSSTRFLAG_SHIFT(flg)        ((size_t)(flg) << JSSTRING_LENGTH_BITS)
#define JSSTRFLAG_MASK              JSSTRFLAG_SHIFT(JS_BITMASK(JSSTRFLAG_BITS))
#define JSSTRFLAG_DEPENDENT         JSSTRFLAG_SHIFT(1)
#define JSSTRFLAG_PREFIX            JSSTRFLAG_SHIFT(2)

/* Universal JSString type inquiry and accessor macros. */
#define JSSTRING_BIT(n)             ((size_t)1 << (n))
#define JSSTRING_BITMASK(n)         (JSSTRING_BIT(n) - 1)
#define JSSTRING_HAS_FLAG(str,flg)  ((str)->length & (flg))
#define JSSTRING_IS_DEPENDENT(str)  JSSTRING_HAS_FLAG(str, JSSTRFLAG_DEPENDENT)
#define JSSTRING_IS_PREFIX(str)     JSSTRING_HAS_FLAG(str, JSSTRFLAG_PREFIX)
#define JSSTRING_CHARS(str)         (JSSTRING_IS_DEPENDENT(str)               \
                                     ? JSSTRDEP_CHARS(str)                    \
                                     : (str)->chars)
#define JSSTRING_LENGTH(str)        (JSSTRING_IS_DEPENDENT(str)               \
                                     ? JSSTRDEP_LENGTH(str)                   \
                                     : (str)->length)
#define JSSTRING_LENGTH_BITS        (sizeof(size_t) * JS_BITS_PER_BYTE        \
                                     - JSSTRFLAG_BITS)
#define JSSTRING_LENGTH_MASK        JSSTRING_BITMASK(JSSTRING_LENGTH_BITS)

/* Specific JSDependentString shift/mask accessor and mutator macros. */
#define JSSTRDEP_START_BITS         (JSSTRING_LENGTH_BITS-JSSTRDEP_LENGTH_BITS)
#define JSSTRDEP_START_SHIFT        JSSTRDEP_LENGTH_BITS
#define JSSTRDEP_START_MASK         JSSTRING_BITMASK(JSSTRDEP_START_BITS)
#define JSSTRDEP_LENGTH_BITS        (JSSTRING_LENGTH_BITS / 2)
#define JSSTRDEP_LENGTH_MASK        JSSTRING_BITMASK(JSSTRDEP_LENGTH_BITS)

#define JSSTRDEP(str)               ((JSDependentString *)(str))
#define JSSTRDEP_START(str)         (JSSTRING_IS_PREFIX(str) ? 0              \
                                     : ((JSSTRDEP(str)->length                \
                                         >> JSSTRDEP_START_SHIFT)             \
                                        & JSSTRDEP_START_MASK))
#define JSSTRDEP_LENGTH(str)        (JSSTRDEP(str)->length                    \
                                     & (JSSTRING_IS_PREFIX(str)               \
                                        ? JSSTRING_LENGTH_MASK                \
                                        : JSSTRDEP_LENGTH_MASK))

#define JSSTRDEP_SET_START_AND_LENGTH(str,off,len)                            \
    (JSSTRDEP(str)->length = JSSTRFLAG_DEPENDENT                              \
                           | ((off) << JSSTRDEP_START_SHIFT)                  \
                           | (len))
#define JSPREFIX_SET_LENGTH(str,len)                                          \
    (JSSTRDEP(str)->length = JSSTRFLAG_DEPENDENT | JSSTRFLAG_PREFIX | (len))

#define JSSTRDEP_BASE(str)          (JSSTRDEP(str)->base)
#define JSSTRDEP_SET_BASE(str,bstr) (JSSTRDEP(str)->base = (bstr))
#define JSPREFIX_BASE(str)          JSSTRDEP_BASE(str)
#define JSPREFIX_SET_BASE(str,bstr) JSSTRDEP_SET_BASE(str,bstr)

#define JSSTRDEP_CHARS(str)                                                   \
    (JSSTRING_IS_DEPENDENT(JSSTRDEP_BASE(str))                                \
     ? js_GetDependentStringChars(str)                                        \
     : JSSTRDEP_BASE(str)->chars + JSSTRDEP_START(str))

extern size_t
js_MinimizeDependentStrings(JSString *str, int level, JSString **basep);

extern jschar *
js_GetDependentStringChars(JSString *str);

extern jschar *
js_GetStringChars(JSString *str);

extern JSString *
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right);

extern const jschar *
js_UndependString(JSContext *cx, JSString *str);

struct JSSubString {
    size_t          length;
    const jschar    *chars;
};

extern jschar      js_empty_ucstr[];
extern JSSubString js_EmptySubString;

/* Unicode character attribute lookup tables. */
extern const uint8 js_X[];
extern const uint8 js_Y[];
extern const uint32 js_A[];

/* Enumerated Unicode general category types. */
typedef enum JSCharType {
    JSCT_UNASSIGNED             = 0,
    JSCT_UPPERCASE_LETTER       = 1,
    JSCT_LOWERCASE_LETTER       = 2,
    JSCT_TITLECASE_LETTER       = 3,
    JSCT_MODIFIER_LETTER        = 4,
    JSCT_OTHER_LETTER           = 5,
    JSCT_NON_SPACING_MARK       = 6,
    JSCT_ENCLOSING_MARK         = 7,
    JSCT_COMBINING_SPACING_MARK = 8,
    JSCT_DECIMAL_DIGIT_NUMBER   = 9,
    JSCT_LETTER_NUMBER          = 10,
    JSCT_OTHER_NUMBER           = 11,
    JSCT_SPACE_SEPARATOR        = 12,
    JSCT_LINE_SEPARATOR         = 13,
    JSCT_PARAGRAPH_SEPARATOR    = 14,
    JSCT_CONTROL                = 15,
    JSCT_FORMAT                 = 16,
    JSCT_PRIVATE_USE            = 18,
    JSCT_SURROGATE              = 19,
    JSCT_DASH_PUNCTUATION       = 20,
    JSCT_START_PUNCTUATION      = 21,
    JSCT_END_PUNCTUATION        = 22,
    JSCT_CONNECTOR_PUNCTUATION  = 23,
    JSCT_OTHER_PUNCTUATION      = 24,
    JSCT_MATH_SYMBOL            = 25,
    JSCT_CURRENCY_SYMBOL        = 26,
    JSCT_MODIFIER_SYMBOL        = 27,
    JSCT_OTHER_SYMBOL           = 28
} JSCharType;

/* Character classifying and mapping macros, based on java.lang.Character. */
#define JS_CCODE(c)     (js_A[js_Y[(js_X[(uint16)(c)>>6]<<6)|((c)&0x3F)]])
#define JS_CTYPE(c)     (JS_CCODE(c) & 0x1F)

#define JS_ISALPHA(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                     \
			   (1 << JSCT_LOWERCASE_LETTER) |                     \
			   (1 << JSCT_TITLECASE_LETTER) |                     \
			   (1 << JSCT_MODIFIER_LETTER) |                      \
			   (1 << JSCT_OTHER_LETTER))                          \
			  >> JS_CTYPE(c)) & 1)

#define JS_ISALNUM(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                     \
			   (1 << JSCT_LOWERCASE_LETTER) |                     \
			   (1 << JSCT_TITLECASE_LETTER) |                     \
			   (1 << JSCT_MODIFIER_LETTER) |                      \
			   (1 << JSCT_OTHER_LETTER) |                         \
			   (1 << JSCT_DECIMAL_DIGIT_NUMBER))                  \
			  >> JS_CTYPE(c)) & 1)

/* A unicode letter, suitable for use in an identifier. */
#define JS_ISUC_LETTER(c)   ((((1 << JSCT_UPPERCASE_LETTER) |                 \
			   (1 << JSCT_LOWERCASE_LETTER) |                     \
			   (1 << JSCT_TITLECASE_LETTER) |                     \
			   (1 << JSCT_MODIFIER_LETTER) |                      \
			   (1 << JSCT_OTHER_LETTER) |                         \
			   (1 << JSCT_LETTER_NUMBER))                         \
			  >> JS_CTYPE(c)) & 1)

/*
* 'IdentifierPart' from ECMA grammar, is Unicode letter or
* combining mark or digit or connector punctuation.
*/
#define JS_ISID_PART(c) ((((1 << JSCT_UPPERCASE_LETTER) |                     \
			   (1 << JSCT_LOWERCASE_LETTER) |                     \
			   (1 << JSCT_TITLECASE_LETTER) |                     \
			   (1 << JSCT_MODIFIER_LETTER) |                      \
			   (1 << JSCT_OTHER_LETTER) |                         \
			   (1 << JSCT_LETTER_NUMBER) |                        \
			   (1 << JSCT_NON_SPACING_MARK) |                     \
			   (1 << JSCT_COMBINING_SPACING_MARK) |               \
			   (1 << JSCT_DECIMAL_DIGIT_NUMBER) |                 \
			   (1 << JSCT_CONNECTOR_PUNCTUATION))                 \
			  >> JS_CTYPE(c)) & 1)

/* Unicode control-format characters, ignored in input */
#define JS_ISFORMAT(c) (((1 << JSCT_FORMAT) >> JS_CTYPE(c)) & 1)

#define JS_ISWORD(c)    (JS_ISALNUM(c) || (c) == '_')

/* XXXbe unify on A/X/Y tbls, avoid ctype.h? */
#define JS_ISIDENT_START(c) (JS_ISUC_LETTER(c) || (c) == '_' || (c) == '$')
#define JS_ISIDENT(c)       (JS_ISID_PART(c) || (c) == '_' || (c) == '$')

#define JS_ISDIGIT(c)   (JS_CTYPE(c) == JSCT_DECIMAL_DIGIT_NUMBER)

/* XXXbe fs, etc. ? */
#define JS_ISSPACE(c)   ((JS_CCODE(c) & 0x00070000) == 0x00040000)
#define JS_ISPRINT(c)   ((c) < 128 && isprint(c))

#define JS_ISUPPER(c)   (JS_CTYPE(c) == JSCT_UPPERCASE_LETTER)
#define JS_ISLOWER(c)   (JS_CTYPE(c) == JSCT_LOWERCASE_LETTER)

#define JS_TOUPPER(c)   ((JS_CCODE(c) & 0x00100000) ? (c) - ((int32)JS_CCODE(c) >> 22) : (c))
#define JS_TOLOWER(c)   ((JS_CCODE(c) & 0x00200000) ? (c) + ((int32)JS_CCODE(c) >> 22) : (c))

#define JS_TOCTRL(c)    ((c) ^ 64)      /* XXX unsafe! requires uppercase c */

/* Shorthands for ASCII (7-bit) decimal and hex conversion. */
#define JS7_ISDEC(c)    ((c) < 128 && isdigit(c))
#define JS7_UNDEC(c)    ((c) - '0')
#define JS7_ISHEX(c)    ((c) < 128 && isxdigit(c))
#define JS7_UNHEX(c)    (uintN)(isdigit(c) ? (c) - '0' : 10 + tolower(c) - 'a')
#define JS7_ISLET(c)    ((c) < 128 && isalpha(c))

/* Initialize truly global state associated with JS strings. */
extern JSBool
js_InitStringGlobals(void);

extern void
js_FreeStringGlobals(void);

/* Initialize per-runtime string state for the first context in the runtime. */
extern JSBool
js_InitRuntimeStringState(JSContext *cx);

extern void
js_FinishRuntimeStringState(JSContext *cx);

/* Initialize the String class, returning its prototype object. */
extern JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj);

extern const char js_escape_str[];
extern const char js_unescape_str[];
extern const char js_uneval_str[];
extern const char js_decodeURI_str[];
extern const char js_encodeURI_str[];
extern const char js_decodeURIComponent_str[];
extern const char js_encodeURIComponent_str[];

/* GC-allocate a string descriptor for the given malloc-allocated chars. */
extern JSString *
js_NewString(JSContext *cx, jschar *chars, size_t length, uintN gcflag);

extern JSString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length, uintN gcflag);

/* Copy a counted string and GC-allocate a descriptor for it. */
extern JSString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n, uintN gcflag);

/* Copy a C string and GC-allocate a descriptor for it. */
extern JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s, uintN gcflag);

/* Free the chars held by str when it is finalized by the GC. */
extern void
js_FinalizeString(JSContext *cx, JSString *str);

extern void
js_FinalizeStringRT(JSRuntime *rt, JSString *str);

/* Wrap a string value in a String object. */
extern JSObject *
js_StringToObject(JSContext *cx, JSString *str);

/*
 * Convert a value to a string, returning null after reporting an error,
 * otherwise returning a new string reference.
 */
extern JSString *
js_ValueToString(JSContext *cx, jsval v);

/*
 * Convert a value to its source expression, returning null after reporting
 * an error, otherwise returning a new string reference.
 */
extern JSString *
js_ValueToSource(JSContext *cx, jsval v);

#ifdef HT_ENUMERATE_NEXT	/* XXX don't require jshash.h */
/*
 * Compute a hash function from str.
 */
extern JSHashNumber
js_HashString(JSString *str);
#endif

/*
 * Return less than, equal to, or greater than zero depending on whether
 * str1 is less than, equal to, or greater than str2.
 */
extern intN
js_CompareStrings(JSString *str1, JSString *str2);

/*
 * Boyer-Moore-Horspool superlinear search for pat:patlen in text:textlen.
 * The patlen argument must be positive and no greater than BMH_PATLEN_MAX.
 * The start argument tells where in text to begin the search.
 *
 * Return the index of pat in text, or -1 if not found.
 */
#define BMH_CHARSET_SIZE 256    /* ISO-Latin-1 */
#define BMH_PATLEN_MAX   255    /* skip table element is uint8 */

#define BMH_BAD_PATTERN  (-2)   /* return value if pat is not ISO-Latin-1 */

extern jsint
js_BoyerMooreHorspool(const jschar *text, jsint textlen,
                      const jschar *pat, jsint patlen,
                      jsint start);

extern size_t
js_strlen(const jschar *s);

extern jschar *
js_strchr(const jschar *s, jschar c);

extern jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit);

#define js_strncpy(t, s, n)     memcpy((t), (s), (n) * sizeof(jschar))

/*
 * Return s advanced past any Unicode white space characters.
 */
extern const jschar *
js_SkipWhiteSpace(const jschar *s);

/*
 * Inflate bytes to JS chars and vice versa.  Report out of memory via cx
 * and return null on error, otherwise return the jschar or byte vector that
 * was JS_malloc'ed.
 */
extern jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t length);

extern char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length);

/*
 * Inflate bytes to JS chars into a buffer.
 * 'chars' must be large enough for 'length'+1 jschars.
 */
extern void
js_InflateStringToBuffer(jschar *chars, const char *bytes, size_t length);

/*
 * Associate bytes with str in the deflated string cache, returning true on
 * successful association, false on out of memory.
 */
extern JSBool
js_SetStringBytes(JSString *str, char *bytes, size_t length);

/*
 * Find or create a deflated string cache entry for str that contains its
 * characters chopped from Unicode code points into bytes.
 */
extern char *
js_GetStringBytes(JSString *str);

JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval);

JS_END_EXTERN_C

#endif /* jsstr_h___ */
