// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/NBLIC
// 
// NBLIC - Niu-Bi Lossless Image Compression
//   is a lossless & near-lossless image compressor
//
// Advantages:
//   - very high compression ratio, better than state-of-the-art lossless image compression standards such as JPEG-XL lossless, AVIF lossless, etc.
//   - low code with pure C language
//   - acceptable performace (manybe faster in future versions)
//   - only single pass scan to encode a image, good for FPGA hardware streaming impl.
// 
// Development progress:
//   [y] 8-bit gray image lossless      compression/decompression is support now.
//   [y] 8-bit gray image near-lossless compression/decompression is support now.
//   [x] 24-bit RGB image lossless      compression/decompression will be supported later.
//   [x] 24-bit RGB image near-lossless compression/decompression will be supported later.
//
// Warning:
//   Currently in the development phase,
//   so there is no guarantee that the generated compressed files will be compatible with subsequent versions
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "NBLIC.h"

const char *title = "NBLIC0.3";

typedef    unsigned char          UI8;
typedef    uint32_t               U32;
typedef    int64_t                I64;


#define    SWAP(type,a,b)         {type t; (t)=(a); (a)=(b); (b)=(t);}

#define    ABS(x)                 ( ((x) < 0) ? (-(x)) : (x) )                             // get absolute value
#define    CLIP(x,a,b)            ( ((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)) )            // clip x between a~b

#define    G2D(ptr,width,i,j)     (*( (ptr) + (width)*(i) + (j) ))
#define    SPIX(ptr,width,i,j,v0) (((0<=(i)) && (0<=(j)) && ((j)<(width))) ? G2D((ptr),(width),(i),(j)) : (v0))

#define    MAX_N_CHANNEL          1

#define    MIN_EFFORT             1
#define    MAX_EFFORT             3

#define    MAX_VAL                255
#define    MID_VAL                ((MAX_VAL+1)/2)

#define    MAX_PX_INC             (MAX_VAL - MID_VAL)
#define    MIN_PX_INC             (-MAX_PX_INC)

#define    MAX_NEAR               (MAX_VAL / 26)

#define    MIN_K_STEP             3

#define    N_QD                   16
#define    N_CONTEXT              ((N_QD>>1) * 256)

#define    CTX_COEF               7
#define    CTX_SCALE              8

#define    N_QW                   32

#define    N_MAPPER               20

#define    MAX_COUNTER            256

#define    PROB_MAX               (1 << 12)

#define    FB1                    12
#define    FB2                    2
#define    FB3                    (FB1 - FB2)

#define    FIT_BASE               MID_VAL
#define    ALPHA                  5
#define    BETA                   3

#define    BIAS_INIT              (2    << FB2)
#define    BIAS_MAX               (1024 << FB2)
#define    BIAS_COEF              21

#define    GET_M(n)               (1+(n)+(n)*(n))

const static int N_LIST [MAX_EFFORT+1] = {-1, 0, 6, 10};

#define    MAX_N                  10



#define SET_ARRAY_ZERO(array,len) {   \
    int i;                            \
    for (i=0; i<(len); i++)           \
        (array)[i] = 0;               \
}                                     \


#define COPY_ARRAY(p_dst,p_src,len) { \
    int i;                            \
    for (i=0; i<(len); i++)           \
        (p_dst)[i] = (p_src)[i];      \
}                                     \



