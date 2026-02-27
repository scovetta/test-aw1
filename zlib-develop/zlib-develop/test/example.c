/* example.c -- usage example of the zlib compression library
 * Copyright (C) 1995-2026 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "zlib.h"
#include <stdio.h>

#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#endif

#if defined(VMS)
#  define TESTFILE "foo-gz"
#elif defined(__riscos) && !defined(__TARGET_UNIXLIB__)
#  define TESTFILE "foo/gz"
#else
#  define TESTFILE "foo.gz"
#endif

#define CHECK_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

static z_const char hello[] = "hello, hello!";
/* "hello world" would be more standard, but the repeated "hello"
 * stresses the compression code better, sorry...
 */

static const char dictionary[] = "hello";
static uLong dictId;    /* Adler32 value of the dictionary */

#ifdef Z_SOLO

static void *myalloc(void *q, unsigned n, unsigned m) {
    (void)q;
    return calloc(n, m);
}

static void myfree(void *q, void *p) {
    (void)q;
    free(p);
}

static alloc_func zalloc = myalloc;
static free_func zfree = myfree;

#else /* !Z_SOLO */

static alloc_func zalloc = (alloc_func)0;
static free_func zfree = (free_func)0;

/* ===========================================================================
 * Test compress() and uncompress()
 */
