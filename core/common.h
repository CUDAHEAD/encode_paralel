/*****************************************************************************
 * common.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: common.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
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

#ifndef _COMMON_H
#define _COMMON_H 1

#include <stdint.h>

#include "../x264.h"
#include "bs.h"
#include "set.h"
#include "predict.h"
#include "pixel.h"
#include "mc.h"
#include "frame.h"
#include "dct.h"
#include "cabac.h"
#include "csp.h"

#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define X264_ABS(a)   ( (a)< 0 ? -(a) : (a) )

/* x264_malloc : will do or emulate a memalign
 * XXX you HAVE TO use x264_free for buffer allocated
 * with x264_malloc
 */
void *x264_malloc( int );
void *x264_realloc( void *p, int i_size );
void  x264_free( void * );

/* mdate: return the current date in microsecond */
int64_t x264_mdate( void );

static inline int x264_clip3( int v, int i_min, int i_max )
{
    if( v < i_min )
    {
        return i_min;
    }
    else if( v > i_max )
    {
        return i_max;
    }
    else
    {
        return v;
    }
}

enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,
    SLICE_TYPE_SI = 4
};

typedef struct
{
    x264_sps_t *sps;
    x264_pps_t *pps;

    int i_type;
    int i_first_mb;

    int i_pps_id;

    int i_frame_num;

    int b_field_pic;
    int b_bottom_field;

    int i_idr_pic_id;   /* -1 if nal_type != 5 */

    int i_poc_lsb;
    int i_delta_poc_bottom;

    int i_delta_poc[2];
    int i_redundant_pic_cnt;

    int b_direct_spatial_mv_pred;

    int b_num_ref_idx_override;
    int i_num_ref_idx_l0_active;
    int i_num_ref_idx_l1_active;

    int i_cabac_init_idc;

    int i_qp_delta;
    int b_sp_for_swidth;
    int i_qs_delta;

    /* deblocking filter */
    int i_disable_deblocking_filter_idc;
    int i_alpha_c0_offset;
    int i_beta_offset;

} x264_slice_header_t;

/* From ffmpeg
 */
#define X264_SCAN8_SIZE (6*8)
#define X264_SCAN8_0 (4+1*8)

static const int x264_scan8[16+2*4] =
{
    /* Luma */
    4+1*8, 5+1*8, 4+2*8, 5+2*8,
    6+1*8, 7+1*8, 6+2*8, 7+2*8,
    4+3*8, 5+3*8, 4+4*8, 5+4*8,
    6+3*8, 7+3*8, 6+4*8, 7+4*8,

    /* Cb */
    1+1*8, 2+1*8,
    1+2*8, 2+2*8,

    /* Cr */
    1+4*8, 2+4*8,
    1+5*8, 2+5*8,
};
/*
   0 1 2 3 4 5 6 7
 0
 1   B B   L L L L
 2   B B   L L L L
 3         L L L L
 4   R R   L L L L
 5   R R
*/

#define X264_BFRAME_MAX 16

typedef struct x264_ratecontrol_t   x264_ratecontrol_t;
typedef struct x264_vlc_table_t     x264_vlc_table_t;

struct x264_t
{
    /* encoder parameters */
    x264_param_t    param;

    /* bitstream output */
    struct
    {
        int         i_nal;
        x264_nal_t  nal[3];         /* for now 3 is enought */
        int         i_bitstream;    /* size of p_bitstream */
        uint8_t     *p_bitstream;   /* will hold data for all nal */
        bs_t        bs;
    } out;

    /* frame number/poc */
    int             i_frame;
    int             i_poc;

    int             i_frame_offset; /* decoding only */
    int             i_frame_num;    /* decoding only */
    int             i_poc_msb;      /* decoding only */
    int             i_poc_lsb;      /* decoding only */

    /* We use only one SPS and one PPS */
    x264_sps_t      sps_array[32];
    x264_sps_t      *sps;
    x264_pps_t      pps_array[256];
    x264_pps_t      *pps;
    int             i_idr_pic_id;

    /* Slice header */
    x264_slice_header_t sh;

    /* cabac context */
    x264_cabac_t    cabac;

    struct
    {
        /* Frames to be encoded */
        x264_frame_t *current[X264_BFRAME_MAX+1];
        /* Temporary buffer (eg B frames pending until we reach the I/P) */
        x264_frame_t *next[X264_BFRAME_MAX+1];
        /* Unused frames */
        x264_frame_t *unused[X264_BFRAME_MAX+1];

        /* frames used for reference +1 for decoding */
        x264_frame_t *reference[16+1];

