// fastfilters
// Copyright (c) 2016 Sven Peter
// sven.peter@iwr.uni-heidelberg.de or mail@svenpeter.me
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "fastfilters.h"
#include "common.h"
#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define FASTFILTERS_FIR_CONVOLVE_NOSIMD_IMPL_H

// yo dawg, i herd you like #includes so i put an #include in your #include so you can #include while you #include
#include <boost/preprocessor/library.hpp>

#define BOOST_PP_ITERATION_LIMITS (1, FF_UNROLL)
#define BOOST_PP_FILENAME_1 "fir_convolve_nosimd_impl.h"
#include BOOST_PP_ITERATE()
#undef BOOST_PP_ITERATION_LIMITS
#undef BOOST_PP_FILENAME_1

#define KERNEL_LEN_RUNTIME
#include "fir_convolve_nosimd_impl.h"
#undef KERNEL_LEN_RUNTIME

#define BORDER_MIRROR 0
#define BORDER_OPTIMISTIC 1

#define BORDER2STR_mirror(x) BOOST_PP_IF(BOOST_PP_EQUAL(x, BORDER_MIRROR), mirror, ~)
#define BORDER2STR(x)

#define IMPL_FNAME(z, n, text) &BOOST_PP_CAT(BOOST_PP_CAT(fir_convolve_impl_, text), BOOST_PP_INC(n))
#define IMPL_OUTER_FNAME(z, n, text) &BOOST_PP_CAT(BOOST_PP_CAT(fir_convolve_outer_impl_, text), BOOST_PP_INC(n))

#define TBLNAME_SYMMETRIC2(x) BOOST_PP_IF(x, symmetric, antisymmetric)
#define TBLNAME_SYMMETRIC(x) BOOST_PP_CAT(TBLNAME_SYMMETRIC2(x), _)

#define TBLNAME_BORDER2(x) BOOST_PP_CAT(border_, x)
#define TBLNAME_BORDER(x) BOOST_PP_CAT(TBLNAME_BORDER2(x), _)

#define N_BORDER_TYPES 3

#define ENUM_BORDER(x) BOOST_PP_CAT(border_enum_, x)

#define BOOST_PP_CAT5(a, b, c, d, e) BOOST_PP_CAT(BOOST_PP_CAT(BOOST_PP_CAT(a, b), BOOST_PP_CAT(c, d)), e)

#define DEFINE_JMPTBL(outer, left_border, right_border, symmetric)                                                     \
    static impl_fn_t BOOST_PP_CAT(                                                                                     \
        BOOST_PP_IF(outer, impl_fn_outer_, impl_fn_),                                                                  \
        BOOST_PP_CAT(TBLNAME_SYMMETRIC(symmetric),                                                                     \
                     BOOST_PP_CAT(TBLNAME_BORDER(left_border), TBLNAME_BORDER2(right_border))))[] = {                  \
        BOOST_PP_ENUM(FF_UNROLL, BOOST_PP_IF(outer, IMPL_OUTER_FNAME, IMPL_FNAME),                                     \
                      BOOST_PP_CAT(BOOST_PP_CAT(TBLNAME_BORDER(left_border), TBLNAME_BORDER(right_border)),            \
                                   TBLNAME_SYMMETRIC2(symmetric))),                                                    \
        BOOST_PP_CAT5(BOOST_PP_IF(outer, fir_convolve_outer_impl_, fir_convolve_impl_), TBLNAME_BORDER(left_border),   \
                      TBLNAME_BORDER(right_border), TBLNAME_SYMMETRIC2(symmetric), N)};

#define DECL_DEFINE_JMPTBL_INNER(z, n0, n1)                                                                            \
    DEFINE_JMPTBL(0, n0, n1, 0);                                                                                       \
    DEFINE_JMPTBL(0, n0, n1, 1);                                                                                       \
    DEFINE_JMPTBL(1, n0, n1, 0);                                                                                       \
    DEFINE_JMPTBL(1, n0, n1, 1);

#define DECL_DEFINE_JMPTBL_OUTER(z, n, text) BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_INNER, n)
BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_OUTER, ~);

