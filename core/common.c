/*****************************************************************************
 * common.c: h264 library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: common.c,v 1.1 2004/06/03 19:27:06 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "common.h"
#include "cpu.h"

/****************************************************************************
 * x264_param_default:
 ****************************************************************************/
void    x264_param_default( x264_param_t *param )
{
    /* */
    memset( param, 0, sizeof( x264_param_t ) );

    /* CPU autodetect */
    param->cpu = x264_cpu_detect();
    fprintf( stderr, "x264: cpu capabilities: %s%s%s%s%s%s\n",
             param->cpu&X264_CPU_MMX ? "MMX " : "",
             param->cpu&X264_CPU_MMXEXT ? "MMXEXT " : "",
             param->cpu&X264_CPU_SSE ? "SSE " : "",
             param->cpu&X264_CPU_SSE2 ? "SSE2 " : "",
             param->cpu&X264_CPU_3DNOW ? "3DNow! " : "",
             param->cpu&X264_CPU_ALTIVEC ? "Altivec " : "" );


    /* Video properties */
    param->i_csp           = X264_CSP_I420;
    param->i_width         = 0;
    param->i_height        = 0;
    param->vui.i_sar_width = 0;
    param->vui.i_sar_height= 0;
    param->f_fps           = 25.0;

    /* Encoder parameters */
    param->i_frame_reference = 1;
    param->i_idrframe = 2;
    param->i_iframe = 60;
    param->i_bframe = 0;

    param->b_deblocking_filter = 1;
    param->i_deblocking_filter_alphac0 = 0;
    param->i_deblocking_filter_beta = 0;

    param->b_cabac = 0;
    param->i_cabac_init_idc = -1;

    param->i_bitrate = 3000;
    param->i_qp_constant = 26;

    param->analyse.intra = X264_ANALYSE_I4x4;
    param->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
}

/****************************************************************************
 * x264_picture_alloc:
 ****************************************************************************/
void x264_picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height )
{
    pic->i_type = X264_TYPE_AUTO;
    pic->i_qpplus1 = 0;
    pic->img.i_csp = i_csp;
    switch( i_csp & X264_CSP_MASK )
    {
        case X264_CSP_I420:
        case X264_CSP_YV12:
            pic->img.i_plane = 3;
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height / 2 );
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height / 4;
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width / 2;
            pic->img.i_stride[2] = i_width / 2;
            break;

        case X264_CSP_I422:
            pic->img.i_plane = 3;
            pic->img.plane[0] = x264_malloc( 2 * i_width * i_height );
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height / 2;
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width / 2;
            pic->img.i_stride[2] = i_width / 2;
            break;

        case X264_CSP_I444:
            pic->img.i_plane = 3;
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height );
            pic->img.plane[1] = pic->img.plane[0] + i_width * i_height;
            pic->img.plane[2] = pic->img.plane[1] + i_width * i_height;
            pic->img.i_stride[0] = i_width;
            pic->img.i_stride[1] = i_width;
            pic->img.i_stride[2] = i_width;
            break;

        case X264_CSP_YUYV:
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 2 * i_width * i_height );
            pic->img.i_stride[0] = 2 * i_width;
            break;

        case X264_CSP_RGB:
        case X264_CSP_BGR:
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 3 * i_width * i_height );
            pic->img.i_stride[0] = 3 * i_width;
            break;

        case X264_CSP_BGRA:
            pic->img.i_plane = 1;
            pic->img.plane[0] = x264_malloc( 4 * i_width * i_height );
            pic->img.i_stride[0] = 4 * i_width;
            break;

        default:
            fprintf( stderr, "invalid CSP\n" );
            pic->img.i_plane = 0;
            break;
    }
}

/****************************************************************************
 * x264_picture_clean:
 ****************************************************************************/
void x264_picture_clean( x264_picture_t *pic )
{
    x264_free( pic->img.plane[0] );

    /* just to be safe */
    memset( pic, 0, sizeof( x264_picture_t ) );
}

/****************************************************************************
 * x264_nal_encode:
 ****************************************************************************/
int x264_nal_encode( void *p_data, int *pi_data, int b_annexeb, x264_nal_t *nal )
{
    uint8_t *dst = p_data;
    uint8_t *src = nal->p_payload;
    uint8_t *end = &nal->p_payload[nal->i_payload];

    int i_count = 0;

    /* FIXME this code doesn't check overflow */

    if( b_annexeb )
    {
        /* long nal start code (we always use long ones)*/
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    }

    /* nal header */
    *dst++ = ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type;

    while( src < end )
    {
        if( i_count == 2 && *src <= 0x03 )
        {
            *dst++ = 0x03;
            i_count = 0;
        }
        if( *src == 0 )
        {
            i_count++;
        }
        else
        {
            i_count = 0;
        }
        *dst++ = *src++;
    }
    *pi_data = dst - (uint8_t*)p_data;

    return *pi_data;
}

/****************************************************************************
 * x264_nal_decode:
 ****************************************************************************/
int x264_nal_decode( x264_nal_t *nal, void *p_data, int i_data )
{
    uint8_t *src = p_data;
    uint8_t *end = &src[i_data];
    uint8_t *dst = nal->p_payload;

    nal->i_type    = src[0]&0x1f;
    nal->i_ref_idc = (src[0] >> 5)&0x03;

    src++;

    while( src < end )
    {
        if( src < end - 3 && src[0] == 0x00 && src[1] == 0x00  && src[2] == 0x03 )
        {
            *dst++ = 0x00;
            *dst++ = 0x00;

            src += 3;
            continue;
        }
        *dst++ = *src++;
    }

    nal->i_payload = dst - (uint8_t*)p_data;
    return 0;
}



/****************************************************************************
 * x264_malloc:
 ****************************************************************************/
void *x264_malloc( int i_size )
{
#ifdef HAVE_MALLOC_H
    return memalign( 16, i_size );
#else
    uint8_t * buf;
    uint8_t * align_buf;
    buf = (uint8_t *) malloc( i_size + 15 + sizeof( void ** ) +
              sizeof( int ) );
    align_buf = buf + 15 + sizeof( void ** ) + sizeof( int );
    align_buf -= (long) align_buf & 15;
    *( (void **) ( align_buf - sizeof( void ** ) ) ) = buf;
    *( (int *) ( align_buf - sizeof( void ** ) - sizeof( int ) ) ) = i_size;
    return align_buf;
#endif
}

/****************************************************************************
 * x264_free:
 ****************************************************************************/
void x264_free( void *p )
{
    if( p )
    {
#ifdef HAVE_MALLOC_H
        free( p );
#else
        free( *( ( ( void **) p ) - 1 ) );
#endif
    }
}

/****************************************************************************
 * x264_realloc:
 ****************************************************************************/
void *x264_realloc( void *p, int i_size )
{
#ifdef HAVE_MALLOC_H
    return realloc( p, i_size );
#else
    int       i_old_size = 0;
    uint8_t * p_new;
    if( p )
    {
        i_old_size = *( (int*) ( (uint8_t*) p ) - sizeof( void ** ) -
                         sizeof( int ) );
    }
    p_new = x264_malloc( i_size );
    if( i_old_size > 0 && i_size > 0 )
    {
        memcpy( p_new, p, ( i_old_size < i_size ) ? i_old_size : i_size );
    }
    x264_free( p );
    return p_new;
#endif
}