        int i_last_idr; /* How many I non IDR frames from last IDR */
        int i_last_i;   /* How many P/B frames from last I */
    } frames;

    /* current frame being encoded */
    x264_frame_t    *fenc;

    /* frame being reconstructed */
    x264_frame_t    *fdec;

    /* references lists */
    int             i_ref0;
    x264_frame_t    *fref0[16];       /* ref list 0 */
    int             i_ref1;
    x264_frame_t    *fref1[16];       /* ref list 1 */



    /* Current MB DCT coeffs */
    struct
    {
        DECLARE_ALIGNED( int, luma16x16_dc[16], 16 );
        DECLARE_ALIGNED( int, chroma_dc[2][4], 16 );
        struct
        {
            DECLARE_ALIGNED( int, residual_ac[15], 16 );
            DECLARE_ALIGNED( int, luma4x4[16], 16 );
        } block[16+8];
    } dct;

    /* MB table and cache for current frame/mb */
    struct
    {
        /* Strides */
        int     i_mb_stride;

        /* Current index */
        int     i_mb_x;
        int     i_mb_y;
        int     i_mb_xy;

        unsigned int i_neighbour;

        /* mb table */
        int8_t  *type;                      /* mb type */
        int8_t  *qp;                        /* mb qp */
        int16_t *cbp;                       /* mb cbp: 0x0?: luma, 0x?0: chroma, 0x100: luma dc, 0x0200 and 0x0400: chroma dc  (all set for PCM)*/
        int8_t  (*intra4x4_pred_mode)[7];   /* intra4x4 pred mode. for non I4x4 set to I_PRED_4x4_DC(2) */
        uint8_t (*non_zero_count)[16+4+4];  /* nzc. for I_PCM set to 16 */
        int8_t  *chroma_pred_mode;          /* chroma_pred_mode. cabac only. for non intra I_PRED_CHROMA_DC(0) */
        int16_t (*mv[2])[2];                /* mb mv. set to 0 for intra mb */
        int16_t (*mvd[2])[2];               /* mb mv difference with predict. set to 0 if intra. cabac only */
        int8_t   *ref[2];                   /* mb ref. set to -1 if non used (intra or Lx only */

        /* current value */
        int     i_type;
        int     i_partition;
        int     i_sub_partition[4];

        int     i_cbp_luma;
        int     i_cbp_chroma;

        int     i_intra16x16_pred_mode;
        int     i_chroma_pred_mode;

        struct
        {
            /* pointer over mb of the frame to be compressed */
            uint8_t *p_fenc[3];

            /* pointer over mb of the frame to be reconstrucated  */
            uint8_t *p_fdec[3];

            /* pointer over mb of the references */
            uint8_t *p_fref[2][16][3];

            /* common stride */
            int     i_stride[3];
        } pic;

        /* cache */
        struct
        {
            /* real intra4x4_pred_mode if I_4X4, I_PRED_4x4_DC if mb available, -1 if not */
            int     intra4x4_pred_mode[X264_SCAN8_SIZE];

            /* i_non_zero_count if availble else 0x80 */
            int     non_zero_count[X264_SCAN8_SIZE];

            /* -1 if unused, -2 if unavaible */
            int8_t  ref[2][X264_SCAN8_SIZE];

            /* 0 if non avaible */
            int16_t mv[2][X264_SCAN8_SIZE][2];
            int16_t mvd[2][X264_SCAN8_SIZE][2];
        } cache;

        /* */
        int     i_last_qp;  /* last qp */
        int     i_last_dqp; /* last delta qp */

    } mb;

    /* rate control encoding only */
    x264_ratecontrol_t *rc;

    /* stats */
    struct
    {
        /* per slice info */
        int   i_slice_count[5];
        int   i_slice_size[5];
        float f_psnr_y[5];
        float f_psnr_u[5];
        float f_psnr_v[5];
        int   i_mb_count[5][18];
    } stat;

    /* CPU functions dependants */
    x264_predict_t      predict_16x16[4+3];
    x264_predict_t      predict_8x8[4+3];
    x264_predict_t      predict_4x4[9+3];

    x264_pixel_function_t pixf;
    x264_mc_function_t    mc[2];
    x264_dct_function_t   dctf;
    x264_csp_function_t   csp;

    /* vlc table for decoding purpose only */
    x264_vlc_table_t *x264_coeff_token_lookup[5];
    x264_vlc_table_t *x264_level_prefix_lookup;
    x264_vlc_table_t *x264_total_zeros_lookup[15];
    x264_vlc_table_t *x264_total_zeros_dc_lookup[3];
    x264_vlc_table_t *x264_run_before_lookup[7];
};

#endif

