
#include <stdlib.h>

#include "QNBLIC.h"

typedef    unsigned char          UI8;


#define    ENABLE_MULTITHREAD     1

#define    ABS(x)                 ( ((x)<0) ? (-(x)) : (x) )                             // get absolute value
#define    CLIP(x,a,b)            ( ((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)) )          // clip x between a~b
#define    MIN(a,b)               ( ((a)<(b)) ? (a) : (b) )
#define    MAX(a,b)               ( ((a)>(b)) ? (a) : (b) )

#define    G2D(ptr,width,i,j)     (*( (ptr) + (width)*(i) + (j) ))
#define    SPIX(ptr,width,i,j,v0) (((0<=(i)) && (0<=(j)) && ((j)<(width))) ? G2D((ptr),(width),(i),(j)) : (v0))

#define    MAX_VAL                255
#define    MID_VAL                ((MAX_VAL+1)/2)

#define    N_QD                   12
#define    N_CONTEXT              (N_QD * 256)

#define    CTX_COEF               7
#define    CTX_SCALE              11

    

// return:  -1:failed  0:success
static int checkSize (int height, int width) {
    if (height <= 0)
        return -1;
    if (width  <= 0)
        return -1;
    if (height > QNBLIC_MAX_HEIGHT)
        return -1;
    if (width  > QNBLIC_MAX_WIDTH)
        return -1;
    if ((height*width) > QNBLIC_MAX_IMG_SIZE)
        return -1;
    return 0;
}


#define    SAMPLE_PIXELS(p_img,width,i,j,a,b,c,d,e,f,g,h,q,r,s) {          \
    a = (int)SPIX(p_img, width, i   , j-1 , MID_VAL);                      \
    b = (int)SPIX(p_img, width, i-1 , j   , MID_VAL);                      \
    if      (i == 0)                                                       \
        b = a;                                                             \
    else if (j == 0)                                                       \
        a = b;                                                             \
    e = (int)SPIX(p_img, width, i   , j-2 , a);                            \
    c = (int)SPIX(p_img, width, i-1 , j-1 , b);                            \
    d = (int)SPIX(p_img, width, i-1 , j+1 , b);                            \
    f = (int)SPIX(p_img, width, i-2 , j   , b);                            \
    g = (int)SPIX(p_img, width, i-2 , j+1 , f);                            \
    h = (int)SPIX(p_img, width, i-2 , j-1 , f);                            \
    q = (int)SPIX(p_img, width, i-1 , j-2 , c);                            \
    r = (int)SPIX(p_img, width, i-2 , j+2 , g);                            \
    s = (int)SPIX(p_img, width, i-2 , j-2 , h);                            \
}


#define    SAMPLE_PIXELS_NEXT(p_img,width,i,j,x,a,b,c,d,e,f,g,h,q,r,s) {   \
    e = a; \
    a = x; \
    q = c; \
    c = b; \
    b = d; \
    s = h; \
    h = f; \
    f = g; \
    g = r; \
    d = (i<=0) ? a : (j+2>=width) ? d : (int)G2D(p_img, width, i-1 , j+2); \
    r = (i<=1) ? d : (j+3>=width) ? r : (int)G2D(p_img, width, i-2 , j+3); \
}


static void initPTLookupTable (UI8 tab[]) {
    int i;
    for (i=0; i<608; i++) {
        if      (i <= 4  ) tab[i] = 0;
        else if (i <= 11 ) tab[i] = 1;
        else if (i <= 33 ) tab[i] = 2;
        else if (i <= 77 ) tab[i] = 3;
        else if (i <= 193) tab[i] = 4;
        else if (i <= 430) tab[i] = 5;
        else if (i <= 600) tab[i] = 6;
        else               tab[i] = 7;
    }
}


