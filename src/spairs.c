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
 * \file spairs.c
 * \brief Implementation of handling of pair sets.
 *
 * \author Christian Eder <ederc@mathematik.uni-kl.de>
 */
#include "spairs.h"

inline ps_t *init_pair_set(gb_t *basis)
{
  ps_t *ps  = (ps_t *)malloc(sizeof(ps_t));
  ps->size  = 2 * basis->size;
  ps->pair  = (spair_t *)malloc(ps->size * sizeof(spair_t));
  ps->load  = 0;

  // generate spairs with the initial elements in basis

  return ps;
}

inline void free_pair_set_dynamic_data(ps_t *ps)
{
  free(ps->pair);
}
