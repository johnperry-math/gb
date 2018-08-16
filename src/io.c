/* gb: Gröbner Basis
 * Copyright (C) 2018 Christian Eder <ederc@mathematik.uni-kl.de>
 * This file is part of gb.
 * gbla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * gbla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with gbla . If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file io.c
 * \brief Input and output handling
 *
 * \author Christian Eder <ederc@mathematik.uni-kl.de>
 */

#include "io.h"

void set_function_pointers(
        const stat_t *st
        )
{
    /* todo: this needs to be generalized for different monomial orders */
    switch (st->mon_order) {
        case 0:
            matrix_row_initial_input_cmp  =
                matrix_row_initial_input_cmp_drl;
            monomial_cmp        = monomial_cmp_drl;
            spair_cmp           = spair_cmp_drl;
            hcm_cmp             = hcm_cmp_pivots_drl;
            break;
        case 1:
            matrix_row_initial_input_cmp  =
                matrix_row_initial_input_cmp_lex;
            monomial_cmp        = monomial_cmp_lex;
            spair_cmp           = spair_cmp_deglex;
            hcm_cmp             = hcm_cmp_pivots_lex;
            break;
        default:
            matrix_row_initial_input_cmp  =
                matrix_row_initial_input_cmp_drl;
            monomial_cmp        = monomial_cmp_drl;
            spair_cmp           = spair_cmp_drl;
            hcm_cmp             = hcm_cmp_pivots_drl;
    }

    switch (st->la_variant) {
        case 1:
            linear_algebra  = exact_sparse_dense_linear_algebra;
            break;
        case 2:
            linear_algebra  = exact_sparse_linear_algebra;
            break;
        case 42:
            linear_algebra  = probabilistic_sparse_dense_linear_algebra;
            break;
        case 43:
            linear_algebra  = probabilistic_sparse_dense_linear_algebra_2;
            break;
        case 44:
            linear_algebra  = probabilistic_sparse_linear_algebra;
            break;
        default:
            linear_algebra  = exact_sparse_dense_linear_algebra;
    }

    /* todo: need to add support for rationals here */
    if (st->field_char < pow(2, 16)) {
        import_julia_data = import_julia_data_16;
    } else {
        if (st->field_char < pow(2, 31)) {
            import_julia_data = import_julia_data_32;
        }
    }

    /* up to 17 bits we can use one modular operation for reducing a row. this works
     * for matrices with #rows <= 54 million */
    if (st->field_char < pow(2, 17)) {
        reduce_dense_row_by_all_pivots =
            reduce_dense_row_by_all_pivots_17_bit;
        reduce_dense_row_by_known_pivots =
            reduce_dense_row_by_known_pivots_17_bit;
        reduce_dense_row_by_known_pivots_sparse =
            reduce_dense_row_by_known_pivots_sparse_17_bit;
        reduce_dense_row_by_dense_new_pivots  =
            reduce_dense_row_by_dense_new_pivots_17_bit;
    } else {
        reduce_dense_row_by_all_pivots =
            reduce_dense_row_by_all_pivots_31_bit;
        reduce_dense_row_by_known_pivots =
            reduce_dense_row_by_known_pivots_31_bit;
        reduce_dense_row_by_known_pivots_sparse =
            reduce_dense_row_by_known_pivots_sparse_31_bit;
        reduce_dense_row_by_dense_new_pivots  =
            reduce_dense_row_by_dense_new_pivots_31_bit;
    }
}

int32_t check_and_set_meta_data(
        stat_t *st;
        const int32_t *lens,
        const int32_t *cfs,
        const int32_t *exps,
        const int32_t field_char,
        const int32_t mon_order,
        const int32_t nr_vars,
        const int32_t nr_gens,
        const int32_t ht_size,
        const int32_t nr_threads,
        const int32_t max_nr_pairs,
        const int32_t regenerate_ht,
        const int32_t la_option,
        const int32_t info_level
        )
{
    if (nr_gens <= 0
            || nr_vars <= 0
            || field_char <= 0
            || lens == NULL
            || cfs == NULL
            || exps == NULL) {
        return 1;
    }

    st->nr_vars = nr_vars;
    /* note: prime check should be done in julia */
    st->field_char  = field_char;
    /* monomial order */
    if (mon_order != 0 && mon_order != 1) {
        st->mon_order = 0;
    } else {
        st->mon_order = mon_order;
    }
    /* set hash table size */
    st->init_ht_sz  = htes <= 0 ? 12 : htes;
    st->info_level  = info_level;
    if (st->info_level < 0) {
        st->info_level  = 0;
    }
    if (st->info_level > 2) {
        st->info_level  = 2;
    }

    /* set number of threads */
    st->nthrds  = nr_threads <= 0 ? 1 : nr_threads;

    /* resetting the global hash table? */
    st->regen_ht  = regenerate_ht <= 0 ? -1 : reset_hash_table;

    /* maximal number of pairs per matrix */
    st->max_nr_pairs  = max_nr_pairs <= 0 ? 2147483647 : max_nr_pairs;

    /* set linear algebra option */
    st->la_variant  = la_option <= 0 ? 1 : la_option;

    set_function_pointers(st);

    return 0;
}

/* note that depending on the input data we set the corresponding
 * function pointers for monomial resp. spair comparisons, taking
 * spairs by a given minimal property for symbolic preprocessing, etc. */