static int simplePredict (int a, int b, int c, int d, int e, int f, int g, int h, int q, int r, int s, UI8 tab[]) {
    int px_lnr, px_ang, cost, csum, cmin, wt;
    
    px_lnr = CLIP((9*a + 9*b + (d<<1) - (c<<1) - e - f), 0, 16*MAX_VAL);
    
    cmin = csum = 2 * (ABS(a-e) + ABS(c-q) + ABS(b-c) + ABS(d-b));
    px_ang = 2 * a;
    
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
    csum = MIN((csum>>3), 608-1);
    wt = tab[csum];
    
    return ((8*wt*px_ang + (8-wt)*px_lnr + 64) >> 7);
}


static void initQDLookupTable (UI8 tab_qd[]) {
    int i;
    for (i=0; i<152; i++) {
        if      (i <= 0  ) tab_qd[i] = 0;
        else if (i <= 1  ) tab_qd[i] = 1;
        else if (i <= 3  ) tab_qd[i] = 2;
        else if (i <= 5  ) tab_qd[i] = 3;
        else if (i <= 8  ) tab_qd[i] = 4;
        else if (i <= 14 ) tab_qd[i] = 5;
        else if (i <= 24 ) tab_qd[i] = 6;
        else if (i <= 38 ) tab_qd[i] = 7;
        else if (i <= 62 ) tab_qd[i] = 8;
        else if (i <= 100) tab_qd[i] = 9;
        else if (i <= 150) tab_qd[i] =10;
        else               tab_qd[i] =11;
    }
}


#define  GET_CONTEXT_ADDRESS(adr,a,b,c,d,e,f,px,qd) {  \
    adr = qd << 1;   adr |= (px > a);                  \
    adr <<= 1;       adr |= (px > b);                  \
    adr <<= 1;       adr |= (px > c);                  \
    adr <<= 1;       adr |= (px > d);                  \
    adr <<= 1;       adr |= (px > e);                  \
    adr <<= 1;       adr |= (px > f);                  \
    adr <<= 1;       adr |= (px > (2*a-e));            \
    adr <<= 1;       adr |= (px > (2*b-f));            \
}


#define  CORRECT_PX(ctx,px0,px,sign)  {                \
    sign = ((ctx) >> (CTX_SCALE-1)) & 1;               \
    px   = px0 + ((ctx) >> CTX_SCALE) + sign;          \
    px   = CLIP(px, 0, MAX_VAL);                       \
}


#define  UPDATE_CONTEXT(ctx,err)  {                    \
    ctx  = ((ctx<<CTX_COEF) - ctx);                    \
    ctx += (err << CTX_SCALE);                         \
    ctx += ((1 << (CTX_COEF-1)) - 1);                  \
    ctx>>= CTX_COEF;                                   \
}


static int mapXtoY (int x, int px, int sign) {
    const int ty = MIN(px, (MAX_VAL-px));
    int sy = (x >= px);
    int y  = ABS(x - px);
    
    if (y <= 0)
        return 0;
    else if (y <= ty)
        return 2*y - (sy^sign);
    else
        return y + ty;
}


static int mapYtoX (int z, int px, int sign) {
    const int ty = MIN(px, (MAX_VAL-px));
    int y;
    
    if (z <= 0) {
        return px;
    } else if (z <= 2*ty) {
        y = (z + 1) >> 1;
        return px + (((z & 1) ^ sign) ? y : -y);
    } else {
        return px + ((px < MID_VAL) ? (z-ty) : (ty-z));
    }
}




#define    NORM_BITS            15
#define    NORM_MASK            ((1 << NORM_BITS) - 1)
#define    NORM_SUM             ((1 << NORM_BITS)    )              // sum of normalized histogram

#define    ANS_MVAL             MAX_VAL

#define    ANS_BITS             16
#define    ANS_MASK             ((1 << (  ANS_BITS)) - 1)
#define    ANS_LOW_BOUND        ((1 << (  ANS_BITS))    )
#define    ANS_HIGH_BOUND_NORM  ((1 << (2*ANS_BITS-NORM_BITS)) - 1)


static void initHistAcc (uint32_t hist[], uint32_t hist_acc[]) {
    int i;
    hist_acc[0] = 0;
    for (i=1; i<=ANS_MVAL; i++)
        hist_acc[i] = hist_acc[i-1] + hist[i-1];
}