// return:
//      0 : failed
//      1 : success
static int AVPsolveAxb (int n, I64 *p_mat_A, I64 *p_vec_b) {
    int i, j, k, kk;
    I64 Akk, Aik, Akj;
    
    for (k=0; k<(n-1); k++) {
        // find main row number kk -------------------------------------
        kk = k;
        for (i=k+1; i<n; i++)
            if (ABS(G2D(p_mat_A, n, i, k)) > ABS(G2D(p_mat_A, n, kk, k)))
                kk = i;
        
        // swap row kk and k -------------------------------------------
        if (kk != k) {
            SWAP(I64, p_vec_b[k], p_vec_b[kk]);
            for (j=k; j<n; j++)
                SWAP(I64, G2D(p_mat_A, n, k, j), G2D(p_mat_A, n, kk, j));
        }
        
        // gaussian elimination ----------------------------------------
        Akk = G2D(p_mat_A, n, k, k);
        if (Akk == 0) return 0;
        for (i=k+1; i<n; i++) {
            Aik = G2D(p_mat_A, n, i, k);
            G2D(p_mat_A, n, i, k) = 0;
            if (Aik != 0) {
                for (j=k+1; j<n; j++) {
                    Akj = G2D(p_mat_A, n, k, j);
                    G2D(p_mat_A, n, i, j) -= Akj * Aik / Akk;
                }
                Akj = p_vec_b[k];
                p_vec_b[i] -= Akj * Aik / Akk;
            }
        }
    }
    
    for (k=(n-1); k>0; k--) {
        Akk = G2D(p_mat_A, n, k, k);
        if (Akk == 0) return 0;
        for (i=0; i<k; i++) {
            Aik = G2D(p_mat_A, n, i, k);
            G2D(p_mat_A, n, i, k) = 0;
            if (Aik != 0) {
                Akj = p_vec_b[k];
                p_vec_b[i] -= Akj * Aik / Akk;
            }
        }
    }
    
    return 1;
}


static void AVPgetVecN (I64 *vec_n, int n, int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s, int t) {
    if (n > 0) vec_n[0] = a;
    if (n > 1) vec_n[1] = b;
    if (n > 2) vec_n[2] = c;
    if (n > 3) vec_n[3] = d;
    if (n > 4) vec_n[4] = e;
    if (n > 5) vec_n[5] = f;
    if (n > 6) vec_n[6] = t;
    if (n > 7) vec_n[7] = h;
    if (n > 8) vec_n[8] = q;
    if (n > 9) vec_n[9] = g;
    //if (n >10) vec_n[10]= r;
    //if (n >11) vec_n[11]= s;
    
    {
        int k;
        for (k=0; k<n; k++)
            vec_n[k] -= FIT_BASE;
    }
}


static void AVPprecalcuate (int m, I64 *p_F_row, I64 *p_B_row, int width) {
    int j, k;
    
    for (j=width-1; j>=0; j--) {
        I64 *p_B  = p_B_row + (m * j);
        I64 *p_F  = p_F_row + (m * j);
        I64 *p_F2 = p_F_row + (m *(j+1));
        int ab = BETA;
        
        for (k=0; k<m; k++) {
            if (j == width-1)
                p_F[k] = 0;
            else
                p_F[k] = (p_F2[k] * (ab-1) + ab/2) / ab;
            p_F[k] += p_B[k];
            ab = ALPHA;
        }
    }
}


// return:
//      0 : failed
//      1 : success
static int AVPpredict (int n, int m, I64 *p_E, I64 *p_F, I64 *vec_n, I64 bias, I64 *p_px) {
    int k;
    
    I64  dataset [GET_M(MAX_N)];
    I64 *vec_b = dataset + 1;
    I64 *mat_A = dataset + 1 + n;
    
    for (k=1; k<m; k++)
        dataset[k] = p_E[k] + p_F[k];
    
    for (k=0; k<n; k++) {
        vec_b[k]            += bias << FB3;
        G2D(mat_A, n, k, k) += bias * n;
    }
    
    if ( AVPsolveAxb(n, mat_A, vec_b) ) {
        I64 px = FIT_BASE << FB1;
        
        for (k=0; k<n; k++) {
            I64 Akk = G2D(mat_A, n, k, k);
            px += (((vec_b[k] * vec_n[k]) << FB2) + (Akk>>1)) / Akk;
        }
        
        *p_px = CLIP(px, 0, (MAX_VAL<<FB1));
        
        return 1;
    } else {
        return 0;
    }
}


