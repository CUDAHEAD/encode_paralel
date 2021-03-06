#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#
#include "core/common.h"
#ifdef HAVE_MMXEXT
#include "core/i386/pixel.h"
#include "core/i386/dct.h"
#include "core/i386/mc.h"
#endif
#ifdef HAVE_ALTIVEC
#include "core/ppc/pixel.h"
#endif

/* buf1, buf2: initialised to randome data and shouldn't write into them */
uint8_t * buf1, * buf2;
/* buf3, buf4: used to store output */
uint8_t * buf3, * buf4;

static int check_pixel()
{
    x264_pixel_function_t pixel_c = {{0},{0},{0}};
    x264_pixel_function_t pixel_asm = {{0}, {0},{0}};
    int ret = 0, ok;
    int i;

    memset( &pixel_asm, 0, sizeof( x264_pixel_function_t ) );
    x264_pixel_init( 0, &pixel_c );
#ifdef HAVE_MMXEXT
    x264_pixel_init( X264_CPU_MMX|X264_CPU_MMXEXT, &pixel_asm );
#endif
#ifdef HAVE_ALTIVEC
    x264_pixel_altivec_init( &pixel_asm );
#endif

    for( i = 0, ok = 1; i < 7; i++ )
    {
        int res_c, res_asm;
        if( pixel_asm.sad[i] )
        {
            res_c   = pixel_c.sad[i]( buf1, 32, buf2, 32 );
            res_asm =  pixel_asm.sad[i]( buf1, 32, buf2, 32 );
            if( res_c != res_asm )
            {
                ok = 0;
                fprintf( stderr, "sad[%d]: %d != %d [FAILED]\n", i, res_c, res_asm );
            }
        }
    }
    if( ok )
        fprintf( stderr, " - pixel sad :           [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - pixel sat :           [FAILED]\n" );
    }

    for( i = 0, ok = 1; i < 7; i++ )
    {
        int res_c, res_asm;
        if( pixel_asm.satd[i] )
        {
            res_c   = pixel_c.satd[i]( buf1, 32, buf2, 32 );
            res_asm = pixel_asm.satd[i]( buf1, 32, buf2, 32 );
            if( res_c != res_asm )
            {
                ok = 0;
                fprintf( stderr, "satd[%d]: %d != %d [FAILED]\n", i, res_c, res_asm );
            }
        }
    }

    if( ok )
        fprintf( stderr, " - pixel satd :          [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - pixel satd :          [FAILED]\n" );
    }

    for( i = 0, ok = 1; i < 7; i++ )
    {
        if( pixel_asm.avg[i] )
        {
            memcpy( buf3, buf1, 32*32 );
            memcpy( buf4, buf1, 32*32 );
            pixel_c.satd[i]( buf3, 32, buf2, 32 );
            pixel_asm.satd[i]( buf4, 32, buf2, 32 );
            if( memcmp( buf3, buf4, 32*32 ) )
            {
                ok = 0;
                fprintf( stderr, "avg[%d]: [FAILED]\n", i );
            }
        }
    }

    if( ok )
        fprintf( stderr, " - pixel avg :           [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - pixel avg :           [FAILED]\n" );
    }

    return ret;
}