static void initDecodeLookupTable (UI8 tab_dec[], uint32_t hist_acc[]) {
    uint32_t i, v;
    for (v=0; v<ANS_MVAL; v++)
        for (i=hist_acc[v]; i<hist_acc[v+1]; i++)
            tab_dec[i] = (UI8)v;
    for (i=hist_acc[ANS_MVAL]; i<NORM_SUM; i++)
        tab_dec[i] = ANS_MVAL;
}


static void normHist (uint32_t hist[]) {
    uint32_t i, j=0;
    uint32_t sum      = 0;
    uint32_t nz_count = 0;
    double   scale;  // this func is only used in encoder, so using floating-point types here will not result in images being unable to be decoded across platforms
    
    for (i=0; i<=ANS_MVAL; i++) {
        if (hist[i] > 0) {
            sum += hist[i];
            nz_count ++;
            j = i;
        }
    }
    
    scale = (1.0 * NORM_SUM) / sum;
    
    if        (nz_count <= 0) {
        hist[0] = NORM_SUM - 1;
        hist[1] = 1;
        
    } else if (nz_count == 1) {
        hist[j] = NORM_SUM - 1;
        j = ((j+1) % (ANS_MVAL+1));
        hist[j] = 1;
        
    } else {
        sum = 0;
        
        for (i=0; i<=ANS_MVAL; i++) {
            if (hist[i] > 0) {
                hist[i] = (uint32_t) (0.49 + scale * hist[i]);
                hist[i] = MAX(hist[i], 1);
                sum += hist[i];
            }
        }
        
        for (i=0; sum>NORM_SUM; i=((i+1)%(ANS_MVAL+1)) ) {
            if (hist[i] > 1) {
                hist[i] --;
                sum --;
            }
        }
        
        for (j=0; sum<NORM_SUM; i=((i+1)%(ANS_MVAL+1)) ) {
            if (hist[i] > 0) {
                hist[i] ++;
                sum ++;
            }
        }
    }
}



#define   W16BIT(p_buf,value)   { (*((p_buf)++)) = (uint16_t)(value); }
#define   R16BIT(p_buf,value)   { (value) = (*((p_buf)++)); }
#define   ROR16BIT(p_buf,value) { (value)|= (*((p_buf)++)); }


#define   ANS_ENC(ans,p_buf,h,hacc)  {                    \
    uint32_t dans = (ans) / (h);                          \
    if (dans > ANS_HIGH_BOUND_NORM) {                     \
        W16BIT(p_buf, (ANS_MASK & (ans)));                \
        (ans) >>= ANS_BITS;                               \
        dans = (ans) / (h);                               \
    }                                                     \
    (ans) %= (h);                                         \
    (ans) += (dans << NORM_BITS) + (hacc);                \
}


#define   ANS_ENC_INIT_VALUE     ANS_LOW_BOUND


#define   ANS_ENC_FIN(ans,p_buf)  {                       \
    W16BIT(p_buf, (ANS_MASK &  (ans)           ));        \
    W16BIT(p_buf, (ANS_MASK & ((ans)>>ANS_BITS)));        \
}


#define   ANS_DEC_START(ans,p_buf)  {                     \
    R16BIT(  p_buf, ans);                                 \
    (ans) <<= ANS_BITS;                                   \
    ROR16BIT(p_buf, ans);                                 \
}


#define  ANS_DEC(ans,p_buf,value,hist,hist_acc,tab_dec) { \
    uint32_t lb = ans & NORM_MASK;                        \
    value = tab_dec[lb];                                  \
    ans >>= NORM_BITS;                                    \
    ans  *= hist[value];                                  \
    ans  += lb;                                           \
    ans  -= hist_acc[value];                              \
    if (ans < ANS_LOW_BOUND) {                            \
        ans <<= ANS_BITS;                                 \
        ROR16BIT(p_buf, ans);                             \
    }                                                     \
}