static void AVPupdate (int n, int m, I64 *p_E, I64 *p_B, I64 *vec_n, int x, I64 s_curr, I64 s_sum) {
    int j, k, ab;
    
    I64  s_sum_2;
    I64  dataset [GET_M(MAX_N)];
    I64 *vec_b = dataset + 1;
    I64 *mat_A = dataset + 1 + n;
    
    dataset[0] = s_curr;
    
    x -= FIT_BASE;
    
    s_sum = CLIP((s_sum+(1<<FB1)), (1<<FB1), (16<<FB1));
    s_sum_2 = s_sum >> 1;
    
    for (k=0; k<n; k++)
        vec_b[k]                = (((       x * vec_n[k]) << (4+FB1+FB1)) + s_sum_2) / s_sum;   // b = x * n
    
    for (j=0; j<n; j++)
        for (k=0; k<n; k++)
            G2D(mat_A, n, j, k) = (((vec_n[j] * vec_n[k]) << (4+FB2+FB1)) + s_sum_2) / s_sum;   // A = n * n.T
    
    //for (k=0; k<n; k++)
    //    vec_b[k]                = ((       x * vec_n[k]) << FB1);   // b = x * n
    //for (j=0; j<n; j++)
    //    for (k=0; k<n; k++)
    //        G2D(mat_A, n, j, k) = ((vec_n[j] * vec_n[k]) << FB2);   // A = n * n.T
    
    ab = BETA;
    
    for (k=0; k<m; k++) {
        p_B[k] *= (ab-1);
        p_B[k] += (ab>>1);
        p_B[k] /= ab;
        p_B[k] += dataset[k];
        p_E[k] *= (ab-1);
        p_E[k] += (ab>>1);
        p_E[k] /= ab;
        p_E[k] += p_B[k];
        ab = ALPHA;
    }
}



static void sampleNeighbourPixels (UI8 *p_img, int width, int i, int j, int *p_a, int *p_b, int *p_c, int *p_d, int *p_e, int *p_f, int *p_g, int *p_h, int *p_q, int *p_r, int *p_s, int *p_t) {
    *p_a = (int)SPIX(p_img, width, i   , j-1 , MID_VAL);
    *p_b = (int)SPIX(p_img, width, i-1 , j   , MID_VAL);
    if      (i == 0)
        *p_b = *p_a;
    else if (j == 0)
        *p_a = *p_b;
    *p_e = (int)SPIX(p_img, width, i   , j-2 , *p_a);
    *p_c = (int)SPIX(p_img, width, i-1 , j-1 , *p_b);
    *p_d = (int)SPIX(p_img, width, i-1 , j+1 , *p_b);
    *p_f = (int)SPIX(p_img, width, i-2 , j   , *p_b);
    *p_g = (int)SPIX(p_img, width, i-2 , j+1 , *p_f);
    *p_h = (int)SPIX(p_img, width, i-2 , j-1 , *p_f);
    *p_q = (int)SPIX(p_img, width, i-1 , j-2 , *p_c);
    *p_r = (int)SPIX(p_img, width, i-2 , j+2 , *p_g);
    *p_s = (int)SPIX(p_img, width, i-2 , j-2 , *p_h);
    *p_t = (int)SPIX(p_img, width, i-1 , j+2 , *p_d);
}


static int simplePredict (int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s) {
    const static int c_thresholds [] = {1*(MAX_VAL/8), 3*(MAX_VAL/8), 9*(MAX_VAL/8), 20*(MAX_VAL/8), 50*(MAX_VAL/8), 110*(MAX_VAL/8), 300*(MAX_VAL/8), 800*(MAX_VAL/8)};
    
    int px_lnr, px_ang=0, cost, csum=0, cmin=0xFFFFFF, wt;
    
    px_lnr = CLIP((9*a+9*b+2*d-2*c-e-f), 0, 16*MAX_VAL);
    
    cost = 2 * (ABS(a-e) + ABS(c-q) + ABS(b-c) + ABS(d-b));
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = 2 * a;
    }
    
    cost = 2 * (ABS(a-c) + ABS(c-h) + ABS(b-f) + ABS(d-g));
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = 2 * b;
    }
    
    cost = 2 * (ABS(a-q) + ABS(c-s) + ABS(b-h) + ABS(d-f));
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = 2 * c;
    }
    
    cost = 2 * (ABS(a-b) + ABS(c-f) + ABS(b-g) + ABS(d-r));
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = 2 * d;
    }
    
    cost = ABS(2*a-e-q) + ABS(2*c-q-s) + ABS(2*b-c-h) + ABS(2*d-b-f);
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = a + c;
    }
    
    cost = ABS(2*a-q-c) + ABS(2*c-s-h) + ABS(2*b-h-f) + ABS(2*d-f-g);
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = c + b;
    }
    
    cost = ABS(2*a-c-b) + ABS(2*c-h-f) + ABS(2*b-f-g) + ABS(2*d-g-r);
    csum += cost;
    if (cmin > cost) {
        cmin = cost;
        px_ang = b + d;
    }
    
    csum -= (7 * cmin);
    
    for (wt=0; wt<8; wt++)
        if (c_thresholds[wt] > csum)
            break;
    
    return ((8*wt*px_ang + (8-wt)*px_lnr + 64) >> 7);
}


