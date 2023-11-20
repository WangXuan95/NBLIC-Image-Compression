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


#include "NBLIC.h"

const char *title = "NBLIC0.1";

typedef    unsigned char          UI8;
typedef    long long int          I64;


#define    SWAP(type,a,b)         {type t; (t)=(a); (a)=(b); (b)=(t);}

#define    ABS(x)                 ( ((x) < 0) ? (-(x)) : (x) )                             // get absolute value
#define    CLIP(x,a,b)            ( ((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)) )            // clip x between a~b

#define    GET2D(ptr,width,i,j)   (*( (ptr) + (width)*(i) + (j) ))
#define    SPIX(ptr,width,i,j,v0) (((0<=(i)) && (0<=(j)) && ((j)<(width))) ? GET2D((ptr),(width),(i),(j)) : (v0))

#define    MAX_N_CHANNEL          1

#define    MAX_VAL                255
#define    MID_VAL                ((MAX_VAL+1)/2)

#define    MAX_BIAS               (MAX_VAL - MID_VAL)
#define    MIN_BIAS               (-MAX_BIAS)

#define    MAX_NEAR               (MAX_VAL / 10)

#define    N_QD                   16
#define    N_CONTEXT              ((N_QD>>1) * 256)

#define    CTX_COEF               7
#define    CTX_SCALE              8

#define    N_QW                   32

#define    N_MAPPER               20

#define    MAX_COUNTER            256
#define    PROB_ONE               (1 << 16)

#define    EDP_I                  5
#define    EDP_N                  6
#define    EDP_T1                 5
#define    EDP_T2                 (EDP_T1 - 1 + EDP_I)
#define    EDP_T3                 6
#define    EDP_M                  (EDP_T1 + (1 + EDP_T1 + EDP_T2) * EDP_T3)
#define    EDP_SFT                12



static void matrixMultiply (I64 *p_dst, I64 *p_src1, I64 *p_src2, int m, int n1, int n2) {
    int i, j, k;
    for (i=0; i<n1; i++) {
        for (j=0; j<n2; j++) {
            I64 s = 0;
            for (k=0; k<m; k++)
                s += GET2D(p_src1, n1, k, i) * GET2D(p_src2, n2, k, j);
            GET2D(p_dst, n2, i, j) = s;
        }
    }
}


// return:
//      1 : failed
//      0 : success
static int Solve_Ax_b (I64 *p_mat_A, I64 *p_vec_b, int n) {
    int k, i, j;
    I64 Akk, Aik, Akj;
    
    for (k=0; k<(n-1); k++) {
        // find main row number kk -------------------------------------
        int kk = k;
        for (i=k+1; i<n; i++)
            if (ABS(GET2D(p_mat_A, n, i, k)) > ABS(GET2D(p_mat_A, n, kk, k)))
                kk = i;
        
        // swap row kk and k -------------------------------------
        if (kk != k) {
            SWAP(I64, p_vec_b[k], p_vec_b[kk]);
            for (j=k; j<n; j++)
                SWAP(I64, GET2D(p_mat_A, n, k, j), GET2D(p_mat_A, n, kk, j));
        }
        
        // gaussian elimination -------------------------------------
        Akk = GET2D(p_mat_A, n, k, k);
        if (Akk == 0)
            return 1;
        for (i=k+1; i<n; i++) {
            Aik = GET2D(p_mat_A, n, i, k);
            GET2D(p_mat_A, n, i, k) = 0;
            if (Aik != 0) {
                for (j=k+1; j<n; j++) {
                    Akj = GET2D(p_mat_A, n, k, j);
                    GET2D(p_mat_A, n, i, j) -= Akj * Aik / Akk;
                }
                Akj = p_vec_b[k];
                p_vec_b[i] -= Akj * Aik / Akk;
            }
        }
    }
    
    for (k=(n-1); k>0; k--) {
        Akk = GET2D(p_mat_A, n, k, k);
        if (Akk == 0)
            return 1;
        for (i=0; i<k; i++) {
            Aik = GET2D(p_mat_A, n, i, k);
            GET2D(p_mat_A, n, i, k) = 0;
            if (Aik != 0) {
                Akj = p_vec_b[k];
                p_vec_b[i] -= Akj * Aik / Akk;
            }
        }
    }
    
    for (k=0; k<n; k++) {
        Akk = GET2D(p_mat_A, n, k, k);
        if (Akk == 0)
            return 1;
        p_vec_b[k] += (Akk>>1);
        p_vec_b[k] /= Akk;
    }
    
    return 0;
}