static void reverseWords (uint16_t *p_start, uint16_t *p_end) {
    uint16_t tmp;
    p_end --;
    while (p_start < p_end) {
        tmp = *p_start;
        *p_start = *p_end;
        *p_end = tmp;
        p_start ++;
        p_end   --;
    }
}



// encode a histogram
// use a simple compression code:
//         |  a 16-bit code   | explain                                                      |
//   case1 | 0AAAAAAAAAAAAAAA | where AAAAAAAAAAAAAAA is a 15-bit histogram value            |
//   case2 | 10BBBBBBBCCCCCCC | where BBBBBBB and CCCCCCC are two 7-bit histogram values     |
//   case3 | 1100DDDDEEEEFFFF | where DDDD, EEEE, and FFFF are three 4-bit histogram values  |
//   case4 | 1101GGGHHHIIIJJJ | where GGG, HHH, III, and JJJ are four 3-bit histogram values |
//   case5 | 111XKKKKRRRRRRRR | repeat X for (RRRRRRRR+4) times (X is 0 or 1),
//                              and follows a 4-bit value KKKK   
//                              if KKKK==X, KKKK should be ignored                           |
static void decodeHist (uint16_t **pp_buf, uint32_t hist[]) {
    uint32_t i, sum=0;
    
    for (i=0; i<=ANS_MVAL; i++)
        hist[i] = 0;
    
    for (i=0; i<=ANS_MVAL && sum<NORM_SUM ;) {
        uint16_t code, len, h0, he;
        
        R16BIT(*pp_buf, code);
        
        if        ((code>>15) == 0) {
            sum += ( hist[i++] = code );
        } else if ((code>>14) == 2) {
            sum += ( hist[i++] = (0x7F& (code>>7) ) );
            sum += ( hist[i++] = (0x7F& (code   ) ) );
        } else if ((code>>12) == 12) {
            sum += ( hist[i++] = (0xF & (code>>8) ) );
            sum += ( hist[i++] = (0xF & (code>>4) ) );
            sum += ( hist[i++] = (0xF & (code   ) ) );
        } else if ((code>>12) == 13) {
            sum += ( hist[i++] = (0x7 & (code>>9) ) );
            sum += ( hist[i++] = (0x7 & (code>>6) ) );
            sum += ( hist[i++] = (0x7 & (code>>3) ) );
            sum += ( hist[i++] = (0x7 & (code   ) ) );
        } else {
            len = 0xFF &  code;
            he  = 0xF  & (code >> 8);
            h0  = 0x1  & (code >> 12);
            
            for (len+=4; len>0; len--)        // repeat
                sum += ( hist[i++] = h0 );
            
            if (he != h0)
                sum += ( hist[i++] = he );
        }
    }
}


static void encodeHist (uint16_t **pp_buf, uint32_t hist[]) {
    uint32_t i, j, sum=0;
    
    for (i=0; i<=ANS_MVAL && sum<NORM_SUM ;) {
        uint16_t code, len, h1, h2, h3, he=0xFFFF, h0;
        
        h0 = (uint16_t)hist[i];
        
        for (j=i+1; j<=ANS_MVAL; j++) {
            he = (uint16_t)hist[j];
            if (h0 != he)
                break;
        }
        
        len = (uint16_t)(j - i);
        
        if (h0<=1 && len>=4) {                                // repeat length >= 4
            if (j<=ANS_MVAL && he<=15)                        // not reaches end, and he can repr by 4-bit
                j ++;                                         // encode one more value (he, KKKK in case5)
            else
                he = h0;
            code = (7<<13) | (h0<<12) | (he<<8) | (len-4);
        } else {
            h1 = (i+1<=ANS_MVAL) ? (uint16_t)hist[i+1] : 0xFFFF;
            h2 = (i+2<=ANS_MVAL) ? (uint16_t)hist[i+2] : 0xFFFF;
            h3 = (i+3<=ANS_MVAL) ? (uint16_t)hist[i+3] : 0xFFFF;
            
            if        (h0<=7  && h1<=7  && h2<=7 && h3<=7) {
                code = (13<<12) | (h0<<9) | (h1<<6) | (h2<<3) | h3;
                j = i + 4;
            } else if (h0<=15 && h1<=15 && h2<=15) {
                code = (12<<12) | (h0<<8) | (h1<<4) | h2;
                j = i + 3;
            } else if (h0<=127 && h1<=127) {
                code =  (2<<14) | (h0<<7) |  h1;
                j = i + 2;
            } else {
                code =             h0;
                j = i + 1;
            }
        }
        
        W16BIT(*pp_buf, code);
        
        for (; i<j; i++)
            sum += hist[i];
    }
}