static void getQuantizedDelta (int a, int b, int c, int d, int e, int f, int g, int err, int *p_qu, int *p_qv, int *p_qw) {
    const static int q_mid [] = {0, 2, 4, 7, 10, 14, 20, 26, 34, 42, 52, 64, 78, 95, 135, 200};
    
    int delta = ABS(a-e) + ABS(b-c) + ABS(b-d) + ABS(a-c) + ABS(b-f) + ABS(d-g) + 2*ABS(err);
    int qd;
    
    for (qd=0; qd<(N_QD-1); qd++)
        if (delta <= q_mid[qd])
            break;
    
    *p_qu = *p_qv = qd;
    *p_qw = 0;
    
    if (delta < q_mid[qd]) {
        *p_qw = N_QW * (delta - q_mid[qd-1]) / (q_mid[qd] - q_mid[qd-1]);
        if ((*p_qw) < (N_QW/2)) {
            *p_qu = qd - 1;
        } else {
            *p_qv = qd - 1;
            *p_qw = N_QW - *p_qw;
        }
    }
}


static int getContextAddress (int a, int b, int c, int d, int e, int f, int qu, int px) {
    qu >>= 1;
    qu <<= 8;
    qu |= ((px > a)       ? 0x01 : 0);
    qu |= ((px > b)       ? 0x02 : 0);
    qu |= ((px > c)       ? 0x04 : 0);
    qu |= ((px > d)       ? 0x08 : 0);
    qu |= ((px > e)       ? 0x10 : 0);
    qu |= ((px > f)       ? 0x20 : 0);
    qu |= ((px > (2*a-e)) ? 0x40 : 0);
    qu |= ((px > (2*b-f)) ? 0x80 : 0);
    return qu;
}


static int correctPxByContext (int ctx_item, int px, int *p_sign) {
    int px_inc;
    *p_sign = (ctx_item >> (CTX_SCALE-1)) & 1;
    px_inc  = (ctx_item >> CTX_SCALE) + *p_sign;
    return CLIP(px+px_inc, 0, MAX_VAL);
}


static void updateContext (int *p_ctx_item, int err) {
    int v = (*p_ctx_item);
    v  *= ((1<<CTX_COEF)-1);
    v  += (err << CTX_SCALE);
    v  += (1 << (CTX_COEF-1));
    v >>= CTX_COEF;
    (*p_ctx_item) = v;
}


static int mapXtoY (int x, int px, int sign, int near) {
    const int ty = (CLIP(px, 0, MAX_VAL - px) + near) / (2*near + 1);
    int sy = (x >= px) ? 1 : 0;
    int y  = ABS(x - px);
    
    y = (y + near) / (2*near + 1);
    
    if (y <= 0)
        return 0;
    else if (y <= ty)
        return 2*y - (sy^sign);
    else
        return y + ty;
}


static int mapYtoX (int z, int px, int sign, int near) {
    const int ty = (CLIP(px, 0, MAX_VAL - px) + near) / (2*near + 1);
    int y, sy;
    
    if (z <= 0) {
        y  = 0;
        sy = 0;
    } else if (z <= 2*ty) {
        y  = (z + 1) / 2;
        sy = (z & 1) ^ sign;
    } else {
        y  = z - ty;
        sy = (px < MID_VAL) ? 1 : 0;
    }
    
    y *= (2*near + 1);
    y = px + (sy ? y : -y);
    
    return CLIP(y, 0, MAX_VAL);
}



