/*
 * Copyright (c) 2007, 2008, 2009, 2010 Joseph Gaeddert
 * Copyright (c) 2007, 2008, 2009, 2010 Virginia Polytechnic
 *                                      Institute & State University
 *
 * This file is part of liquid.
 *
 * liquid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liquid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with liquid.  If not, see <http://www.gnu.org/licenses/>.
 */

//
// iirfilt : Infinite impulse response filter
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// defined:
//  IIRFILT()       name-mangling macro
//  TO              output type
//  TC              coefficients type
//  TI              input type
//  WINDOW()        window macro
//  DOTPROD()       dotprod macro
//  PRINTVAL()      print macro

// use structured dot product? 0:no, 1:yes
#define LIQUID_IIRFILT_USE_DOTPROD   (0)

struct IIRFILT(_s) {
    TC * b;             // numerator (feed-forward coefficients)
    TC * a;             // denominator (feed-back coefficients)
    TI * v;             // internal filter state (buffer)
    unsigned int n;     // filter length (order+1)

    unsigned int nb;    // numerator length
    unsigned int na;    // denominator length

    // filter structure type
    enum {
        IIRFILT_TYPE_NORM=0,
        IIRFILT_TYPE_SOS
    } type;

#if LIQUID_IIRFILT_USE_DOTPROD
    DOTPROD() dpb;      // numerator dot product
    DOTPROD() dpa;      // denominator dot product
#endif

    // second-order sections 
    IIRFILTSOS() * qsos;    // second-order sections filters
    unsigned int nsos;      // number of second-order sections
};

// create iirfilt (infinite impulse response filter) object
//  _b      :   numerator, feed-forward coefficients [size: _nb x 1]
//  _nb     :   length of numerator
//  _a      :   denominator, feed-back coefficients [size: _na x 1]
//  _na     :   length of denominator
IIRFILT() IIRFILT(_create)(TC * _b,
                           unsigned int _nb,
                           TC * _a,
                           unsigned int _na)
{
    // validate input
    if (_nb == 0) {
        fprintf(stderr,"error: iirfilt_xxxt_create(), numerator length cannot be zero\n");
        exit(1);
    } else if (_na == 0) {
        fprintf(stderr,"error: iirfilt_xxxt_create(), denominator length cannot be zero\n");
        exit(1);
    }

    // create structure and initialize
    IIRFILT() q = (IIRFILT()) malloc(sizeof(struct IIRFILT(_s)));
    q->nb = _nb;
    q->na = _na;
    q->n = (q->na > q->nb) ? q->na : q->nb;
    q->type = IIRFILT_TYPE_NORM;

    // allocate memory for numerator, denominator
    q->b = (TC *) malloc((q->na)*sizeof(TC));
    q->a = (TC *) malloc((q->nb)*sizeof(TC));

    // normalize coefficients to _a[0]
    TC a0 = _a[0];

    unsigned int i;
#if 0
    // read values in reverse order
    for (i=0; i<q->nb; i++)
        q->b[i] = _b[q->nb - i - 1];

    for (i=0; i<q->na; i++)
        q->a[i] = _a[q->na - i - 1];
#else
    for (i=0; i<q->nb; i++)
        q->b[i] = _b[i] / a0;

    for (i=0; i<q->na; i++)
        q->a[i] = _a[i] / a0;
#endif

    // create buffer and initialize
    q->v = (TI *) malloc((q->n)*sizeof(TI));

#if LIQUID_IIRFILT_USE_DOTPROD
    q->dpa = DOTPROD(_create)(q->a+1, q->na-1);
    q->dpb = DOTPROD(_create)(q->b,   q->nb);
#endif

    // reset internal state
    IIRFILT(_clear)(q);
    
    // return iirfilt object
    return q;
}

// create iirfilt (infinite impulse response filter) object based
// on second-order sections form
//  _B      :   numerator, feed-forward coefficients [size: _nsos x 3]
//  _A      :   denominator, feed-back coefficients [size: _nsos x 3]
//  _nsos   :   number of second-order sections
//
// NOTE: The number of second-order sections can be computed from the
// filter's order, n, as such:
//   r = n % 2
//   L = (n-r)/2
//   nsos = L+r
IIRFILT() IIRFILT(_create_sos)(TC * _B,
                               TC * _A,
                               unsigned int _nsos)
{
    // validate input
    if (_nsos == 0) {
        fprintf(stderr,"error: iirfilt_xxxt_create_sos(), filter must have at least one 2nd-order section\n");
        exit(1);
    }

    // create structure and initialize
    IIRFILT() q = (IIRFILT()) malloc(sizeof(struct IIRFILT(_s)));
    q->type = IIRFILT_TYPE_SOS;
    q->nsos = _nsos;
    q->qsos = (IIRFILTSOS()*) malloc( (q->nsos)*sizeof(IIRFILTSOS()) );
    q->n = _nsos * 2;

    // create coefficients array and copy over
    q->b = (TC *) malloc(3*(q->nsos)*sizeof(TC));
    q->a = (TC *) malloc(3*(q->nsos)*sizeof(TC));
    memmove(q->b, _B, 3*(q->nsos)*sizeof(TC));
    memmove(q->a, _A, 3*(q->nsos)*sizeof(TC));

    TC at[3];
    TC bt[3];
    unsigned int i,k;
    for (i=0; i<q->nsos; i++) {
        for (k=0; k<3; k++) {
            at[k] = q->a[3*i+k];
            bt[k] = q->b[3*i+k];
        }
        q->qsos[i] = IIRFILTSOS(_create)(bt,at);
        //q->qsos[i] = IIRFILT(_create)(q->b+3*i,3,q->a+3*i,3);
    }
    return q;
}


