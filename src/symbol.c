/* gb: Gröbner Basis
 * Copyright (C) 2015 Christian Eder <ederc@mathematik.uni-kl.de>
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
 * \file symbol.c
 * \brief Implementation of the symbolic pre- and postprocessing in gb.
 *
 * \author Christian Eder <ederc@mathematik.uni-kl.de>
 */

#include "symbol.h"

spd_t *symbolic_preprocessing(ps_t *ps, const gb_t *basis, const gb_t *sf)
{
  nelts_t i, idx, last_div, nsel, nlcm;
  hash_t hash_pos;

  /* clears hash table index: there we store during symbolic preprocessing a 2
   * if it is a lead monomial and 1 if it is not a lead monomial. all other
   * entries keep 0, thus they are not part of this reduction step */

  /* clear_hash_table_idx(ht); */

  nsel  = ht->sort.get_pairs_by_minimal_degree(ps);

  /* for (size_t k = 0; k < ps->load; ++k)
   *   printf("%u | %u %u | deg %u\n", k, ps->pairs[k].gen1, ps->pairs[k].gen2, ps->pairs[k].deg); */

  /* check if limit for spair handling is set */
  if (basis->max_sel != 0) {
    while (nsel > basis->max_sel)
      nsel  = nsel / 2;
    /* nsel  = nsel < basis->max_sel ? nsel : nsebasis->max_sel; */
    /* try to keep all spairs of same lcm together */
    while (nsel != ps->load && ps->pairs[nsel-1].lcm == ps->pairs[nsel].lcm)
      nsel++;
  }
  /* compute number of different lcms in pair selection
   * => we know the number of elements for the lower matrix part:
   *    2 * nsel - nlcm */
  nlcm  = 0;
  for (size_t j = 1; j < nsel; ++j) {
    if (ps->pairs[j-1].lcm != ps->pairs[j].lcm) {
      ++nlcm;
    }
  }
  
  meta_data->sel_pairs  = nsel;
  meta_data->curr_deg   = ps->pairs[0].deg;

  /* list of monomials that appear in the matrix */
  pre_t *mon      = init_preprocessing_hash_list(20*nsel*basis->nt[ps->pairs[0].gen2]);
  /* the lower part of the gbla matrix resp. the selection is fixed:
   * those are just the second generators of the spairs, thus we need nsel
   * places. */

  /* NOTE: We allocate enough memory for sel_low and sel_upp such that we do
  *       not need to reallocate during select_pairs. Later on we might need
  *       to reallocated memory for sel_upp, sel_low is fixed after pair
  *       selection. */
  sel_t *sel_low  = init_selection(2*nsel - nlcm);
  sel_t *sel_upp  = init_selection(20*nsel);
  sel_upp->deg    = ps->pairs[0].deg;
  sel_low->deg    = ps->pairs[0].deg;
  /* list of polynomials and their multipliers */
  select_pairs(ps, sel_upp, sel_low, mon, basis, sf, nsel);

  /* we use mon as LIFO: last in, first out. thus we can easily remove and add
   * new elements to mon */
  idx = 0;
  nelts_t hio;
  /* hash_t h, ho; */
  while (idx < mon->load) {
    /* printf("mon_load %u\n", mon->load); */
    hash_pos  = mon->hpos[idx];
    /* only if not already a lead monomial, e.g. if coming from spair */
    if (ht->idx[hash_pos] != 2) {
      last_div  = ht->div[hash_pos];

      /* takes last element in basis and test for division, goes down until we
       * reach the last known divisor last_div */
      hio = 0;
      /* ho  = 0; */
      /* if (last_div > 0 && basis->red[last_div] == 0) {
       *   hio = last_div;
       *   goto done;
       * }
       * i   = last_div == 0 ? basis->st : last_div; */

#if 1
      /* max value for an unsigned data type in order to ensure that the first
       * polynomial is taken */
      /* nelts_t nto = UINT32_MAX; */

      /* if (last_div != 0) {
       *   if (basis->red[i] == 0) {
       *     hio = i;
       *     ho  = get_multiplier(hash_pos, basis->eh[i][0], ht);
       *     nto = basis->nt[i];
       *     goto done;
       *   } else {
       *     i++;
       *   }
       * } */

      if (last_div > 0) {
        i = last_div;
      } else {
        i = basis->st;
      }
      while (i<basis->load) {
        if (check_monomial_division(hash_pos, basis->eh[i][0], ht)) {
          while (basis->red[i] != 0)
            i = basis->red[i];
          hio = i;
          ht->div[hash_pos]  = hio;
          goto done;
        }
        i++;
      }
#else
      nelts_t b = i;
      hio = 0;
      /* max value for an unsigned data type in order to ensure that the first
       * polynomial is taken */
      for (i=basis->load; i>b; --i) {
        if (basis->red[i-1] == 0 && check_monomial_division(hash_pos, basis->eh[i-1][0], ht)) {
          /* h = get_multiplier(hash_pos, basis->eh[i-1][0], ht); */
          hio = i-1;
          ho  = h;
          break;
        }
      }
#endif
      ht->div[hash_pos]  = i;
      idx++;
      continue;

done:
      /* ho = get_multiplier(hash_pos, basis->eh[hio][0], ht); */
      mon->nlm++;
      /* printf("this is the reducer finally taken %3u\n",hio); */
      /* if (ht->div[hash_pos] != hio)
       *   printf("hdiv changed: %u --> %u\n", ht->div[hash_pos], hio); */
      /* if multiple is not already in the selected list
       * we have found another element with such a monomial, since we do not
       * take care of the lead monomial below when entering the other lower
       * order monomials, we have to adjust the idx for this given monomial
       * here.
       * we have reducer, i.e. the monomial is a leading monomial (important for
       * splicing matrix later on */
      ht->idx[hash_pos] = 2;
      if (sel_upp->load == sel_upp->size)
        adjust_size_of_selection(sel_upp, 2*sel_upp->size);
      sel_upp->mpp[sel_upp->load].bi  = hio;
      sel_upp->mpp[sel_upp->load].sf  = 0;
      sel_upp->mpp[sel_upp->load].mlm = hash_pos;
      sel_upp->mpp[sel_upp->load].mul = get_multiplier(hash_pos, basis->eh[hio][0], ht);
#if 0
      sel_upp->mpp[sel_upp->load].nt  = basis->nt[hio];
      sel_upp->mpp[sel_upp->load].eh  = basis->eh[hio];
      sel_upp->mpp[sel_upp->load].cf  = basis->cf[hio];
#endif
      sel_upp->load++;

      if (mon->size-mon->load+1 < basis->nt[sel_upp->mpp[sel_upp->load-1].bi]) {
        const nelts_t max = 2*mon->size > basis->nt[sel_upp->mpp[sel_upp->load-1].bi] ?
          2*mon->size : basis->nt[sel_upp->mpp[sel_upp->load-1].bi];
        adjust_size_of_preprocessing_hash_list(mon, max);
      }
#define SIMPLIFY  1
#if SIMPLIFY
      /* function pointer set correspondingly if simplify option is set or not */
      ht->sf.simplify(&sel_upp->mpp[sel_upp->load-1], basis, sf);
      /* try_to_simplify(&sel_upp->mpp[sel_upp->load-1], basis, sf); */
      /* now add new monomials to preprocessing hash list */
      if (sel_upp->mpp[sel_upp->load-1].sf > 0) {
        enter_monomial_to_preprocessing_hash_list(
            /* sel_upp->mpp[sel_upp->load-1], */
            sel_upp->mpp[sel_upp->load-1].mul,
            sel_upp->mpp[sel_upp->load-1].sf,
            sf,
            mon,
            ht);
      } else {
        enter_monomial_to_preprocessing_hash_list(
            /* sel_upp->mpp[sel_upp->load-1], */
            sel_upp->mpp[sel_upp->load-1].mul,
            sel_upp->mpp[sel_upp->load-1].bi,
            basis,
            mon,
            ht);
      }
#else
      enter_monomial_to_preprocessing_hash_list(
          /* sel_upp->mpp[sel_upp->load-1], */
          sel_upp->mpp[sel_upp->load-1].mul,
          sel_upp->mpp[sel_upp->load-1].bi,
          basis,
          mon,
          ht);
#endif
    }
    idx++;
  }

  /* next we store the information needed to construct the GBLA matrix in the
   * following */
  spd_t *mat  = (spd_t *)malloc(sizeof(spd_t));

  /* adjust memory */
  adjust_size_of_selection(sel_upp, sel_upp->load);
  adjust_size_of_preprocessing_hash_list(mon, mon->load);
#if SYMBOL_DEBUG
  for (int ii=0; ii<sel_low->load; ++ii) {
    for (int jj=0; jj<ht->nv; ++jj) {
      printf("%u ",ht->exp[sel_low->mpp[ii].mul][jj]);
    }
    printf(" || ");
    for (int jj=0; jj<ht->nv; ++jj) {
      printf("%u ",ht->exp[basis->eh[sel_low->mpp[ii].bi][0]][jj]);
    }
    printf(" ||| ");
    for (int jj=0; jj<ht->nv; ++jj) {
      printf("%u ",ht->exp[sel_low->mpp[ii].mul][jj] + ht->exp[basis->eh[sel_low->mpp[ii].bi][0]][jj]);
    }
    printf("\n");
  }
#endif
  mat->selu = sel_upp;
  mat->sell = sel_low;
  mat->col  = mon;

  return mat;
}