static void sampleNeighbourPixels (UI8 *p_img, int width, int i, int j, int *p_a, int *p_b, int *p_c, int *p_d, int *p_e, int *p_f, int *p_g, int *p_h, int *p_q, int *p_r, int *p_s) {
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
}


static void getVecBforEDP (I64 *p_vec, int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s) {
    int ii;
    if (EDP_N > 0) p_vec[0] = a;
    if (EDP_N > 1) p_vec[1] = b;
    if (EDP_N > 2) p_vec[2] = c;
    if (EDP_N > 3) p_vec[3] = d;
    if (EDP_N > 4) p_vec[4] = e;
    if (EDP_N > 5) p_vec[5] = f;
    if (EDP_N > 6) p_vec[6] = g;
    if (EDP_N > 7) p_vec[7] = q;
    if (EDP_N > 8) p_vec[8] = h;
    if (EDP_N > 9) p_vec[9] = r;
    if (EDP_N >10) p_vec[10]= s;
    for (ii=0; ii<EDP_N; ii++)
        p_vec[ii] -= MID_VAL;
}


// return:
//      1 : failed
//      0 : success
static int fitEDP (I64 p_vec_r [EDP_N], UI8 *p_img, int width, int i, int j) {
    int  a, b, c, d, e, f, g, h, q, r, s;
    int ii, jj, m=0;
    
    I64 mat_C [EDP_M * EDP_N];                                // (M*N) matrix
    I64 vec_y [EDP_M        ];                                // (M*1) matrix, i.e. a column vector
    I64 mat_R [EDP_N * EDP_N];                                // (N*N) matrix
    
    for (ii=i-EDP_T3; ii<=i; ii++) {
        for (jj=j-EDP_T1; jj<=j+EDP_T2; jj++) {
            if (ii==i && jj==j)
                break;
            sampleNeighbourPixels(p_img, width, ii, jj, &a, &b, &c, &d, &e, &f, &g, &h, &q, &r, &s);
            getVecBforEDP(&GET2D(mat_C, EDP_N, m, 0), a, b, c, d, e, f, g, h, q, r, s);
            vec_y[m++] = (I64)SPIX(p_img, width, ii, jj, MID_VAL) - MID_VAL;
        }
    }
    
    matrixMultiply(mat_R, mat_C, mat_C, EDP_M, EDP_N, EDP_N); // R = C.T * T
    
    matrixMultiply(p_vec_r, mat_C, vec_y, EDP_M, EDP_N, 1);   // r = C.T * y
    
    for (ii=(EDP_N-1); ii>=0; ii--)
        p_vec_r[ii] <<= EDP_SFT;
    
    return Solve_Ax_b(mat_R, p_vec_r, EDP_N);                 // R * a = r , solve a, put a to vec_r
}


