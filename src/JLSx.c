// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A enhanced implementation of JPEG-LS extension (ITU-T T.870) image compressor/decompressor
// which will get a better compression ratio than original JPEG-LS extension,
// and also, significantly better than JPEG-LS baseline (ITU-T T.87)
//
// It now supports lossless & lossy compression of 8-bit gray images
//
// for standard documents, see:
//    JPEG-LS baseline  (ITU-T T.87) : https://www.itu.int/rec/T-REC-T.87/en
//    JPEG-LS extension (ITU-T T.870): https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not compliant with these standards, although it is modified from ITU-T T.870
//


const char *title = "JLSx v0.7";


typedef    unsigned char        UI8;


#define    ABS(x)               ( ((x) < 0) ? (-(x)) : (x) )                             // get absolute value
#define    CLIP(x,a,b)          ( ((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)) )            // clip x between a~b

#define    GET2D(ptr,xsz,i,j)   (*( (ptr) + (xsz)*(i) + (j) ))
#define    SPIX(ptr,xsz,i,j,v0) (((0<=(i)) && (0<=(j)) && ((j)<(xsz))) ? GET2D((ptr),(xsz),(i),(j)) : (v0))

#define    MAX_VAL              255
#define    MID_VAL              ((MAX_VAL+1)/2)

#define    MAX_BIAS             (MAX_VAL - MID_VAL)
#define    MIN_BIAS             (-MAX_BIAS)

#define    N_QD                 16
#define    N_CONTEXT            ((N_QD>>1) * 256)

#define    N_WT                 256

#define    CTX_SCALE            10
#define    CTX_COEF             7

#define    MAX_CNT_SUM          256



static void sampleNeighbourPixels (const UI8 *img, int xsz, int i, int j, int *p_a, int *p_b, int *p_c, int *p_d, int *p_e, int *p_f, int *p_g, int *p_h, int *p_q, int *p_r, int *p_s) {
    *p_a = (int)SPIX(img, xsz, i   , j-1 , MID_VAL);
    *p_b = (int)SPIX(img, xsz, i-1 , j   , MID_VAL);
    if      (i == 0)
        *p_b = *p_a;
    else if (j == 0)
        *p_a = *p_b;
    *p_e = (int)SPIX(img, xsz, i   , j-2 , *p_a);
    *p_c = (int)SPIX(img, xsz, i-1 , j-1 , *p_b);
    *p_d = (int)SPIX(img, xsz, i-1 , j+1 , *p_b);
    *p_f = (int)SPIX(img, xsz, i-2 , j   , *p_b);
    *p_g = (int)SPIX(img, xsz, i-2 , j+1 , *p_f);
    *p_h = (int)SPIX(img, xsz, i-2 , j-1 , *p_f);
    *p_q = (int)SPIX(img, xsz, i-1 , j-2 , *p_c);
    *p_r = (int)SPIX(img, xsz, i-2 , j+2 , *p_g);
    *p_s = (int)SPIX(img, xsz, i-2 , j-2 , *p_h);
}