#define   TITLE    "Q0.2"

#define   HDR1     ( (((uint16_t)TITLE[1])<<8) + ((uint16_t)TITLE[0]) )
#define   HDR2     ( (((uint16_t)TITLE[3])<<8) + ((uint16_t)TITLE[2]) )

#define   WHEADER(p_buf,height,width) {     \
    W16BIT(p_buf, HDR1);                    \
    W16BIT(p_buf, HDR2);                    \
    W16BIT(p_buf, height);                  \
    W16BIT(p_buf, width);                   \
}

#define   RHEADER(p_buf,height,width,ret) { \
    uint16_t hdr1, hdr2;                    \
    R16BIT(p_buf, hdr1);                    \
    R16BIT(p_buf, hdr2);                    \
    if (hdr1 == HDR1 && hdr2 == HDR2) {     \
        R16BIT(p_buf, height);              \
        R16BIT(p_buf, width);               \
        ret = checkSize(height, width);     \
    } else {                                \
        ret =  -1;                          \
    }                                       \
}



// return :
//                 0 : success
//                -1 : failed
int QNBLICdecompress (uint16_t *p_buf, UI8 *p_img, int *p_height, int *p_width) {
    int  i, j, height, width;
    int  ctx_array [N_CONTEXT] = {0};
    UI8  tab_qd    [152];
    UI8  tab_pt    [608];
    uint32_t ans;
    
    uint32_t hist     [N_QD][ANS_MVAL+1];
    uint32_t hist_acc [N_QD][ANS_MVAL+1];
    
    UI8 (*tab_dec) [NORM_SUM];
    
    RHEADER(p_buf, height, width, i);
    
    if (i)
        return -1;
    
    *p_height = height;
    *p_width  = width;
    
    tab_dec = (UI8 (*) [NORM_SUM]) malloc (sizeof(UI8) * NORM_SUM * N_QD);
    
    if (tab_dec == NULL)
        return -1;
    
    initQDLookupTable(tab_qd);
    initPTLookupTable(tab_pt);
    
    for (i=0; i<N_QD; i++) {
        decodeHist(&p_buf, hist[i]);
        initHistAcc(hist[i], hist_acc[i]);
        initDecodeLookupTable(tab_dec[i], hist_acc[i]);
    }
    
    ANS_DEC_START(ans, p_buf);
    
    for (i=0; i<height; i++) {
        int x=0, a=0, b=0, c=0, d=0, e=0, f=0, g=0, h=0, q=0, r=0, s=0;
        int err = 0;
        
        SAMPLE_PIXELS(p_img, width, i, 0, a, b, c, d, e, f, g, h, q, r, s);
        
        for (j=0; j<width; j++) {
            int px0, px, qd, adr, ctx, sign, y;
            
            px0 = simplePredict(a, b, c, d, e, f, g, h, q, r, s, tab_pt);
            
            qd = ABS(a-e) + ABS(b-c) + ABS(b-d) + ABS(a-c) + ABS(b-f) + ABS(d-g) + 2*ABS(err);
            qd = MIN(qd, 152-1);
            qd = tab_qd[qd];
            
            GET_CONTEXT_ADDRESS(adr, a, b, c, d, e, f, px0, qd);
            
            ctx = ctx_array[adr];
            CORRECT_PX(ctx, px0, px, sign);
            
            ANS_DEC(ans, p_buf, y, hist[qd], hist_acc[qd], tab_dec[qd]);
            
            x = mapYtoX(y, px, sign);
            G2D(p_img, width, i, j) = (UI8)x;
            
            err = x - px0;
            
            UPDATE_CONTEXT(ctx, err);
            ctx_array[adr] = ctx;
            
            SAMPLE_PIXELS_NEXT(p_img, width, i, j, x, a, b, c, d, e, f, g, h, q, r, s);
        }
    }
    
    free(tab_dec);
    
    return 0;
}