static int doEDP (I64 p_vec_r [EDP_N], int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s) {
    I64 px;
    I64 vec_b [EDP_N];                                        // (N*1) matrix, i.e. a column vector
    
    getVecBforEDP(vec_b, a, b, c, d, e, f, g, h, q, r, s);
    
    matrixMultiply(&px, p_vec_r, vec_b, EDP_N, 1, 1);         // px = a · b
    
    px = ((px + (1<<EDP_SFT>>1)) >> EDP_SFT);
    
    return (int)CLIP(px+MID_VAL, 0, MAX_VAL);
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


static int correctPxByContext (int ctx_item, int *p_px) {
    int sign = (ctx_item >> (CTX_SCALE-1)) & 1;
    int bias = (ctx_item >> CTX_SCALE) + sign;
    (*p_px) = CLIP((*p_px)+bias, 0, MAX_VAL);
    return sign;
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
    int  creg;
    int  areg;
    UI8  buf0;
    UI8  buf1;
    UI8  decode;
} CODEC_t;


static CODEC_t newCodec (int decode, UI8 *p_buf) {
    CODEC_t codec = {0, 0, 0xff*0xff, 0, 0, 0};
    codec.decode  = (UI8)decode;
    codec.p_buf   = p_buf;
    if (decode) {
        codec.p_buf+= 2;
        codec.creg  = ( *(codec.p_buf++) ) * 0xff;
        codec.creg +=   *(codec.p_buf++);
    }
    return codec;
}


static void normEncoder (CODEC_t *p_co) {
    if (!p_co->decode) {
        if (p_co->creg >= 0xff*0xff) {
            p_co->creg -= 0xff*0xff;
            p_co->buf0 ++;
            if (p_co->buf0 == 0xff) {
                p_co->buf0 = 0;
                p_co->buf1 ++;
            }
        }
        *(p_co->p_buf++) = p_co->buf1;
        p_co->buf1 = p_co->buf0;
        p_co->buf0 = (UI8)( ((p_co->creg >> 8) + p_co->creg + 1) >> 8 );
        p_co->creg += p_co->buf0;
        p_co->creg &= 0xff;
        p_co->creg = (p_co->creg << 8) - p_co->creg;
    }
}


static void flushEncoder (CODEC_t *p_co) {
    normEncoder(p_co);
    normEncoder(p_co);
    normEncoder(p_co);
    normEncoder(p_co);
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


static void AriUpdateCounter (BIN_CNT_t *p_bc, int bin, int qw) {
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


static int getProb0 (BIN_CNT_t *p_bc) {
    int c0 = p_bc->c0;
    int c1 = p_bc->c1;
    return (PROB_ONE * c0) / (c0 + c1);
}


static int AriGetAvd (int areg, int prob) {
    const static int table_th [30] = {0x7800, 0x7000, 0x6800, 0x6000, 0x5800, 0x5000, 0x4800, 0x4000, 0x3c00, 0x3800, 0x3400, 0x3000, 0x2c00, 0x2800, 0x2400, 0x2000, 0x1c00, 0x1800, 0x1400, 0x1000, 0x0e00, 0x0c00, 0x0a00, 0x0800, 0x0600, 0x0400, 0x0300, 0x0200, 0x0180, 0x0101};
    const static int table_av [31] = {0x7ab6, 0x7068, 0x6678, 0x5ce2, 0x53a6, 0x4ac0, 0x4230, 0x39f4, 0x33fc, 0x301a, 0x2c4c, 0x2892, 0x24ea, 0x2156, 0x1dd6, 0x1a66, 0x170a, 0x13c0, 0x1086, 0x0d60, 0x0b0e, 0x0986, 0x0804, 0x0686, 0x050a, 0x0394, 0x027e, 0x01c6, 0x013e, 0x0100, 0x0002};
    int i, hd, av;
    
    for (i=0; i<30; i++)
        if (table_th[i] < prob)
            break;
    
    av = table_av[i];
    
    for (hd=0x8000; hd>areg; hd>>=1)
        if (av > 0x0002)
            av >>= 1;
    
    av = areg - av;
    
    if (av < hd)
        av = (av+hd) / 2;
    
    return av;
}


static void AriCodec (CODEC_t *p_co, BIN_CNT_t *p_ubc, BIN_CNT_t *p_vbc, int qw, int *p_bin) {
    int prob = (getProb0(p_ubc) * (N_QW-qw) + getProb0(p_vbc) * qw + N_QW/2) / N_QW;
    int mps  = (prob < PROB_ONE/2) ? 1 : 0;
    int av   = AriGetAvd(p_co->areg, (mps ? prob : (PROB_ONE-prob)));
    
    if (p_co->decode)
        *p_bin = (p_co->creg < av) ? mps : !mps;
    
    if ((*p_bin) == mps) {
        p_co->areg  = av;
    } else {
        p_co->areg -= av;
        p_co->creg += p_co->decode ? -av : av;
    }
    
    AriUpdateCounter(p_ubc, *p_bin, N_QW-qw);
    AriUpdateCounter(p_vbc, *p_bin, qw);
    
    if (p_co->areg <= 0xff) {
        if (p_co->decode)
            p_co->creg = (p_co->creg << 8) - p_co->creg + (*(p_co->p_buf++));
        
        normEncoder(p_co);
        
        p_co->areg = (p_co->areg << 8) - p_co->areg;
    }
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
static int checkParam (int height, int width, int n_channel, int near, int k_step, int effort) {
    if (height < 0 || height > NBLIC_MAX_HEIGHT)
        return -1;
    if (width < 0 || width > NBLIC_MAX_WIDTH)
        return -1;
    if (n_channel < 0 || n_channel > MAX_N_CHANNEL)
        return -1;
    if (near < 0 || near > MAX_NEAR)
        return -1;
    if (k_step < 3 || k_step > N_QD)
        return -1;
    if (effort < MIN_EFFORT || effort > MAX_EFFORT)
        return -1;
    return 0;
}



int NBLICcodec (int decode, UI8 *p_buf, UI8 *p_img, int *p_height, int *p_width, int *p_near, int *p_effort) {
    int n_channel=1, i, j, k_step;
    
    int ctx_array [N_CONTEXT];
    
    BIN_CNT_t bc_tree [N_QD][256];
    
    AutoMapper_t maps [256][2];
    
    CODEC_t codec;
    
    UI8 *p_buf_base = p_buf;
    
    if (decode) {
        if (getHeader(&p_buf, &n_channel, p_height, p_width, p_near, &k_step, p_effort))
            return -1;
    } else {
        *p_near   = CLIP(*p_near, 0, MAX_NEAR);
        k_step    = CLIP(3+2*(*p_near), 3, N_QD);
        *p_effort = CLIP(*p_effort, MIN_EFFORT, MAX_EFFORT);
        putHeader(&p_buf, n_channel, *p_height, *p_width, *p_near, k_step, *p_effort);
    }
    
    if (checkParam(*p_height, *p_width, n_channel, *p_near, k_step, *p_effort))
        return -1;
    
    codec = newCodec(decode, p_buf);
    
    for (i=0; i<N_CONTEXT; i++)
        ctx_array[i] = 0;
    
    initBinCounterTree(bc_tree);
    
    for (i=0; i<256; i++) {
        initAutoMapper(&maps[i][0]);
        initAutoMapper(&maps[i][1]);
    }
    
    for (i=0; i<(*p_height); i++) {
        int err0 = 0;
        
        int fit_okay = 0;
        I64 vec_r [EDP_N];
        
        for (j=0; j<(*p_width); j++) {
            int a, b, c, d, e, f, g, h, q, r, s;
            int px, qu, qv, qw, adr, sign, x, y=0, z=0;
            
            sampleNeighbourPixels(p_img, (*p_width), i, j, &a, &b, &c, &d, &e, &f, &g, &h, &q, &r, &s);
            
            if (*p_effort == MAX_EFFORT)
                if ((j%EDP_I) == 0)
                    fit_okay = !fitEDP(vec_r, p_img, (*p_width), i, j);
            
            if (fit_okay)
                px = doEDP(vec_r, a, b, c, d, e, f, g, h, q, r, s);
            else
                px = simplePredict(a, b, c, d, e, f, g, h, q, r, s);
            
            getQuantizedDelta(a, b, c, d, e, f, g, err0, &qu, &qv, &qw);
            
            err0 = px;
            
            adr = getContextAddress(a, b, c, d, e, f, qu, px);
            
            sign = correctPxByContext(ctx_array[adr], &px);
            
            if (!decode) {
                x = GET2D(p_img, *p_width, i, j);
                y = mapXtoY(x, px, sign, *p_near);
                z = mapYtoZ(&maps[px][sign], y);
            }
            
            Zcodec(&codec, k_step, bc_tree, qu, qv, qw, &z);
            
            if (decode)
                y = mapZtoY(&maps[px][sign], z);
            
            addY(&maps[px][sign], y);
            
            x = mapYtoX(y, px, sign, *p_near);
            
            GET2D(p_img, (*p_width), i, j) = (UI8)x;
            
            err0 = CLIP((x-err0), MIN_BIAS, MAX_BIAS);
            
            updateContext(&ctx_array[adr], err0);
        }
    }
    
    flushEncoder(&codec);
    
    if (decode)
        return 0;
    else
        return codec.p_buf - p_buf_base;
}