typedef struct {
    UI8 y2z  [N_MAPPER];
    UI8 z2y  [N_MAPPER];
    int hist [N_MAPPER];
} AutoMapper_t;


static void initAutoMapper (AutoMapper_t *p_map) {
    int i;
    for (i=0; i<N_MAPPER; i++) {
        p_map->y2z[i] = (UI8)i;
        p_map->z2y[i] = (UI8)i;
        p_map->hist[i] = (N_MAPPER - 1 - i) * 2;
    }
}


static int mapYtoZ (AutoMapper_t *p_map, int y) {
    return (y < N_MAPPER) ? p_map->y2z[y] : y;
}


static int mapZtoY (AutoMapper_t *p_map, int z) {
    return (z < N_MAPPER) ? p_map->z2y[z] : z;
}


static void addY (AutoMapper_t *p_map, int y) {
    if (y < N_MAPPER) {
        UI8 z, z2, y2;
        int h, h2;
        
        z = p_map->y2z[y];
        
        p_map->hist[z] ++;
        
        if (z > 0) {
            z2 = z - 1;
            y2 = p_map->z2y[z2];
            
            h  = p_map->hist[z];
            h2 = p_map->hist[z2];
            
            if (h2 < h) {
                p_map->hist[z ] = h2;
                p_map->hist[z2] = h;
                p_map->z2y [z ] = y2;
                p_map->z2y [z2] = (UI8)y;
                p_map->y2z [y ] = z2;
                p_map->y2z [y2] = z;
            }
        }
    }
}



typedef struct {
    UI8 *p_buf;
    U32  v1;      // Range, initially [0, 1), scaled by 2^32
    U32  v2;
    U32  v;       // last 4 input bytes of compressed stream (only for decode)
    UI8  decode;  // 1:decode    0:encode
} CODEC_t;


static CODEC_t newCodec (int decode, UI8 *p_buf) {
    CODEC_t codec = {NULL, 0, 0xFFFFFFFF, 0, 0};
    codec.decode  = (UI8)decode;
    codec.p_buf   = p_buf;
    
    if (decode) {    // for decode, let v = first 4 bytes of compressed stream
        codec.v = (codec.v<<8) + (*(codec.p_buf++));
        codec.v = (codec.v<<8) + (*(codec.p_buf++));
        codec.v = (codec.v<<8) + (*(codec.p_buf++));
        codec.v = (codec.v<<8) + (*(codec.p_buf++));
    }
    
    return codec;
}


static void binCodec (CODEC_t *p_co, int *p_bin, U32 prob) {
    U32 vm = p_co->v1 + ((p_co->v2-p_co->v1)>>12)*prob + (((p_co->v2-p_co->v1)&0xfff)*prob>>12);
    
    if (p_co->decode)
        *p_bin = (p_co->v <= vm) ? 1 : 0;
    
    if (*p_bin)
        p_co->v2 = vm;
    else
        p_co->v1 = vm + 1;
    
    while (((p_co->v1^p_co->v2)&0xff000000) == 0) {
        p_co->v <<= 8;
        if (p_co->decode)
            p_co->v += (*(p_co->p_buf++));                // read byte from compressed stream
        else
            (*(p_co->p_buf++)) = (uint8_t)(p_co->v2>>24); // write byte to compressed stream
        p_co->v1 <<= 8;
        p_co->v2 <<= 8;
        p_co->v2  += 0xFF;
    }
}


static void flushEncoder (CODEC_t *p_co) {
    if (!p_co->decode) {
        (*(p_co->p_buf++)) = (uint8_t)(p_co->v1>>24);
        p_co->v1 <<= 8;
        (*(p_co->p_buf++)) = (uint8_t)(p_co->v1>>24);
        p_co->v1 <<= 8;
        (*(p_co->p_buf++)) = (uint8_t)(p_co->v1>>24);
        p_co->v1 <<= 8;
        (*(p_co->p_buf++)) = (uint8_t)(p_co->v1>>24);
    }
}