// return :
//    positive value : compressed stream length
//                -1 : failed
static int QNBLICcompressX (uint16_t *p_buf, UI8 *p_img, int height, int width) {
    int  i, j;
    int  ctx_array [N_CONTEXT] = {0};
    UI8  tab_qd    [152]; 
    UI8  tab_pt    [608];
    
    uint32_t hist     [N_QD][ANS_MVAL+1] = {{0}};
    uint32_t hist_acc [N_QD][ANS_MVAL+1];
    
    uint16_t *p_buf_base = p_buf;
    
    struct { UI8 qd; UI8 y; } *py_base, *py;
    
    if (checkSize(height, width))
        return -1;
    
    py_base = py = malloc(sizeof(*py_base) * height * width);
    
    if (py_base == NULL)
        return -1;
    
    initQDLookupTable(tab_qd);
    initPTLookupTable(tab_pt);
    
    for (i=0; i<height; i++) {
        int x=0, a=0, b=0, c=0, d=0, e=0, f=0, g=0, h=0, q=0, r=0, s=0;
        int err = 0;
        
        SAMPLE_PIXELS(p_img, width, i, 0, a, b, c, d, e, f, g, h, q, r, s);
        
        for (j=0; j<width; j++) {
            int px, qd, adr, ctx, sign, y;
            
            x = G2D(p_img, width, i, j);
            
            px = simplePredict(a, b, c, d, e, f, g, h, q, r, s, tab_pt);
            
            qd = ABS(a-e) + ABS(b-c) + ABS(b-d) + ABS(a-c) + ABS(b-f) + ABS(d-g) + 2*ABS(err);
            qd = MIN(qd, 152-1);
            qd = tab_qd[qd];
            
            err = x - px;
            
            GET_CONTEXT_ADDRESS(adr, a, b, c, d, e, f, px, qd);
            
            ctx = ctx_array[adr];
            CORRECT_PX(ctx, px, px, sign);
            
            y = mapXtoY(x, px, sign);
            
            py->qd = (UI8)qd;
            py->y  = (UI8)y;
            py ++;
            
            hist[qd][y] ++;
            
            UPDATE_CONTEXT(ctx, err);
            ctx_array[adr] = ctx;
            
            SAMPLE_PIXELS_NEXT(p_img, width, i, j, x, a, b, c, d, e, f, g, h, q, r, s);
        }
    }
    
    WHEADER(p_buf, height, width);
    
    for (i=0; i<N_QD; i++) {
        normHist(hist[i]);
        initHistAcc(hist[i], hist_acc[i]);
        encodeHist(&p_buf, hist[i]);
    }
    
    //printf("    header+hist len = %ld B\n", 2*(p_buf-p_buf_base));
    
    {
        uint16_t *p_buf_start = p_buf;
        uint32_t ans = ANS_ENC_INIT_VALUE;
        
        for (py--; py>=py_base; py--) {
            UI8 qd = py->qd;
            UI8 y  = py->y;
            uint32_t h  = hist[qd][y];
            uint32_t ha = hist_acc[qd][y];
            ANS_ENC(ans, p_buf, h, ha);
        }
        
        ANS_ENC_FIN(ans, p_buf);
        
        reverseWords(p_buf_start, p_buf);
    }
    
    free(py_base);
    
    return p_buf - p_buf_base;
}




#if  ((!ENABLE_MULTITHREAD) || (!defined(_WIN32)))   // multithread disabled, or not windows

int QNBLICcompress (uint16_t *p_buf, UI8 *p_img, int height, int width) {
    return QNBLICcompressX(p_buf, p_img, height, width);
}