struct impl_fn_jmptbl_selection {
    impl_fn_t *jmptbl;
    fastfilters_border_treatment_t left_border;
    fastfilters_border_treatment_t right_border;
    bool is_symmetric;
};

#define DEFINE_JMPTBL_STRUCT(outer, x, y, symmetric)                                                                   \
    {                                                                                                                  \
        .jmptbl = BOOST_PP_CAT(                                                                                        \
            BOOST_PP_IF(outer, impl_fn_outer_, impl_fn_),                                                              \
            BOOST_PP_CAT(TBLNAME_SYMMETRIC(symmetric), BOOST_PP_CAT(TBLNAME_BORDER(x), TBLNAME_BORDER2(y)))),          \
        .left_border = ENUM_BORDER(x), .right_border = ENUM_BORDER(y),                                                 \
        .is_symmetric = BOOST_PP_IF(symmetric, true, false)                                                            \
    }

#define DECL_DEFINE_JMPTBL_STRUCT_INNER(z, n0, n1)                                                                     \
    DEFINE_JMPTBL_STRUCT(0, n0, n1, 0), DEFINE_JMPTBL_STRUCT(0, n0, n1, 1),

#define DECL_DEFINE_JMPTBL_STRUCT_OUTER(z, n0, text)                                                                   \
    BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_STRUCT_INNER, n0)

#define DECL_DEFINE_JMPTBL_STRUCT_INNER2(z, n0, n1)                                                                    \
    DEFINE_JMPTBL_STRUCT(1, n0, n1, 0), DEFINE_JMPTBL_STRUCT(1, n0, n1, 1),

#define DECL_DEFINE_JMPTBL_STRUCT_OUTER2(z, n0, text)                                                                  \
    BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_STRUCT_INNER2, n0)

static const struct impl_fn_jmptbl_selection impl_fn_tbls_inner[] = {
    BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_STRUCT_OUTER, 0)};

static const struct impl_fn_jmptbl_selection impl_fn_tbls_outer[] = {
    BOOST_PP_REPEAT(N_BORDER_TYPES, DECL_DEFINE_JMPTBL_STRUCT_OUTER2, 0)};

static impl_fn_t find_fn(fastfilters_kernel_fir_t kernel, fastfilters_border_treatment_t left_border,
                         fastfilters_border_treatment_t right_border, const struct impl_fn_jmptbl_selection *jmptbls,
                         size_t n_jmptbls)
{
    if (kernel->len == 0)
        return NULL;

    impl_fn_t *jmptbl = NULL;

    for (unsigned int i = 0; i < n_jmptbls; ++i) {
        if (left_border != jmptbls[i].left_border)
            continue;
        if (right_border != jmptbls[i].right_border)
            continue;
        if (kernel->is_symmetric != jmptbls[i].is_symmetric)
            continue;
        jmptbl = jmptbls[i].jmptbl;
        break;
    }

    if (jmptbl == NULL)
        return NULL;

    if (kernel->len > FF_UNROLL)
        return jmptbl[FF_UNROLL];
    else
        return jmptbl[kernel->len - 1];
}