typedef struct {
    int c0;
    int c1;
} BIN_CNT_t;


static void initBinCounterTree (BIN_CNT_t bc_tree [][256]) {
    int i, j;
    for (i=0; i<N_QD; i++) {
        for (j=0; j<256; j++) {
            bc_tree[i][j].c0 = N_QW;
            bc_tree[i][j].c1 = N_QW;
        }
    }
}


static void counterUpdate (BIN_CNT_t *p_bc, int bin, int qw) {
    if (bin)
        p_bc->c1 += qw;
    else
        p_bc->c0 += qw;
    
    if ((p_bc->c0 + p_bc->c1) > (N_QW * MAX_COUNTER)) {
        p_bc->c0 ++;
        p_bc->c0 >>= 1;
        p_bc->c1 ++;
        p_bc->c1 >>= 1;
    }
}


static int getProb1 (BIN_CNT_t *p_bc) {
    int c0 = p_bc->c0;
    int c1 = p_bc->c1;
    return (PROB_MAX * c1) / (c0 + c1);
}


static void AriCodec (CODEC_t *p_co, BIN_CNT_t *p_ubc, BIN_CNT_t *p_vbc, int qw, int *p_bin) {
    int prob = (getProb1(p_ubc) * (N_QW-qw) + getProb1(p_vbc) * qw + N_QW/2) / N_QW;
    
    prob = CLIP(prob, 1, (PROB_MAX-1));
    
    binCodec(p_co, p_bin, (U32)prob);
    
    counterUpdate(p_ubc, *p_bin, N_QW-qw);
    counterUpdate(p_vbc, *p_bin, qw);
}


static void Zcodec (CODEC_t *p_co, int k_step, BIN_CNT_t bc_tree [][256], int qu, int qv, int qw, int *p_z) {
    const int k_max = (N_QD-1) / k_step;
    int i, k, bin;
    
    if ((qv / k_step) != (qu / k_step))
        qv = qu;
    
    for (i=0; ; ) {
        k = qu / k_step;
        
        if (!p_co->decode)
            bin = (i >> k_max) < ((*p_z) >> k);
        
        AriCodec(p_co, &bc_tree[qu][i], &bc_tree[qv][i], qw, &bin);
        
        if (!bin)
            break;
        
        i += (1 << k_max);
        if (i >= 256) {
            i >>= 1;
            qv = qu = (k + 1) * k_step;
        }
    }
    
    if (p_co->decode)
        (*p_z) = ((i >> k_max) << k);
    
    for (i++, k--; k>=0; k--) {
        if (!p_co->decode)
            bin = ((*p_z) >> k) & 1;
        
        AriCodec(p_co, &bc_tree[qu][i], &bc_tree[qv][i], qw, &bin);
        
        if (p_co->decode)
            (*p_z) += bin ? (1<<k) : 0;
        
        i += bin ? (1<<k) : 1;
    }
}


static void putHeader (UI8 **pp_buf, int n_channel, int height, int width, int near, int k_step, int effort) {
    int i;
    for (i=0; title[i]!=0; i++)                 // put title
        *((*pp_buf)++) = (UI8)title[i];
    *((*pp_buf)++) = (UI8)n_channel;            // put n_channel
    *((*pp_buf)++) = (UI8)(height >> 8);        // put image height 
    *((*pp_buf)++) = (UI8)(height >> 0);
    *((*pp_buf)++) = (UI8)(width >> 8);         // put image width 
    *((*pp_buf)++) = (UI8)(width >> 0);
    *((*pp_buf)++) = (UI8)near;                 // put near
    *((*pp_buf)++) = (UI8)k_step;               // put k_step
    *((*pp_buf)++) = (UI8)effort;               // put effort
}