#else         // windows, use multithread to speedup QNBLIC compressor


//#include <stdio.h>
#include <Windows.h>
#include <process.h>


#define N_THREAD      4


typedef struct {
    UI8     x;
    UI8     px;
    int16_t adr;
} MetaData_t;


typedef struct {
    int         height;
    int         width;
    int         row_per_unit;
    int         i_thd;
    UI8        *p_img;
    MetaData_t *p_meta;
    HANDLE      semaphore;
} ThreadArg_t;


static void PredictThreadFunc (void* arg) {
    UI8  tab_qd [152]; 
    UI8  tab_pt [608];
    
    int i, j, height, width, row_per_unit, i_thd;
    UI8        *p_img;
    MetaData_t *p_meta;
    HANDLE      semaphore;
    
    height       = ((ThreadArg_t*)arg)->height;
    width        = ((ThreadArg_t*)arg)->width;
    row_per_unit = ((ThreadArg_t*)arg)->row_per_unit;
    i_thd        = ((ThreadArg_t*)arg)->i_thd;
    p_img        = ((ThreadArg_t*)arg)->p_img;
    p_meta       = ((ThreadArg_t*)arg)->p_meta;
    semaphore    = ((ThreadArg_t*)arg)->semaphore;
    
    initQDLookupTable(tab_qd);
    initPTLookupTable(tab_pt);
    
    for (i=i_thd*row_per_unit; i<height; ) {
        int x=0, a=0, b=0, c=0, d=0, e=0, f=0, g=0, h=0, q=0, r=0, s=0;
        int err = 0;
        
        SAMPLE_PIXELS(p_img, width, i, 0, a, b, c, d, e, f, g, h, q, r, s);
        
        for (j=0; j<width; j++) {
            int px, qd, adr;
            
            x = G2D(p_img, width, i, j);
            
            px = simplePredict(a, b, c, d, e, f, g, h, q, r, s, tab_pt);
            
            qd = ABS(a-e) + ABS(b-c) + ABS(b-d) + ABS(a-c) + ABS(b-f) + ABS(d-g) + 2*ABS(err);
            qd = MIN(qd, 152-1);
            qd = tab_qd[qd];
            
            err = x - px;
            
            GET_CONTEXT_ADDRESS(adr, a, b, c, d, e, f, px, qd);
            
            p_meta->x   = (UI8)x;
            p_meta->px  = (UI8)px;
            p_meta->adr = (int16_t)adr;
            p_meta ++;
            
            SAMPLE_PIXELS_NEXT(p_img, width, i, j, x, a, b, c, d, e, f, g, h, q, r, s);
        }
        
        i++;
        
        if ((i%row_per_unit) == 0 || i==height) {
            ReleaseSemaphore(semaphore, 1, NULL);
            i += (N_THREAD-1) * row_per_unit;
        }
    }
}