bool fastfilters_fir_convolve_fir_inner(const float *inptr, size_t n_pixels, size_t pixel_stride, size_t n_outer,
                                        size_t outer_stride, float *outptr, size_t outptr_stride,
                                        fastfilters_kernel_fir_t kernel, fastfilters_border_treatment_t left_border,
                                        fastfilters_border_treatment_t right_border, const float *borderptr_left,
                                        const float *borderptr_right, size_t border_outer_stride)
{
    impl_fn_t fn = NULL;

    if (unlikely(kernel->len == 0)) {
        if (fabs(kernel->coefs[0] - 1.0) > 1e-6)
            return false;

        if (inptr == outptr)
            return true;

        if (outer_stride != n_pixels * pixel_stride)
            return false;

        memcpy(outptr, inptr, outer_stride * n_outer * sizeof(float));
        return true;
    }

    if (unlikely(kernel->fn_inner_mirror == NULL)) {
        kernel->fn_inner_mirror = find_fn(kernel, FASTFILTERS_BORDER_MIRROR, FASTFILTERS_BORDER_MIRROR,
                                          impl_fn_tbls_inner, ARRAY_LENGTH(impl_fn_tbls_inner));
        kernel->fn_inner_optimistic = find_fn(kernel, FASTFILTERS_BORDER_OPTIMISTIC, FASTFILTERS_BORDER_OPTIMISTIC,
                                              impl_fn_tbls_inner, ARRAY_LENGTH(impl_fn_tbls_inner));
        kernel->fn_inner_ptr = find_fn(kernel, FASTFILTERS_BORDER_PTR, FASTFILTERS_BORDER_PTR, impl_fn_tbls_inner,
                                       ARRAY_LENGTH(impl_fn_tbls_inner));
    }

    if (likely(left_border == right_border)) {
        switch (left_border) {
        case FASTFILTERS_BORDER_MIRROR:
            fn = kernel->fn_inner_mirror;
            break;
        case FASTFILTERS_BORDER_OPTIMISTIC:
            fn = kernel->fn_inner_optimistic;
            break;
        case FASTFILTERS_BORDER_PTR:
            fn = kernel->fn_inner_ptr;
            break;
        default:
            fn = NULL;
            break;
        }
    }

    if (unlikely(fn == NULL))
        fn = find_fn(kernel, left_border, right_border, impl_fn_tbls_inner, ARRAY_LENGTH(impl_fn_tbls_inner));

    if (unlikely(fn == NULL))
        return false;

    return fn(inptr, borderptr_left, borderptr_right, n_pixels, pixel_stride, n_outer, outer_stride, outptr,
              outptr_stride, border_outer_stride, kernel);
}

bool fastfilters_fir_convolve_fir_outer(const float *inptr, size_t n_pixels, size_t pixel_stride, size_t n_outer,
                                        size_t outer_stride, float *outptr, size_t outptr_stride,
                                        fastfilters_kernel_fir_t kernel, fastfilters_border_treatment_t left_border,
                                        fastfilters_border_treatment_t right_border, const float *borderptr_left,
                                        const float *borderptr_right, size_t border_outer_stride)
{
    impl_fn_t fn = NULL;

    if (unlikely(kernel->len == 0)) {
        if (fabs(kernel->coefs[0] - 1.0) > 1e-6)
            return false;

        if (inptr == outptr)
            return true;

        // FIXME: memcpy here
        return false;
    }

    if (unlikely(kernel->fn_outer_mirror == NULL)) {
        kernel->fn_outer_mirror = find_fn(kernel, FASTFILTERS_BORDER_MIRROR, FASTFILTERS_BORDER_MIRROR,
                                          impl_fn_tbls_outer, ARRAY_LENGTH(impl_fn_tbls_outer));
        kernel->fn_outer_optimistic = find_fn(kernel, FASTFILTERS_BORDER_OPTIMISTIC, FASTFILTERS_BORDER_OPTIMISTIC,
                                              impl_fn_tbls_outer, ARRAY_LENGTH(impl_fn_tbls_outer));
        kernel->fn_outer_ptr = find_fn(kernel, FASTFILTERS_BORDER_PTR, FASTFILTERS_BORDER_PTR, impl_fn_tbls_outer,
                                       ARRAY_LENGTH(impl_fn_tbls_outer));
    }

    if (likely(left_border == right_border)) {
        switch (left_border) {
        case FASTFILTERS_BORDER_MIRROR:
            fn = kernel->fn_outer_mirror;
            break;
        case FASTFILTERS_BORDER_OPTIMISTIC:
            fn = kernel->fn_outer_optimistic;
            break;
        case FASTFILTERS_BORDER_PTR:
            fn = kernel->fn_outer_ptr;
            break;
        default:
            fn = NULL;
            break;
        }
    }

    if (unlikely(fn == NULL))
        fn = find_fn(kernel, left_border, right_border, impl_fn_tbls_outer, ARRAY_LENGTH(impl_fn_tbls_outer));

    if (unlikely(fn == NULL))
        return false;

    return fn(inptr, borderptr_left, borderptr_right, n_pixels, pixel_stride, n_outer, outer_stride, outptr,
              outptr_stride, border_outer_stride, kernel);
}