IIRFILT() IIRFILT(_create_prototype)(unsigned int _n)
{
    printf("warning: iirfilt_create_prototype(), not yet implemented\n");
    return NULL;
}

// destroy iirfilt object
void IIRFILT(_destroy)(IIRFILT() _q)
{
#if LIQUID_IIRFILT_USE_DOTPROD
    DOTPROD(_destroy)(_q->dpa);
    DOTPROD(_destroy)(_q->dpb);
#endif
    free(_q->b);
    free(_q->a);
    // if filter is comprised of cascaded second-order sections,
    // delete sub-filters separately
    if (_q->type == IIRFILT_TYPE_SOS) {
        unsigned int i;
        for (i=0; i<_q->nsos; i++)
            IIRFILTSOS(_destroy)(_q->qsos[i]);
        free(_q->qsos);
    } else {
        free(_q->v);
    }

    free(_q);
}

// print iirfilt object internals
void IIRFILT(_print)(IIRFILT() _q)
{
    printf("iir filter [%s]:\n", _q->type == IIRFILT_TYPE_NORM ? "normal" : "sos");
    unsigned int i;

    if (_q->type == IIRFILT_TYPE_SOS) {
        for (i=0; i<_q->nsos; i++)
            IIRFILTSOS(_print)(_q->qsos[i]);
    } else {

        printf("  b :");
        for (i=0; i<_q->nb; i++)
            PRINTVAL_TC(_q->b[i],%12.8f);
        printf("\n");

        printf("  a :");
        for (i=0; i<_q->na; i++)
            PRINTVAL_TC(_q->a[i],%12.8f);
        printf("\n");

#if 0
        printf("  v :");
        for (i=0; i<_q->n; i++)
            PRINTVAL(_q->v[i]);
        printf("\n");
#endif
    }
}

// clear
void IIRFILT(_clear)(IIRFILT() _q)
{
    unsigned int i;

    if (_q->type == IIRFILT_TYPE_SOS) {
        // clear second-order sections
        for (i=0; i<_q->nsos; i++) {
            IIRFILTSOS(_clear)(_q->qsos[i]);
        }
    } else {
        // set internal buffer to zero
        for (i=0; i<_q->n; i++)
            _q->v[i] = 0;
    }
}

// execute normal iir filter using traditional numerator/denominator
// form (not second-order sections form)
//  _q      :   iirfilt object
//  _x      :   input sample
//  _y      :   output sample
void IIRFILT(_execute_norm)(IIRFILT() _q,
                            TI _x,
                            TO *_y)
{
    unsigned int i;

    // advance buffer
    for (i=_q->n-1; i>0; i--)
        _q->v[i] = _q->v[i-1];

#if LIQUID_IIRFILT_USE_DOTPROD
    // compute new v
    TI v0;
    DOTPROD(_execute)(_q->dpa, _q->v+1, & v0);
    v0 = _x - v0;
    _q->v[0] = v0;

    // compute new y
    DOTPROD(_execute)(_q->dpb, _q->v, _y);
#else
    // compute new v
    TI v0 = _x;
    for (i=1; i<_q->na; i++)
        v0 -= _q->a[i] * _q->v[i];
    _q->v[0] = v0;

    // compute new y
    TO y0 = 0;
    for (i=0; i<_q->nb; i++)
        y0 += _q->b[i] * _q->v[i];

    // set return value
    *_y = y0;
#endif
}

// execute iir filter using second-order sections form
//  _q      :   iirfilt object
//  _x      :   input sample
//  _y      :   output sample
void IIRFILT(_execute_sos)(IIRFILT() _q,
                           TI _x,
                           TO *_y)
{
    TI t0 = _x;     // intermediate input
    TO t1 = 0.;     // intermediate output
    unsigned int i;
    for (i=0; i<_q->nsos; i++) {
        // run each filter separately
        IIRFILTSOS(_execute)(_q->qsos[i], t0, &t1);

        // output for filter n becomes input to filter n+1
        t0 = t1;
    }
    *_y = t1;
}

// execute iir filter, switching to type-specific function
//  _q      :   iirfilt object
//  _x      :   input sample
//  _y      :   output sample
void IIRFILT(_execute)(IIRFILT() _q,
                       TI _x,
                       TO *_y)
{
    if (_q->type == IIRFILT_TYPE_NORM)
        IIRFILT(_execute_norm)(_q,_x,_y);
    else
        IIRFILT(_execute_sos)(_q,_x,_y);
}

// get filter length (order + 1)
unsigned int IIRFILT(_get_length)(IIRFILT() _q)
{
    return _q->n;
}