static int predictMIX (int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s) {
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


static void getQuantizedDelta (int a, int b, int c, int d, int e, int f, int g, int err, int *p_qd1, int *p_qd2, int *p_qd2wt) {
    const static int q_mid [] = {0, 2, 4, 7, 10, 14, 20, 26, 34, 42, 52, 64, 78, 95, 135, 200};
    
    int dh = ABS(a-e) + ABS(b-c) + ABS(b-d);
    int dv = ABS(a-c) + ABS(b-f) + ABS(d-g);
    int delta = dh + dv + 2*ABS(err);
    
    int qd, wt;
    
    for (qd=0; qd<(N_QD-1); qd++)
        if (delta <= q_mid[qd])
            break;
    
    *p_qd1 = *p_qd2 = qd;
    *p_qd2wt = 0;
    
    if (delta < q_mid[qd]) {
        wt = N_WT * (delta - q_mid[qd-1]) / (q_mid[qd] - q_mid[qd-1]);
        if (wt < (N_WT/2)) {
            *p_qd1   = qd - 1;
            *p_qd2wt = wt;
        } else {
            *p_qd2   = qd - 1;
            *p_qd2wt = N_WT - wt;
        }
    }
}


static int getContextAddress (int a, int b, int c, int d, int e, int f, int qd, int px) {
    qd >>= 1;
    qd <<= 8;
    qd |= ((px > a)       ? 0x01 : 0);
    qd |= ((px > b)       ? 0x02 : 0);
    qd |= ((px > c)       ? 0x04 : 0);
    qd |= ((px > d)       ? 0x08 : 0);
    qd |= ((px > e)       ? 0x10 : 0);
    qd |= ((px > f)       ? 0x20 : 0);
    qd |= ((px > (2*a-e)) ? 0x40 : 0);
    qd |= ((px > (2*b-f)) ? 0x80 : 0);
    return qd;
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


static int mapXtoZ (int x, int px, int sign, int near) {
    const int ty = (CLIP(px, 0, MAX_VAL - px) + near) / (2*near + 1);
    int sy = (x >= px) ? 1 : 0;
    int y  = ABS(x - px);
    
    y = (y + near) / (2*near + 1);
    
    if      (y <= 0)
        return 0;
    else if (y <= ty)
        return 2*y - (sy^sign);
    else
        return y + ty;
}


static int mapZtoX (int z, int px, int sign, int near) {
    const int ty = (CLIP(px, 0, MAX_VAL - px) + near) / (2*near + 1);
    int y, sy;
    
    if        (z <= 0) {
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
    UI8 *p_buf;
    int  creg;
    int  areg;
    UI8  buf0;
    UI8  buf1;
} ARI_CODER_t;


static ARI_CODER_t newAriEncoder (UI8 *p_buf) {
    ARI_CODER_t coder = {0, 0, 0xff*0xff, 0, 0};
    coder.p_buf = p_buf;
    return coder;
}


static ARI_CODER_t newAriDecoder (UI8 *p_buf) {
    ARI_CODER_t coder = {0, 0, 0xff*0xff, 0, 0};
    coder.p_buf = p_buf + 2;
    coder.creg  = ( *(coder.p_buf++) ) * 0xff;
    coder.creg +=   *(coder.p_buf++);
    return coder;
}


static void AriEncodeFinish (ARI_CODER_t *p_coder) {
    int i;
    for (i=0; i<4; i++) {
        if (p_coder->creg >= 0xff*0xff) {
            p_coder->creg -= 0xff*0xff;
            p_coder->buf0 ++;
            if (p_coder->buf0 == 0xff) {
                p_coder->buf0 = 0;
                p_coder->buf1 ++;
            }
        }
        *(p_coder->p_buf++) = p_coder->buf1;
        p_coder->buf1 = p_coder->buf0;
        p_coder->buf0 = (UI8)( ((p_coder->creg >> 8) + p_coder->creg + 1) >> 8 );
        p_coder->creg += p_coder->buf0;
        p_coder->creg = ((p_coder->creg&0xff) << 8) - (p_coder->creg & 0xff);
    }
}


static int AriGetAvd (int areg, int c0, int c1) {
    const static int table_th [30] = {0x7800, 0x7000, 0x6800, 0x6000, 0x5800, 0x5000, 0x4800, 0x4000, 0x3c00, 0x3800, 0x3400, 0x3000, 0x2c00, 0x2800, 0x2400, 0x2000, 0x1c00, 0x1800, 0x1400, 0x1000, 0x0e00, 0x0c00, 0x0a00, 0x0800, 0x0600, 0x0400, 0x0300, 0x0200, 0x0180, 0x0101};
    const static int table_av [31] = {0x7ab6, 0x7068, 0x6678, 0x5ce2, 0x53a6, 0x4ac0, 0x4230, 0x39f4, 0x33fc, 0x301a, 0x2c4c, 0x2892, 0x24ea, 0x2156, 0x1dd6, 0x1a66, 0x170a, 0x13c0, 0x1086, 0x0d60, 0x0b0e, 0x0986, 0x0804, 0x0686, 0x050a, 0x0394, 0x027e, 0x01c6, 0x013e, 0x0100, 0x0002};
    
    int prob, i, hd, av;
    
    prob = (c0 < c1) ? c0 : c1;
    prob <<= 16;
    prob /= c0 + c1;
    
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


static void AriUpdateCounter (int *p_c0, int *p_c1, int bin, int wt) {
    if (bin)
        (*p_c1) += wt;
    else
        (*p_c0) += wt;
    
    if (((*p_c0) + (*p_c1)) > MAX_CNT_SUM*N_WT) {
        (*p_c0) ++;
        (*p_c1) ++;
        (*p_c0) >>= 1;
        (*p_c1) >>= 1;
    }
}


static void AriEncode (ARI_CODER_t *p_coder, int *p_c0, int *p_c1, int *p_c02, int *p_c12, int wt, int bin) {
    int c0 = ((*p_c0) * (N_WT-wt) + (*p_c02) * wt + N_WT/2) / N_WT;
    int c1 = ((*p_c1) * (N_WT-wt) + (*p_c12) * wt + N_WT/2) / N_WT;
    
    int av = AriGetAvd(p_coder->areg, c0, c1);
    
    int mps_bin = (c0 < c1) ? 1 : 0;
    
    if (mps_bin == bin) {                                               // Update creg and areg
        p_coder->areg  = av;
    } else {
        p_coder->areg -= av;
        p_coder->creg += av;
    }
    
    AriUpdateCounter(p_c0 , p_c1 , bin, N_WT-wt);                      // Update counters
    AriUpdateCounter(p_c02, p_c12, bin, wt);                           // Update counters
    
    if ( p_coder->areg < 0x100 ) {                                     // Renormalization of areg and creg and output data bit stream
        if (p_coder->creg >= 0xff*0xff) {
            p_coder->creg -= 0xff*0xff;
            p_coder->buf0 ++;
            if (p_coder->buf0 == 0xff) {
                p_coder->buf0 = 0;
                p_coder->buf1 ++;
            }
        }
        *(p_coder->p_buf++) = p_coder->buf1;                           // AppendByteToStream
        p_coder->buf1 = p_coder->buf0;
        p_coder->buf0 = (UI8)( ((p_coder->creg >> 8) + p_coder->creg + 1) >> 8 );
        p_coder->creg += p_coder->buf0;
        p_coder->creg = ((p_coder->creg&0xff) << 8) - (p_coder->creg & 0xff);
        p_coder->areg = p_coder->areg * 0xff;
    }
}


static int AriDecode (ARI_CODER_t *p_coder, int *p_c0, int *p_c1, int *p_c02, int *p_c12, int wt) {
    int c0 = ((*p_c0) * (N_WT-wt) + (*p_c02) * wt + N_WT/2) / N_WT;
    int c1 = ((*p_c1) * (N_WT-wt) + (*p_c12) * wt + N_WT/2) / N_WT;
    
    int av = AriGetAvd(p_coder->areg, c0, c1);
    int bin = (c0 < c1) ? 1 : 0;

    if (p_coder->creg < av) {                                          // Detemination of bin
        p_coder->areg = av;
    } else {
        p_coder->creg = p_coder->creg - av;
        p_coder->areg = p_coder->areg - av;
        bin = !bin;
    }
    
    AriUpdateCounter(p_c0 , p_c1 , bin, N_WT-wt);                      // Update counters
    AriUpdateCounter(p_c02, p_c12, bin, wt);                           // Update counters

    if (p_coder->areg < 0x100) {                                       // Renormalization of areg and creg
        p_coder->creg = (p_coder->creg << 8) - p_coder->creg + *(p_coder->p_buf++);   // GetByteFromStream
        p_coder->areg = (p_coder->areg << 8) - p_coder->areg;
    }

    return bin;
}


static void initCounterTree (int c_tree [][256]) {
    int i, j;
    for (i=0; i<N_QD; i++)
        for (j=0; j<256; j++)
            c_tree[i][j] = N_WT;
}


static void encodeZ (ARI_CODER_t *p_coder, int k_step, int c0_tree [][256], int c1_tree [][256], int qd1, int qd2, int wt, int z) {
    const int k_max = (N_QD-1) / k_step;
    int cl=0, k, bin;
    
    if ((qd2 / k_step) != (qd1 / k_step))
        qd2 = qd1;
    
    for (;;) {
        k = qd1 / k_step;
        
        bin = (cl >> k_max) < (z >> k);
        
        AriEncode(p_coder, &c0_tree[qd1][cl], &c1_tree[qd1][cl], &c0_tree[qd2][cl], &c1_tree[qd2][cl], wt, bin);
        
        if (!bin)
            break;
        
        cl += (1 << k_max);
        
        if (cl >= 256) {
            cl >>= 1;
            qd2 = qd1 = (k + 1) * k_step;
        }
    }
    
    cl ++;
    
    for (k--; k>=0; k--) {
        bin = (z >> k) & 1;
        AriEncode(p_coder, &c0_tree[qd1][cl], &c1_tree[qd1][cl], &c0_tree[qd2][cl], &c1_tree[qd2][cl], wt, bin);
        cl += bin ? (1<<k) : 1;
    }
}


static int decodeZ (ARI_CODER_t *p_coder, int k_step, int c0_tree [][256], int c1_tree [][256], int qd1, int qd2, int wt) {
    const int k_max = (N_QD-1) / k_step;
    int cl=0, k, bin, z;
    
    if ((qd2 / k_step) != (qd1 / k_step))
        qd2 = qd1;

    for (;;) {
        k = qd1 / k_step;
        
        bin = AriDecode(p_coder, &c0_tree[qd1][cl], &c1_tree[qd1][cl], &c0_tree[qd2][cl], &c1_tree[qd2][cl], wt);
        
        if (!bin)
            break;
        
        cl += (1 << k_max);
        
        if (cl >= 256) {
            cl >>= 1;
            qd2 = qd1 = (k + 1) * k_step;
        }
    }
    
    z = ((cl >> k_max) << k);
    cl ++;
    
    for (k--; k>=0; k--) {
        bin = AriDecode(p_coder, &c0_tree[qd1][cl], &c1_tree[qd1][cl], &c0_tree[qd2][cl], &c1_tree[qd2][cl], wt);
        z  += bin ? (1<<k) : 0;
        cl += bin ? (1<<k) : 1;
    }

    return z;
}



static void putHeader (UI8 **pp_buf, int ysz, int xsz, int near) {
    int i;
    for (i=0; title[i]!=0; i++)                 // put title
        *((*pp_buf)++) = (UI8)title[i];
    *((*pp_buf)++) = (UI8)near;                 // put NEAR
    *((*pp_buf)++) = (UI8)(xsz >> 8);           // put image width 
    *((*pp_buf)++) = (UI8)(xsz >> 0);
    *((*pp_buf)++) = (UI8)(ysz >> 8);           // put image height 
    *((*pp_buf)++) = (UI8)(ysz >> 0);
}


// return:  -1:failed  0:success
static int getHeader (UI8 **pp_buf, int *p_ysz, int *p_xsz, int *p_near) {
    int i;
    for (i=0; title[i]; i++)                    // check title 
        if ( *((*pp_buf)++) != (UI8)title[i] )
            return -1;
    *p_near =   *((*pp_buf)++);                 // get NEAR 
    *p_xsz  = ( *((*pp_buf)++) ) << 8;          // get image width  
    *p_xsz +=   *((*pp_buf)++) ;
    *p_ysz  = ( *((*pp_buf)++) ) << 8;          // get image height 
    *p_ysz +=   *((*pp_buf)++) ;
    return 0;
}



// return:
//     positive value : compressed stream length
//     -1             : failed
int JLSxCompress (UI8 *p_buf, UI8 *p_img, int ysz, int xsz, int near) {
    int i, j, k_step;
    
    int ctx_array [N_CONTEXT] = {0};
    
    int c0_tree [N_QD][256], c1_tree [N_QD][256];
    
    ARI_CODER_t coder;
    
    UI8 *p_buf_base = p_buf;
    
    near = CLIP(near, 0, MAX_VAL);
    
    putHeader(&p_buf, ysz, xsz, near);
    
    coder = newAriEncoder(p_buf);
    
    initCounterTree(c0_tree);
    initCounterTree(c1_tree);
    
    k_step = CLIP(3+2*near, 3, N_QD);
    
    for (i=0; i<ysz; i++) {
        int err0 = 0;
        
        for (j=0; j<xsz; j++) {
            int  a, b, c, d, e, f, g, h, q, r, s, px, qd1, qd2, qd2wt, adr, sign, x, z;
            
            sampleNeighbourPixels(p_img, xsz, i, j, &a, &b, &c, &d, &e, &f, &g, &h, &q, &r, &s);
            
            getQuantizedDelta(a, b, c, d, e, f, g, err0, &qd1, &qd2, &qd2wt);
            
            err0 = px = predictMIX(a, b, c, d, e, f, g, h, q, r, s);
            
            adr = getContextAddress(a, b, c, d, e, f, qd1, px);
            
            sign = correctPxByContext(ctx_array[adr], &px);
            
            x = GET2D(p_img, xsz, i, j);
            
            z = mapXtoZ(x, px, sign, near);
            
            encodeZ(&coder, k_step, c0_tree, c1_tree, qd1, qd2, qd2wt, z);
            
            x = mapZtoX(z, px, sign, near);
            
            GET2D(p_img, xsz, i, j) = (UI8)x;
            
            err0 = CLIP((x-err0), MIN_BIAS, MAX_BIAS);
            
            updateContext(&ctx_array[adr], err0);
        }
    }

    AriEncodeFinish(&coder);

    return coder.p_buf - p_buf_base;
}


// return:
//     0 : success
//    -1 : failed
int JLSxDecompress(UI8 *p_buf, UI8 *p_img, int *p_ysz, int *p_xsz, int *p_near) {
    int i, j, k_step;
    
    int ctx_array [N_CONTEXT] = {0};
    
    int c0_tree [N_QD][256], c1_tree [N_QD][256];
    
    ARI_CODER_t coder;

    if ( getHeader(&p_buf, p_ysz, p_xsz, p_near) )
        return -1;
    
    coder = newAriDecoder(p_buf);
    
    initCounterTree(c0_tree);
    initCounterTree(c1_tree);
    
    k_step = CLIP(3+2*(*p_near), 3, N_QD);
    
    for (i=0; i<(*p_ysz); i++) {
        int err0 = 0;
        
        for (j=0; j<(*p_xsz); j++) {
            int  a, b, c, d, e, f, g, h, q, r, s, px, qd1, qd2, qd2wt, adr, sign, x, z;
            
            sampleNeighbourPixels(p_img, (*p_xsz), i, j, &a, &b, &c, &d, &e, &f, &g, &h, &q, &r, &s);
            
            getQuantizedDelta(a, b, c, d, e, f, g, err0, &qd1, &qd2, &qd2wt);
            
            err0 = px = predictMIX(a, b, c, d, e, f, g, h, q, r, s);
            
            adr = getContextAddress(a, b, c, d, e, f, qd1, px);
            
            sign = correctPxByContext(ctx_array[adr], &px);
            
            z = decodeZ(&coder, k_step, c0_tree, c1_tree, qd1, qd2, qd2wt);
            
            x = mapZtoX(z, px, sign, (*p_near));
            
            GET2D(p_img, (*p_xsz), i, j) = (UI8)x;
            
            err0 = CLIP((x-err0), MIN_BIAS, MAX_BIAS);
            
            updateContext(&ctx_array[adr], err0);
        }
    }

    return 0;
}