static int check_dct()
{
    x264_dct_function_t dct_c;
    x264_dct_function_t dct_asm;
    int ret = 0, ok;
    int16_t dct1[16][4][4] __attribute((aligned(16)));
    int16_t dct2[16][4][4] __attribute((aligned(16)));

    memset( &dct_asm, 0, sizeof( dct_asm ) );
    x264_dct_init( 0, &dct_c );
#ifdef HAVE_MMXEXT
    x264_dct_init( X264_CPU_MMX|X264_CPU_MMXEXT, &dct_asm );
#endif
#define TEST_DCT( name, t1, t2, size ) \
    if( dct_asm.name ) \
    { \
        dct_c.name( t1, buf1, 32, buf2, 32 ); \
        dct_asm.name( t2, buf1, 32, buf2, 32 ); \
        if( memcmp( t1, t2, size ) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
    }
    ok = 1;
    TEST_DCT( sub4x4_dct, dct1[0], dct2[0], 16*2 );
    TEST_DCT( sub8x8_dct, dct1, dct2, 16*2*4 );
    TEST_DCT( sub16x16_dct, dct1, dct2, 16*2*16 );
    if( ok )
        fprintf( stderr, " - sub_dctXxX :          [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - sub_dctXxX :          [FAILED]\n" );
    }
#undef TEST_DCT

#define TEST_IDCT( name, t ) \
    if( dct_asm.name ) \
    { \
        memcpy( buf3, buf1, 32*32 ); \
        memcpy( buf4, buf1, 32*32 ); \
        dct_c.name( buf3, 32, t ); \
        dct_asm.name( buf4, 32, t ); \
        if( memcmp( buf3, buf4, 32*32 ) ) \
        { \
            ok = 0; \
            fprintf( stderr, #name " [FAILED]\n" ); \
        } \
    }
    ok = 1;
    TEST_IDCT( add4x4_idct, dct1[0] );
    TEST_IDCT( add8x8_idct, dct1 );
    TEST_IDCT( add16x16_idct, dct1 );
    if( ok )
        fprintf( stderr, " - add_idctXxX :         [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - add_idctXxX :         [FAILED]\n" );
    }
#undef TEST_IDCT

    ok = 1;
    if( dct_asm.dct4x4dc )
    {
        int16_t dct1[4][4] __attribute((aligned(16))) = { {-12, 42, 23, 67},{2, 90, 89,56}, {67,43,-76,91},{56,-78,-54,1}};
        int16_t dct2[4][4] __attribute((aligned(16))) = { {-12, 42, 23, 67},{2, 90, 89,56}, {67,43,-76,91},{56,-78,-54,1}};

        dct_c.dct4x4dc( dct1 );
        dct_asm.dct4x4dc( dct2 );
        if( memcmp( dct1, dct2, 32 ) )
        {
            ok = 0;
            fprintf( stderr, " - dct4x4dc :        [FAILED]\n" );
        }
    }
    if( dct_asm.idct4x4dc )
    {
        int16_t dct1[4][4] __attribute((aligned(16))) = { {-12, 42, 23, 67},{2, 90, 89,56}, {67,43,-76,91},{56,-78,-54,1}};
        int16_t dct2[4][4] __attribute((aligned(16))) = { {-12, 42, 23, 67},{2, 90, 89,56}, {67,43,-76,91},{56,-78,-54,1}};

        dct_c.idct4x4dc( dct1 );
        dct_asm.idct4x4dc( dct2 );
        if( memcmp( dct1, dct2, 32 ) )
        {
            ok = 0;
            fprintf( stderr, " - idct4x4dc :        [FAILED]\n" );
        }
    }
    if( ok )
        fprintf( stderr, " - (i)dct4x4dc :         [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - (i)dct4x4dc :         [FAILED]\n" );
    }

    ok = 1;
    if( dct_asm.dct2x2dc )
    {
        int16_t dct1[2][2] __attribute((aligned(16))) = { {-12, 42},{2, 90}};
        int16_t dct2[2][2] __attribute((aligned(16))) = { {-12, 42},{2, 90}};

        dct_c.dct2x2dc( dct1 );
        dct_asm.dct2x2dc( dct2 );
        if( memcmp( dct1, dct2, 4*2 ) )
        {
            ok = 0;
            fprintf( stderr, " - dct2x2dc :        [FAILED]\n" );
        }
    }
    if( dct_asm.idct2x2dc )
    {
        int16_t dct1[2][2] __attribute((aligned(16))) = { {-12, 42},{2, 90}};
        int16_t dct2[2][2] __attribute((aligned(16))) = { {-12, 42},{2, 90}};

        dct_c.idct2x2dc( dct1 );
        dct_asm.idct2x2dc( dct2 );
        if( memcmp( dct1, dct2, 4*2 ) )
        {
            ok = 0;
            fprintf( stderr, " - idct2x2dc :       [FAILED]\n" );
        }
    }

    if( ok )
        fprintf( stderr, " - (i)dct2x2dc :         [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - (i)dct2x2dc :         [FAILED]\n" );
    }


    return ret;
}