void import_julia_data_16(
        bs_t *bs,
        ht_t *ht,
        mat_t *mat,
        const int32_t *const lens,
        void *cfs_julia,
        const int32_t *const exps,
        const int32_t nr_gens
        )
{
    int32_t i, j;
    len_t k;
    int32_t off = 0; /* offset in arrays */
    const cf16_t *const cfs = (cf16_t *)cfs_julia;
    const len_t nv  = ht->nv;

    exp_t *e  = (exp_t *)malloc((unsigned long)nv* sizeof(exp_t));

    for (i = 0; i < nr_gens; ++i) {
        /* each matrix row has the following structure:
         * [length | offset | eh1 | cf1 | eh2 | cf2 | .. | ehl | cfl]
         * where piv? is a label for being a known or an unknown pivot */
        bs->hd[i]   = (hl_t *)malloc(((unsigned long)lens[i]+3) * sizeof(dt_t));
        bs.>cf[i]   = (cf16_t *)malloc(
                (unsigned long)(lens[i]+3) * sizeof(cf16_t));
        bs->hd[i][0]  = i; /* link to coefficient array */
        bs->cf[i][0]  = 0; /* not redundant */
        bs->hd[i][1]  = (lens[i] % UNROLL) + 3; /* offset */
        bs->cf[i][1]  = bs->hd[i][1];
        bs->hd[i][2]  = lens[i]+3; /* length */
        bs->cf[i][2]  = bs->hd[i][2];
        if (ht->eld+lens[i] >= ht->esz) {
            enlarge_hash_table(ht);
        }
        for (j = off; j < off+lens[i]; ++j) {
            for (k = 0; k < nv; ++k) {
                e[k]  = (exp_t)(exps+(nvars*j))[k];
            }
            bs->hd[i][j+3-off]  = insert_in_hash_table(e);
            bs->cf[i][j+3-off]  = cfs[j];
        }
        /* mark initial generators, they have to be added to the basis first */
        off +=  lens[i];
    }
    /* needed for normalizing input elements and adding them to
     * the basis as starting point for f4 */
    mat->np = mat->nr = mat->nr = nr_gens;

    free(e);
}

void import_julia_data_32(
        bs_t *bs,
        ht_t *ht,
        mat_t *mat,
        const int32_t *const lens,
        void *cfs_julia,
        const int32_t *const exps,
        const int32_t nr_gens
        )
{
    int32_t i, j;
    len_t k;
    int32_t off = 0; /* offset in arrays */
    const cf32_t *const cfs = (cf32_t *)cfs_julia;
    const len_t nv  = ht->nv;

    exp_t *e  = (exp_t *)malloc((unsigned long)nv* sizeof(exp_t));

    for (i = 0; i < nr_gens; ++i) {
        /* each matrix row has the following structure:
         * [length | offset | eh1 | cf1 | eh2 | cf2 | .. | ehl | cfl]
         * where piv? is a label for being a known or an unknown pivot */
        bs->hd[i]   = (hl_t *)malloc(((unsigned long)lens[i]+3) * sizeof(dt_t));
        bs.>cf[i]   = (cf32_t *)malloc(
                (unsigned long)(lens[i]+3) * sizeof(cf32_t));
        bs->hd[i][0]  = i; /* link to coefficient array */
        bs->cf[i][0]  = 0; /* not redundant */
        bs->hd[i][1]  = (lens[i] % UNROLL) + 3; /* offset */
        bs->cf[i][1]  = bs->hd[i][1];
        bs->hd[i][2]  = lens[i]+3; /* length */
        bs->cf[i][2]  = bs->hd[i][2];
        if (ht->eld+lens[i] >= ht->esz) {
            enlarge_hash_table(ht);
        }
        for (j = off; j < off+lens[i]; ++j) {
            for (k = 0; k < nv; ++k) {
                e[k]  = (exp_t)(exps+(nvars*j))[k];
            }
            bs->hd[i][j+3-off]  = insert_in_hash_table(e);
            bs->cf[i][j+3-off]  = cfs[j];
        }
        /* mark initial generators, they have to be added to the basis first */
        off +=  lens[i];
    }
    /* needed for normalizing input elements and adding them to
     * the basis as starting point for f4 */
    mat->np = mat->nr = mat->nr = nr_gens;

    free(e);
}

int64_t export_julia_data(
        int32_t **bp
        )
{
    len_t i, j, k;
    int64_t ctr_lengths, ctr_elements;
    int32_t *basis  = *bp;

    int64_t len = 0; /* complete length of exported array */
    int64_t nb  = 0; /* # elemnts in basis */

    const len_t lterm = 1 + nvars; /* length of a term */

    /* compute number of terms */
    for (i = 0; i < bload; ++i) {
        if (gbcf[i][0]) {
            continue;
        } else {
            len +=  (int64_t)gbcf[i][2]-3;
            nb++;
        }
    }

    /* compute the length considering the number of variables per exponent */
    len = len * (int64_t)lterm;
    /* add storage for length of each element */
    len = len + nb;
    /* add storage for number of generators in basis */
    len++;

    basis  = (int32_t *)malloc((unsigned long)len * sizeof(int32_t));

    if (nb > (int64_t)(pow(2, 31))) {
        printf("basis too big\n");
        return 0;
    }

    ctr_lengths   = 1;
    ctr_elements  = (int64_t)nb + 1;

    basis[0]  = (int32_t)nb;
    /* basis[1]  = (int32_t)nb; */
    for (i = 0; i < bload; ++i) {
        if (gbcf[gbdt[i][0]][0]) {
            continue;
        } else {
            /* length of polynomial including this length entry itself */
            basis[ctr_lengths++]  = (int32_t)((gbdt[i][2]-3) * lterm);
            for (j = 3; j < gbdt[i][2]; ++j) {
                basis[ctr_elements++] = (int32_t)gbcf[gbdt[i][0]][j]; /* coefficient */
                for (k = 0; k < nvars; ++k) {
                    basis[ctr_elements++] = (int32_t)ev[gbdt[i][j]][k];
                }
            }
        }
    }
    *bp = basis;

    return len;
}