// return:  -1:failed  0:success
static int getHeader (UI8 **pp_buf, int *p_n_channel, int *p_height, int *p_width, int *p_near, int *p_k_step, int *p_effort) {
    int i;
    for (i=0; title[i]; i++)                    // check title 
        if ( *((*pp_buf)++) != (UI8)title[i] )
            return -1;
    *p_n_channel =   *((*pp_buf)++);            // get n_channel
    *p_height    = ( *((*pp_buf)++) ) << 8;     // get image height
    *p_height   +=   *((*pp_buf)++);
    *p_width     = ( *((*pp_buf)++) ) << 8;     // get image width
    *p_width    +=   *((*pp_buf)++);
    *p_near      =   *((*pp_buf)++);            // get near
    *p_k_step    =   *((*pp_buf)++);            // get k_step
    *p_effort    =   *((*pp_buf)++);            // get effort
    return 0;
}

    

// return:  -1:failed  0:success
static int checkSize (int height, int width) {
    if (height <= 0)
        return -1;
    if (width  <= 0)
        return -1;
    if (height > NBLIC_MAX_HEIGHT)
        return -1;
    if (width  > NBLIC_MAX_WIDTH)
        return -1;
    if ((height*width) > NBLIC_MAX_IMG_SIZE)
        return -1;
    return 0;
}


// return:  -1:failed  0:success
static int checkParam (int height, int width, int n_channel, int near, int k_step, int effort) {
    if (checkSize(height, width))
        return -1;
    if (n_channel < 0 || n_channel > MAX_N_CHANNEL)
        return -1;
    if (near < 0 || near > MAX_NEAR)
        return -1;
    if (k_step < MIN_K_STEP || k_step > N_QD)
        return -1;
    if (effort < MIN_EFFORT || effort > MAX_EFFORT)
        return -1;
    return 0;
}