static int check_mc()
{
    x264_mc_function_t mc_c[2] = {0};
    x264_mc_function_t mc_asm[2] = {0};
    uint8_t *src = &buf1[2*32+2];
    uint8_t *dst1 = &buf3[2*32+2];
    uint8_t *dst2 = &buf4[2*32+2];
    int dx, dy;
    int ret = 0, ok[2] = { 1, 1 };

    x264_mc_init( 0, mc_c );
#ifdef HAVE_MMXEXT
    x264_mc_mmxext_init( mc_asm );
#endif

    memset( buf3, 0, 32*32 );
    memset( buf4, 0, 32*32 );

    /* Do the MC */
#define MC_TEST( t, w, h ) \
        if( mc_asm[t] ) \
        { \
            memset(dst1, 0xCD, (h) * 16); \
            mc_c[t]( src, 32, dst1, 16, dx, dy, w, h );     \
            memset(dst2, 0xCD, (h) * 16); \
            mc_asm[t]( src, 32, dst2, 16, dx, dy, w, h );   \
            if( memcmp( dst1, dst2, 16*16 ) )               \
            { \
                fprintf( stderr, "mc["#t"][mv(%d,%d) %2dx%-2d]     [FAILED]\n", dx, dy, w, h );   \
                ok[t] = 0; \
            } \
        }

    for( dy = 0; dy < 4; dy++ )
    {
        for( dx = 0; dx < 4; dx++ )
        {
            MC_TEST( 0, 16, 16 );
            MC_TEST( 0, 16, 8 );
            MC_TEST( 0, 8, 16 );
            MC_TEST( 0, 8, 8 );
            MC_TEST( 0, 8, 4 );
            MC_TEST( 0, 4, 8 );
            MC_TEST( 0, 4, 4 );

            MC_TEST( 1, 8, 8 );
            MC_TEST( 1, 8, 4 );
            MC_TEST( 1, 4, 8 );
            MC_TEST( 1, 4, 4 );
            MC_TEST( 1, 4, 2 );
            MC_TEST( 1, 2, 4 );
            MC_TEST( 1, 2, 2 );
        }
    }
#undef MC_TEST
    if( ok[0] )
        fprintf( stderr, " - mc luma :             [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - mc luma :             [FAILED]\n" );
    }
    if( ok[1] )
        fprintf( stderr, " - mc chroma :           [OK]\n" );
    else {
        ret = -1;
        fprintf( stderr, " - mc chroma :           [FAILED]\n" );
    }
    return ret;
}

int main()
{
    int ret;
    int i;

#ifdef HAVE_MMXEXT
    fprintf( stderr, "x264: MMXEXT against C\n" );
#elif HAVE_ALTIVEC
    fprintf( stderr, "x264: ALTIVEC against C\n" );
#endif

    buf1 = x264_malloc( 1024 ); /* 32 x 32 */
    buf2 = x264_malloc( 1024 );
    buf3 = x264_malloc( 1024 );
    buf4 = x264_malloc( 1024 );

    srand( x264_mdate() );

    for( i = 0; i < 1024; i++ )
    {
        buf1[i] = rand() % 0xFF;
        buf2[i] = rand() % 0xFF;
        buf3[i] = buf4[i] = 0;
    }

    ret = check_pixel() +
          check_dct() +
          check_mc();

    if( ret == 0 )
    {
        fprintf( stderr, "x264: All tests passed Yeah :)\n" );
        return 0;
    }
    fprintf( stderr, "x264: at least one test has failed. Go and fix that Right Now!\n" );
    return -1;
}