static void test_compress(Byte *compr, uLong comprLen, Byte *uncompr,
                   uLong uncomprLen) {
    int err;
    uLong len = (uLong)strlen(hello)+1;

    err = compress(compr, &comprLen, (const Bytef*)hello, len);
    CHECK_ERR(err, "compress");

    strcpy((char*)uncompr, "garbage");

    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
    CHECK_ERR(err, "uncompress");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad uncompress\n");
        exit(1);
    } else {
        printf("uncompress(): %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test read/write of .gz files
 */
static void test_gzio(const char *fname, Byte *uncompr, uLong uncomprLen) {
#ifdef NO_GZCOMPRESS
    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
#else
    int err;
    int len = (int)strlen(hello)+1;
    gzFile file;
    z_off_t pos;

    file = gzopen(fname, "wb");
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    gzputc(file, 'h');
    if (gzputs(file, "ello") != 4) {
        fprintf(stderr, "gzputs err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (gzprintf(file, ", %s!", "hello") != 8) {
        fprintf(stderr, "gzprintf err: %s\n", gzerror(file, &err));
        exit(1);
    }
    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
    gzclose(file);

    file = gzopen(fname, "rb");
    if (file == NULL) {
        fprintf(stderr, "gzopen error\n");
        exit(1);
    }
    strcpy((char*)uncompr, "garbage");

    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
        fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad gzread: %s\n", (char*)uncompr);
        exit(1);
    } else {
        printf("gzread(): %s\n", (char*)uncompr);
    }

    pos = gzseek(file, -8L, SEEK_CUR);
    if (pos != 6 || gztell(file) != pos) {
        fprintf(stderr, "gzseek error, pos=%ld, gztell=%ld\n",
                (long)pos, (long)gztell(file));
        exit(1);
    }

    if (gzgetc(file) != ' ') {
        fprintf(stderr, "gzgetc error\n");
        exit(1);
    }

    if (gzungetc(' ', file) != ' ') {
        fprintf(stderr, "gzungetc error\n");
        exit(1);
    }

    gzgets(file, (char*)uncompr, (int)uncomprLen);
    if (strlen((char*)uncompr) != 7) { /* " hello!" */
        fprintf(stderr, "gzgets err after gzseek: %s\n", gzerror(file, &err));
        exit(1);
    }
    if (strcmp((char*)uncompr, hello + 6)) {
        fprintf(stderr, "bad gzgets after gzseek\n");
        exit(1);
    } else {
        printf("gzgets() after gzseek: %s\n", (char*)uncompr);
    }

    gzclose(file);
#endif
}

#endif /* Z_SOLO */

/* ===========================================================================
 * Test deflate() with small buffers
 */
static void test_deflate(Byte *compr, uLong comprLen) {
    z_stream c_stream; /* compression stream */
    int err;
    uLong len = (uLong)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;

    while (c_stream.total_in != len && c_stream.total_out < comprLen) {
        c_stream.avail_in = c_stream.avail_out = 1; /* force small buffers */
        err = deflate(&c_stream, Z_NO_FLUSH);
        CHECK_ERR(err, "deflate");
    }
    /* Finish the stream, still forcing small buffers: */
    for (;;) {
        c_stream.avail_out = 1;
        err = deflate(&c_stream, Z_FINISH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "deflate");
    }

    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with small buffers
 */
static void test_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                  uLong uncomprLen) {
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 0;
    d_stream.next_out = uncompr;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad inflate\n");
        exit(1);
    } else {
        printf("inflate(): %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test deflate() with large buffers and dynamic change of compression level
 */
static void test_large_deflate(Byte *compr, uLong comprLen, Byte *uncompr,
                        uLong uncomprLen) {
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_SPEED);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    /* At this point, uncompr is still mostly zeroes, so it should compress
     * very well:
     */
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");
    if (c_stream.avail_in != 0) {
        fprintf(stderr, "deflate not greedy\n");
        exit(1);
    }

    /* Feed in already compressed data and switch to no compression: */
    deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
    c_stream.next_in = compr;
    c_stream.avail_in = (uInt)uncomprLen/2;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    /* Switch back to compressing mode: */
    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
    c_stream.next_in = uncompr;
    c_stream.avail_in = (uInt)uncomprLen;
    err = deflate(&c_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "deflate");

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with large buffers
 */
static void test_large_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                        uLong uncomprLen) {
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    for (;;) {
        d_stream.next_out = uncompr;            /* discard the output */
        d_stream.avail_out = (uInt)uncomprLen;
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        CHECK_ERR(err, "large inflate");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (d_stream.total_out != 2*uncomprLen + uncomprLen/2) {
        fprintf(stderr, "bad large inflate: %lu\n", d_stream.total_out);
        exit(1);
    } else {
        printf("large_inflate(): OK\n");
    }
}

/* ===========================================================================
 * Test deflate() with full flush
 */
static void test_flush(Byte *compr, uLong *comprLen) {
    z_stream c_stream; /* compression stream */
    int err;
    uInt len = (uInt)strlen(hello)+1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.next_out = compr;
    c_stream.avail_in = 3;
    c_stream.avail_out = (uInt)*comprLen;
    err = deflate(&c_stream, Z_FULL_FLUSH);
    CHECK_ERR(err, "deflate");

    compr[3]++; /* force an error in first compressed block */
    c_stream.avail_in = len - 3;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        CHECK_ERR(err, "deflate");
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    *comprLen = c_stream.total_out;
}

/* ===========================================================================
 * Test inflateSync()
 */
static void test_sync(Byte *compr, uLong comprLen, Byte *uncompr,
                      uLong uncomprLen) {
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = 2; /* just read the zlib header */

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflate(&d_stream, Z_NO_FLUSH);
    CHECK_ERR(err, "inflate");

    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
    err = inflateSync(&d_stream);           /* but skip the damaged part */
    CHECK_ERR(err, "inflateSync");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    printf("after inflateSync(): hel%s\n", (char *)uncompr);
}

/* ===========================================================================
 * Test deflate() with preset dictionary
 */
static void test_dict_deflate(Byte *compr, uLong comprLen) {
    z_stream c_stream; /* compression stream */
    int err;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    err = deflateSetDictionary(&c_stream,
                (const Bytef*)dictionary, (int)sizeof(dictionary));
    CHECK_ERR(err, "deflateSetDictionary");

    dictId = c_stream.adler;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    c_stream.next_in = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)strlen(hello)+1;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");
}

/* ===========================================================================
 * Test inflate() with a preset dictionary
 */
static void test_dict_inflate(Byte *compr, uLong comprLen, Byte *uncompr,
                       uLong uncomprLen) {
    int err;
    z_stream d_stream; /* decompression stream */

    strcpy((char*)uncompr, "garbage");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)comprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    for (;;) {
        err = inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err == Z_NEED_DICT) {
            if (d_stream.adler != dictId) {
                fprintf(stderr, "unexpected dictionary");
                exit(1);
            }
            err = inflateSetDictionary(&d_stream, (const Bytef*)dictionary,
                                       (int)sizeof(dictionary));
        }
        CHECK_ERR(err, "inflate with dict");
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad inflate with dict\n");
        exit(1);
    } else {
        printf("inflate with dictionary: %s\n", (char *)uncompr);
    }
}

/* ===========================================================================
 * Test crc32() and adler32() checksum functions
 */
static void test_checksums(void) {
    uLong crc, adler;
    const char *data1 = "hello";
    const char *data2 = ", world";

    /* Basic crc32 */
    crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)data1, (uInt)strlen(data1));
    if (crc == 0) {
        fprintf(stderr, "bad crc32\n");
        exit(1);
    }

    /* Incremental crc32 */
    {
        uLong crc2 = crc32(0L, Z_NULL, 0);
        uLong crc_combined;
        crc2 = crc32(crc2, (const Bytef*)data1, (uInt)strlen(data1));
        crc2 = crc32(crc2, (const Bytef*)data2, (uInt)strlen(data2));

        crc_combined = crc32(0L, Z_NULL, 0);
        crc_combined = crc32(crc_combined, (const Bytef*)data1, (uInt)strlen(data1));
        crc_combined = crc32(crc_combined, (const Bytef*)data2, (uInt)strlen(data2));

        if (crc2 != crc_combined) {
            fprintf(stderr, "bad incremental crc32\n");
            exit(1);
        }
    }

    /* Basic adler32 */
    adler = adler32(0L, Z_NULL, 0);
    adler = adler32(adler, (const Bytef*)data1, (uInt)strlen(data1));
    if (adler == 0) {
        fprintf(stderr, "bad adler32\n");
        exit(1);
    }

    /* Incremental adler32 */
    {
        uLong adler2 = adler32(0L, Z_NULL, 0);
        uLong adler_combined;
        adler2 = adler32(adler2, (const Bytef*)data1, (uInt)strlen(data1));
        adler2 = adler32(adler2, (const Bytef*)data2, (uInt)strlen(data2));

        adler_combined = adler32(0L, Z_NULL, 0);
        adler_combined = adler32(adler_combined, (const Bytef*)data1, (uInt)strlen(data1));
        adler_combined = adler32(adler_combined, (const Bytef*)data2, (uInt)strlen(data2));

        if (adler2 != adler_combined) {
            fprintf(stderr, "bad incremental adler32\n");
            exit(1);
        }
    }

    printf("checksums(): OK\n");
}