static int NBLICcodec (int verbose, int decode, UI8 *p_buf, UI8 *p_img, UI8 *p_img_out, int *p_height, int *p_width, int *p_near, int *p_effort) {
    int n_channel=1, n, m, avp_enable, k_step, i, j;
    
    int ctx_array [N_CONTEXT];
    
    BIN_CNT_t bc_tree [N_QD][256];
    
    AutoMapper_t maps [256][2];
    
    CODEC_t codec;
    
    UI8 *p_buf_base = p_buf;
    
    I64 *p_B_row=NULL, *p_F_row=NULL, *p_B=NULL, *p_F=NULL, p_E[GET_M(MAX_N)], vec_n[MAX_N], bias=BIAS_INIT;
    
    if (decode) {
        if (getHeader(&p_buf, &n_channel, p_height, p_width, p_near, &k_step, p_effort))
            return -1;
    } else {
        *p_near   = CLIP(*p_near, 0, MAX_NEAR);
        k_step    = CLIP(MIN_K_STEP+2*(*p_near), MIN_K_STEP, N_QD);
        *p_effort = CLIP(*p_effort, MIN_EFFORT, MAX_EFFORT);
        putHeader(&p_buf, n_channel, *p_height, *p_width, *p_near, k_step, *p_effort);
    }
    
    if (checkParam(*p_height, *p_width, n_channel, *p_near, k_step, *p_effort))
        return -1;
    
    
    n = N_LIST[ (*p_effort) ];
    m = GET_M(n);
    avp_enable = (n > 0) ? 1 : 0;
    
    
    if (avp_enable) {
        p_B_row = (I64*)malloc((*p_width) * m * 2 * sizeof(I64));
        
        if (p_B_row == NULL)
            return -1;
        
        SET_ARRAY_ZERO(p_B_row, (*p_width) * m);
        
        p_F_row = p_B_row + (*p_width) * m;
    }
    
    
    codec = newCodec(decode, p_buf);
    
    SET_ARRAY_ZERO(ctx_array, N_CONTEXT);
    
    initBinCounterTree(bc_tree);
    
    for (i=0; i<256; i++) {
        initAutoMapper(&maps[i][0]);
        initAutoMapper(&maps[i][1]);
    }
    
    
    for (i=0; i<(*p_height); i++) {
        int err = 0;
        
        if (verbose) {
            if ((i&0x7) == 0) {
                printf("\r    effort=%d, %s row %d (%.2lf%%)" , (*p_effort), (decode?"decoding":"encoding"), i, (100.0*i)/(*p_height));
                fflush(stdout);
            }
        }
        
        if (avp_enable) {
            SET_ARRAY_ZERO(p_E, m);
            AVPprecalcuate(m, p_F_row, p_B_row, (*p_width));
        }
        
        for (j=0; j<(*p_width); j++) {
            int a, b, c, d, e, f, g, h, q, r, s, t;
            int px1_vld=0, px2_vld=0;
            I64 bias1=0, bias2=0, px1f=0, px2f=0;
            int px0, px;
            int qu, qv, qw, adr, sign, x, y=0, z=0;
            
            sampleNeighbourPixels(p_img_out, (*p_width), i, j, &a, &b, &c, &d, &e, &f, &g, &h, &q, &r, &s, &t);
            
            if (avp_enable) {
                AVPgetVecN(vec_n, n, a, b, c, d, e, f, g, h, q, r, s, t);
                
                p_B = p_B_row + (m * j);
                p_F = p_F_row + (m * j);
                
                bias1 = bias * BIAS_COEF / (BIAS_COEF+1);
                bias2 = bias * (BIAS_COEF+1) / BIAS_COEF;
                bias1 = CLIP(bias1, -1, bias-1);
                bias2 = CLIP(bias2, bias+1, BIAS_MAX+1);
                bias1 = CLIP(bias1, 0, BIAS_MAX);
                bias2 = CLIP(bias2, 0, BIAS_MAX);
                
                px1_vld = AVPpredict(n, m, p_E, p_F, vec_n, bias1, &px1f);
                px2_vld = AVPpredict(n, m, p_E, p_F, vec_n, bias2, &px2f);
            }
            
            if (px1_vld) {
                px0 = (int)((px1f + (1<<FB1>>1)) >> FB1);
            } else {
                px0 = simplePredict(a, b, c, d, e, f, g, h, q, r, s);
                px1f = px0 << FB1;
            }
            
            getQuantizedDelta(a, b, c, d, e, f, g, err, &qu, &qv, &qw);
            
            adr = getContextAddress(a, b, c, d, e, f, qu, px0);
            
            px = correctPxByContext(ctx_array[adr], px0, &sign);
            
            if (!decode) {
                x = G2D(p_img, *p_width, i, j);
                y = mapXtoY(x, px, sign, *p_near);
                z = mapYtoZ(&maps[px][sign], y);
            }
            
            Zcodec(&codec, k_step, bc_tree, qu, qv, qw, &z);
            
            if (decode)
                y = mapZtoY(&maps[px][sign], z);
            
            addY(&maps[px][sign], y);
            
            x = mapYtoX(y, px, sign, *p_near);
            
            G2D(p_img_out, (*p_width), i, j) = (UI8)x;
            
            err = CLIP((x-px0), MIN_PX_INC, MAX_PX_INC);
            
            updateContext(&ctx_array[adr], err);
            
            if (avp_enable) {
                I64 s_curr = ABS(px1f - (x<<FB1));
                I64 s_sum  = (p_E[0] + p_F[0]) + (s_curr * BETA / (BETA-1));
                
                AVPupdate(n, m, p_E, p_B, vec_n, x, s_curr, s_sum);
                
                if (px1_vld && px2_vld) {
                    px1f = ABS(px1f - (x<<FB1));
                    px2f = ABS(px2f - (x<<FB1));
                    bias = (px1f > px2f) ? bias2 : bias1;
                }
            }
        }
    }
    
    if (verbose)
        printf("\r                                                                        \r");
    
    free(p_B_row);
    
    flushEncoder(&codec);
    
    if (decode)
        return 0;
    else
        return codec.p_buf - p_buf_base;
}



// return :
//    positive value : compressed stream length
//                -1 : failed
int NBLICcompress (int verbose, UI8 *p_buf, UI8 *p_img, int height, int width, int *p_near, int *p_effort) {
    return NBLICcodec(verbose, 0, p_buf, p_img, p_img, &height, &width, p_near, p_effort);
}



// return :
//                 0 : success
//                -1 : failed
int NBLICdecompress (int verbose, UI8 *p_buf, UI8 *p_img, int *p_height, int *p_width, int *p_near, int *p_effort) {
    return NBLICcodec(verbose, 1, p_buf, NULL, p_img, p_height, p_width, p_near, p_effort);
}