static int QNBLICcompressMultiThread (uint16_t *p_buf, UI8 *p_img, int height, int width) {
    int  i, j, i_thd, row_per_unit, unit_count, units_per_thread, row_per_thread;
    int  ctx_array [N_CONTEXT] = {0};
    
    uint32_t hist     [N_QD][ANS_MVAL+1] = {{0}};
    uint32_t hist_acc [N_QD][ANS_MVAL+1];
    
    uint16_t *p_buf_base = p_buf;
    
    MetaData_t *p_meta_base [N_THREAD];
    MetaData_t *p_meta      [N_THREAD];
    
    ThreadArg_t threads_arg       [N_THREAD];
    HANDLE      threads_semaphore [N_THREAD];
    HANDLE      threads_handle    [N_THREAD];
    
    struct { UI8 qd; UI8 y; } *py_base, *py;
    
    if (checkSize(height, width))
        return -1;
    
    py_base = py = malloc(height * width * sizeof(*py_base));
    
    if (py_base == NULL)
        return -1;
    
    if      (width <= 2048)
        row_per_unit = 16;
    else if (width <= 4096)
        row_per_unit = 8;
    else if (width <= 8192)
        row_per_unit = 4;
    else if (width <= 16384)
        row_per_unit = 2;
    else
        row_per_unit = 1;
    
    unit_count       = (height - 1 + row_per_unit) / row_per_unit;
    units_per_thread = (unit_count  - 1 + N_THREAD) / N_THREAD;
    row_per_thread   = units_per_thread * row_per_unit;
    
    //printf("    multithread config:  rpu=%d  u=%d  upt=%d  rpt=%d\n", row_per_unit, unit_count, units_per_thread, row_per_thread);
    
    for (i_thd=0; i_thd<N_THREAD; i_thd++) {
        p_meta_base[i_thd] = p_meta[i_thd] = malloc(row_per_thread * width * sizeof(MetaData_t));
        if ( p_meta[i_thd] == NULL)
            return -1;
        threads_semaphore[i_thd] = CreateSemaphore(NULL, 0, units_per_thread, NULL);
    }
    
    for (i_thd=0; i_thd<N_THREAD; i_thd++) {
        threads_arg[i_thd].height       = height;
        threads_arg[i_thd].width        = width;
        threads_arg[i_thd].row_per_unit = row_per_unit;
        threads_arg[i_thd].i_thd        = i_thd;
        threads_arg[i_thd].p_img        = p_img;
        threads_arg[i_thd].p_meta       = p_meta[i_thd];
        threads_arg[i_thd].semaphore    = threads_semaphore[i_thd];
        threads_handle[i_thd] = (HANDLE)_beginthread(PredictThreadFunc, 0, (void*)(&threads_arg[i_thd]));
    }
    
    for (i=0; i<height; i++) {
        i_thd = (i/row_per_unit) % N_THREAD;
        
        if ((i%row_per_unit) == 0)
            WaitForSingleObject(threads_semaphore[i_thd], INFINITE);
        
        for (j=0; j<width; j++) {
            int x, px0, px, qd, adr, ctx, sign, y;
            
            x   = p_meta[i_thd]->x;
            px0 = p_meta[i_thd]->px;
            adr = p_meta[i_thd]->adr;
            p_meta[i_thd] ++;
            
            qd = adr >> 8;
            
            ctx = ctx_array[adr];
            CORRECT_PX(ctx, px0, px, sign);
            UPDATE_CONTEXT(ctx, (x-px0));
            ctx_array[adr] = ctx;
            
            y = mapXtoY(x, px, sign);
            
            py->qd = (UI8)qd;
            py->y  = (UI8)y;
            py ++;
            
            hist[qd][y] ++;
        }
    }
    
    for (i_thd=0; i_thd<N_THREAD; i_thd++) {
        WaitForSingleObject(threads_handle[i_thd], INFINITE);  // end of subthreads
        free(p_meta_base[i_thd]);
    }
    
    WHEADER(p_buf, height, width);
    
    for (i=0; i<N_QD; i++) {
        normHist(hist[i]);
        initHistAcc(hist[i], hist_acc[i]);
        encodeHist(&p_buf, hist[i]);
    }
    
    {
        uint16_t *p_buf_start = p_buf;
        uint32_t ans = ANS_ENC_INIT_VALUE;
        
        for (py--; py>=py_base; py--) {
            UI8 qd = py->qd;
            UI8 y  = py->y;
            uint32_t h  = hist[qd][y];
            uint32_t ha = hist_acc[qd][y];
            ANS_ENC(ans, p_buf, h, ha);
        }
        
        ANS_ENC_FIN(ans, p_buf);
        
        reverseWords(p_buf_start, p_buf);
    }
    
    free(py_base);
    
    return p_buf - p_buf_base;
}


int QNBLICcompress (uint16_t *p_buf, UI8 *p_img, int height, int width) {
    if (height >= 512 && (height*width) > (512*512))                // use multithread only when image is large enough
        return QNBLICcompressMultiThread(p_buf, p_img, height, width);
    else
        return QNBLICcompressX          (p_buf, p_img, height, width);
}


#endif