/* ===========================================================================
 * Test compress2() and compressBound()
 */
static void test_compress2(Byte *compr, uLong comprLen, Byte *uncompr,
                            uLong uncomprLen) {
    int err;
    uLong len = (uLong)strlen(hello) + 1;
    uLong bound;
    int level;

    /* Test compressBound */
    bound = compressBound(len);
    if (bound < len) {
        fprintf(stderr, "bad compressBound\n");
        exit(1);
    }

    /* Test compress2 with several compression levels */
    for (level = Z_BEST_SPEED; level <= Z_BEST_COMPRESSION; level++) {
        uLong cLen = comprLen;
        uLong uLen = uncomprLen;

        err = compress2(compr, &cLen, (const Bytef*)hello, len, level);
        CHECK_ERR(err, "compress2");

        strcpy((char*)uncompr, "garbage");
        err = uncompress(uncompr, &uLen, compr, cLen);
        CHECK_ERR(err, "uncompress (compress2)");

        if (strcmp((char*)uncompr, hello)) {
            fprintf(stderr, "bad compress2 at level %d\n", level);
            exit(1);
        }
    }
    printf("compress2(): OK\n");
}

/* ===========================================================================
 * Test deflateInit2() with raw deflate and gzip format
 */
static void test_deflate_init2(Byte *compr, uLong comprLen, Byte *uncompr,
                                uLong uncomprLen) {
    int err;
    z_stream c_stream;
    z_stream d_stream;
    uLong len = (uLong)strlen(hello) + 1;

    /* Raw deflate (negative windowBits) */
    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                       -15, 8, Z_DEFAULT_STRATEGY);
    CHECK_ERR(err, "deflateInit2 raw");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)len;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate (raw) should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd raw");

    /* Inflate raw */
    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)c_stream.total_out;
    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflateInit2(&d_stream, -15);
    CHECK_ERR(err, "inflateInit2 raw");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate (raw) should report Z_STREAM_END\n");
        exit(1);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd raw");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad deflateInit2 raw round-trip\n");
        exit(1);
    }
    printf("deflateInit2(): OK\n");
}

/* ===========================================================================
 * Test deflateReset() and inflateReset() for stream reuse
 */
static void test_stream_reset(Byte *compr, uLong comprLen, Byte *uncompr,
                               uLong uncomprLen) {
    int err;
    z_stream c_stream;
    z_stream d_stream;
    uLong len = (uLong)strlen(hello) + 1;
    uLong cLen1, cLen2;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    /* First compression */
    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)len;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;
    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    cLen1 = c_stream.total_out;

    /* Reset and compress again */
    err = deflateReset(&c_stream);
    CHECK_ERR(err, "deflateReset");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)len;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;
    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate after reset should report Z_STREAM_END\n");
        exit(1);
    }
    cLen2 = c_stream.total_out;

    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    if (cLen1 != cLen2) {
        fprintf(stderr, "deflateReset produced different output sizes\n");
        exit(1);
    }

    /* Inflate with reset */
    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)cLen1;
    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate should report Z_STREAM_END\n");
        exit(1);
    }

    /* Reset and inflate again */
    err = inflateReset(&d_stream);
    CHECK_ERR(err, "inflateReset");

    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)cLen1;
    strcpy((char*)uncompr, "garbage");
    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate after reset should report Z_STREAM_END\n");
        exit(1);
    }

    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad stream_reset inflate result\n");
        exit(1);
    }
    printf("stream_reset(): OK\n");
}

/* ===========================================================================
 * Test compress/inflate with empty (zero-length) input
 */
static void test_empty_input(Byte *compr, uLong comprLen, Byte *uncompr,
                              uLong uncomprLen) {
    int err;
    uLong cLen = comprLen;
    uLong uLen = uncomprLen;

    /* compress with zero-length input */
    err = compress(compr, &cLen, (const Bytef*)"", 0);
    CHECK_ERR(err, "compress empty");

    /* uncompress the empty stream */
    err = uncompress(uncompr, &uLen, compr, cLen);
    CHECK_ERR(err, "uncompress empty");

    if (uLen != 0) {
        fprintf(stderr, "bad empty uncompress: expected 0 bytes, got %lu\n", uLen);
        exit(1);
    }
    printf("empty_input(): OK\n");
}

/* ===========================================================================
 * Test deflate() with Z_SYNC_FLUSH
 */
static void test_sync_flush(Byte *compr, uLong comprLen, Byte *uncompr,
                             uLong uncomprLen) {
    int err;
    z_stream c_stream;
    z_stream d_stream;
    uLong len = (uLong)strlen(hello) + 1;

    c_stream.zalloc = zalloc;
    c_stream.zfree = zfree;
    c_stream.opaque = (voidpf)0;

    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
    CHECK_ERR(err, "deflateInit");

    c_stream.next_in  = (z_const unsigned char *)hello;
    c_stream.avail_in = (uInt)len / 2;
    c_stream.next_out = compr;
    c_stream.avail_out = (uInt)comprLen;

    err = deflate(&c_stream, Z_SYNC_FLUSH);
    CHECK_ERR(err, "deflate Z_SYNC_FLUSH");

    c_stream.avail_in = (uInt)len - (uInt)len / 2;
    err = deflate(&c_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "deflate should report Z_STREAM_END\n");
        exit(1);
    }
    err = deflateEnd(&c_stream);
    CHECK_ERR(err, "deflateEnd");

    d_stream.zalloc = zalloc;
    d_stream.zfree = zfree;
    d_stream.opaque = (voidpf)0;
    d_stream.next_in  = compr;
    d_stream.avail_in = (uInt)c_stream.total_out;
    d_stream.next_out = uncompr;
    d_stream.avail_out = (uInt)uncomprLen;

    err = inflateInit(&d_stream);
    CHECK_ERR(err, "inflateInit");

    err = inflate(&d_stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        fprintf(stderr, "inflate (sync_flush) should report Z_STREAM_END\n");
        exit(1);
    }
    err = inflateEnd(&d_stream);
    CHECK_ERR(err, "inflateEnd");

    if (strcmp((char*)uncompr, hello)) {
        fprintf(stderr, "bad sync_flush inflate result\n");
        exit(1);
    }
    printf("sync_flush(): OK\n");
}

/* ===========================================================================
 * Usage:  example [output.gz  [input.gz]]
 */

int main(int argc, char *argv[]) {
    Byte *compr, *uncompr;
    uLong uncomprLen = 20000;
    uLong comprLen = 3 * uncomprLen;
    static const char* myVersion = ZLIB_VERSION;

    if (zlibVersion()[0] != myVersion[0]) {
        fprintf(stderr, "incompatible zlib version\n");
        exit(1);

    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
        fprintf(stderr, "warning: different zlib version linked: %s\n",
                zlibVersion());
    }

    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
            ZLIB_VERSION, (unsigned)ZLIB_VERNUM, zlibCompileFlags());

    compr    = (Byte*)calloc((uInt)comprLen, 1);
    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
    /* compr and uncompr are cleared to avoid reading uninitialized
     * data and to ensure that uncompr compresses well.
     */
    if (compr == Z_NULL || uncompr == Z_NULL) {
        printf("out of memory\n");
        exit(1);
    }

#ifdef Z_SOLO
    (void)argc;
    (void)argv;
#else
    test_compress(compr, comprLen, uncompr, uncomprLen);

    test_gzio((argc > 1 ? argv[1] : TESTFILE),
              uncompr, uncomprLen);
#endif

    test_deflate(compr, comprLen);
    test_inflate(compr, comprLen, uncompr, uncomprLen);

    test_large_deflate(compr, comprLen, uncompr, uncomprLen);
    test_large_inflate(compr, comprLen, uncompr, uncomprLen);

    test_flush(compr, &comprLen);
    test_sync(compr, comprLen, uncompr, uncomprLen);
    comprLen = 3 * uncomprLen;

    test_dict_deflate(compr, comprLen);
    test_dict_inflate(compr, comprLen, uncompr, uncomprLen);

    test_checksums();
    test_compress2(compr, comprLen, uncompr, uncomprLen);
    test_deflate_init2(compr, comprLen, uncompr, uncomprLen);
    test_stream_reset(compr, comprLen, uncompr, uncomprLen);
    test_empty_input(compr, comprLen, uncompr, uncomprLen);
    test_sync_flush(compr, comprLen, uncompr, uncomprLen);

    free(compr);
    free(uncompr);

    return 0;
}
