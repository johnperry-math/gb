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

#include "io.h"

/*  ========== TIMINGS and MEMORY PRINTING ========== */
double walltime(struct timeval t_start)
{
	struct timeval t_end;
	gettimeofday(&t_end, NULL);
	return (double)((t_end.tv_sec - t_start.tv_sec) * 1000000 + t_end.tv_usec - t_start.tv_usec);
}

void print_mem_usage()
{
  char    *unit = "KB";
  double  vms   = 0.0; /*  virtual memory size */
  double  rss   = 0.0; /*  resident set size */
  /*  possibly x86-64 is configured to use 2MB pages */
  long    page_size_kb  = sysconf(_SC_PAGE_SIZE) / 1024;

  unsigned long _vms = 0;
  long          _rss = 0;
  /*  get memory usage from 'file' stat which is not perfect, but gives most */
  /*  reliable information. */
  /*  Note: This corresponds to Martani's memory usage printing in his LELA */
  /*  implementation, thus it is used for comparison reasons. It might be changed */
  /*  in later versions of the gb. */
  const char *fn  = "/proc/self/stat";
  FILE *fh        = fopen(fn,"r");

  /*  dummy vars for leading entries in /proc/self/stat we are not interested in */
  char pid[1024] = "\0", comm[1024] ="\0", state[1024] ="\0", ppid[1024] ="\0", pgrp[1024] ="\0", session[1024] ="\0", tty_nr[1024] ="\0";
  char tpgid[1024] ="\0", flags[1024] ="\0", minflt[1024] ="\0", cminflt[1024] ="\0", majflt[1024] ="\0", cmajflt[1024] ="\0";
  char utime[1024] ="\0", stime[1024] ="\0", cutime[1024] ="\0", cstime[1024] ="\0", priority[1024] ="\0", nice[1024] ="\0";
  char nthrds[1024] ="\0", itrealvalue[1024] ="\0", starttime[1024] ="\0";

  /*  dummy reading of useless information */
  fscanf(fh, "%1023s", &pid[0]);
  fscanf(fh, "%1023s", &comm[0]);
  fscanf(fh, "%1023s", &state[0]);
  fscanf(fh, "%1023s", &ppid[0]);
  fscanf(fh, "%1023s", &pgrp[0]);
  fscanf(fh, "%1023s", &session[0]);
  fscanf(fh, "%1023s", &tty_nr[0]);
  fscanf(fh, "%1023s", &tpgid[0]);
  fscanf(fh, "%1023s", &flags[0]);
  fscanf(fh, "%1023s", &minflt[0]);
  fscanf(fh, "%1023s", &cminflt[0]);
  fscanf(fh, "%1023s", &majflt[0]);
  fscanf(fh, "%1023s", &cmajflt[0]);
  fscanf(fh, "%1023s", &utime[0]);
  fscanf(fh, "%1023s", &stime[0]);
  fscanf(fh, "%1023s", &cutime[0]);
  fscanf(fh, "%1023s", &cstime[0]);
  fscanf(fh, "%1023s", &priority[0]);
  fscanf(fh, "%1023s", &nice[0]);
  fscanf(fh, "%1023s", &nthrds[0]);
  fscanf(fh, "%1023s", &itrealvalue[0]);
  fscanf(fh, "%1023s", &starttime[0]);

  /*  get real memory information */
  fscanf(fh, "%lu", &_vms);
  fscanf(fh, "%ld", &_rss);

  /*  close file */
  fclose(fh);

  /*  TODO: How to read /proc/self/stat ??? */

  vms = _vms / 1024.0;
  rss = _rss * page_size_kb;

  /*  MB ? */
  if (vms > 1024) {
    vms   = vms/1024.0;
    rss   = rss/1024.0;
    unit  = "MB";
  }
  /*  GB ? */
  if (vms > 1024) {
    vms   = vms/1024.0;
    rss   = rss/1024.0;
    unit  = "GB";
  }
  /*  TB ? Just joking! */
  if (vms > 1024) {
    vms   = vms/1024.0;
    rss   = rss/1024.0;
    unit  = "TB";
  }
  printf("MMRY\tRSS - %.3f %s | VMS - %.3f %s\n", rss, unit, vms, unit);
}

inline char *get_variable_name(const char *line, char **prev_pos)
{
  const char comma_splicer  = ',';

  char *tmp_var   = (char *)malloc(50 * sizeof(char));
  char *curr_pos  = strchr(*prev_pos, comma_splicer);
  if (curr_pos != NULL) {
    int var_diff   = (int)(curr_pos - *prev_pos);
    memcpy(tmp_var, *prev_pos, var_diff);
    tmp_var[var_diff] = '\0';
    *prev_pos = curr_pos+1;
  } else { // we are at the last variable
    int prev_idx  = (int)(*prev_pos - line);
    int curr_idx  = (int)(strlen(line)+1);
    int var_diff  = curr_idx - prev_idx;
    memcpy(tmp_var, *prev_pos, var_diff);
    tmp_var[var_diff] = '\0';
  }

  // trim variable, remove blank spaces
  char *tmp_var_begin, *tmp_var_end;
  tmp_var_begin = tmp_var_end =  tmp_var;
  while (isspace(*tmp_var_begin))
    tmp_var_begin++;
  if (*tmp_var_begin != 0) {
    while (*tmp_var_end)
      tmp_var_end++;
    do {
      tmp_var_end--;
    } while (isspace(*tmp_var_end));
  }
  int final_length  = (int)(tmp_var_end - tmp_var_begin + 1);
  char *final_var   = (char *)malloc((final_length+1) * sizeof(char));
  memcpy(final_var, tmp_var_begin, final_length);
  final_var[final_length] = '\0';
  
  free(tmp_var);

  return final_var;
}

inline int get_number_of_terms(const char *line)
{
  const char add_splicer    = '+';
  const char minus_splicer  = '-';
  char *tmp;
  int nterms  = 1;
  tmp = strchr(line, add_splicer);
  while (tmp != NULL) {
    nterms++;
    tmp = strchr(tmp+1, add_splicer);
  }
  tmp = strchr(line, minus_splicer);
  while (tmp != NULL) {
    nterms++;
    tmp = strchr(tmp+1, minus_splicer);
  }

  return nterms;
}
#if __GB_HAVE_SSE2
inline void store_exponent(const char *term, const gb_t *basis, mp_cf4_ht_t *ht)
{
  nvars_t k;
  exp_t *expv = (exp_t *)calloc(ht->nev * ht->vl, sizeof(exp_t));
  const char mult_splicer = '*';
  const char exp_splicer  = '^';
  exp_t exp = 0;
  deg_t deg = 0;

  for (k=0; k<basis->nv; ++k) {
    exp = 0;
    char *var = strstr(term, basis->vnames[k]);
    var   = strtok(var, "\n");
    var   = strtok(var, ",");
    if (var != NULL) {
      // if the next variable follows directly => exp = 1
      if (strncmp(&mult_splicer, var+strlen(basis->vnames[k]), 1) == 0) {
        exp = 1;
      } else {
        // if there follows an exp symbol "^"
        if (strncmp(&exp_splicer, var+strlen(basis->vnames[k]), 1) == 0) {
          char exp_str[100];
          char *mult_pos;
          mult_pos  = strchr(var, mult_splicer);
          if (mult_pos != NULL) {
            int exp_len = (int)(mult_pos - (var+strlen(basis->vnames[k])) - 1);
            memcpy(exp_str, var+strlen(basis->vnames[k])+1, exp_len);
            exp_str[exp_len] = '\0';
            exp = (exp_s)strtol(exp_str, NULL, 10);
          } else { // no further variables in this term
            int exp_len = (int)((var+strlen(var)) + 1 - (var+strlen(basis->vnames[k])) - 1);
            memcpy(exp_str, var+strlen(basis->vnames[k])+1, exp_len);
            exp = (exp_s)strtol(exp_str, NULL, 10);
            exp_str[exp_len] = '\0';
          }
        } else { // we are at the last variable with exp = 1
          if (strcmp(basis->vnames[k], var) == 0)
            exp = 1;
          else
            continue;
        }
      }
    }
    // if we use graded reverse lexicographical ordering (basis->ord = 0) we store
    // the exponents in reverse order so that we can use memcmp to sort the terms
    // efficiently later on
    if (basis->ord == 0)
      deg +=  expv[ht->nv-1-k] = exp;
    else
      deg +=  expv[k] = exp;
  }
  for (k=0; k<ht->nev; ++k)
    ht->ev[ht->load][k]  = _mm_load_si128((__m128i *)expv+(k*ht->vl));
  ht->deg[ht->load] = deg;
  free(expv);
}
#endif
inline exp_t get_exponent(const char *term, const char *var_name)
{
  const char mult_splicer = '*';
  const char exp_splicer  = '^';
  exp_t exp = 0;

  char *var = strstr(term, var_name);
  var   = strtok(var, "\n");
  var   = strtok(var, ",");
  if (var != NULL) {
    // if the next variable follows directly => exp = 1
    if (strncmp(&mult_splicer, var+strlen(var_name), 1) == 0) {
      exp = 1;
    } else {
      // if there follows an exp symbol "^"
      if (strncmp(&exp_splicer, var+strlen(var_name), 1) == 0) {
        char exp_str[100];
        char *mult_pos;
        mult_pos  = strchr(var, mult_splicer); 
        if (mult_pos != NULL) {
          int exp_len = (int)(mult_pos - (var+strlen(var_name)) - 1);
          memcpy(exp_str, var+strlen(var_name)+1, exp_len);
          exp_str[exp_len] = '\0';
          exp = (exp_t)strtol(exp_str, NULL, 10);
        } else { // no further variables in this term
          int exp_len = (int)((var+strlen(var)) + 1 - (var+strlen(var_name)) - 1);
          memcpy(exp_str, var+strlen(var_name)+1, exp_len);
          exp = (exp_t)strtol(exp_str, NULL, 10);
          exp_str[exp_len] = '\0';
        }
      } else { // we are at the last variable with exp = 1
        if (strcmp(var_name, var) == 0)
          exp = 1;
      }
    }
  }
  return exp;
}

inline void get_term(const char *line, char **prev_pos,
    char **term)
{
  // note that maximal term length we handle
  const char add_splicer    = '+';
  const char minus_splicer  = '-';

  char *start_pos;
  char *curr_pos_add    = strchr(*prev_pos, add_splicer);
  char *curr_pos_minus  = strchr(*prev_pos, minus_splicer);

  if (*prev_pos != line)
    start_pos = *prev_pos - 1;
  else
    start_pos = *prev_pos;

  if (curr_pos_add != NULL && curr_pos_minus != NULL) {
    int term_diff_add   = (int)(curr_pos_add - start_pos);
    int term_diff_minus = (int)(curr_pos_minus - start_pos);
    // if minus is nearer
    if (term_diff_add > term_diff_minus) {
      memcpy(*term, start_pos, term_diff_minus);
      (*term)[term_diff_minus]  = '\0';
      *prev_pos                 = curr_pos_minus+1;
      return;
    // if plus is nearer
    } else {
      memcpy(*term, start_pos, term_diff_add);
      (*term)[term_diff_add]  = '\0';
      *prev_pos               = curr_pos_add+1;
      return;
    }
  } else {
    if (curr_pos_add != NULL) {
      int term_diff_add   = (int)(curr_pos_add - start_pos);
      memcpy(*term, start_pos, term_diff_add);
      (*term)[term_diff_add]  = '\0';
      *prev_pos               = curr_pos_add+1;
      return;
    }
    if (curr_pos_minus != NULL) {
      int term_diff_minus = (int)(curr_pos_minus - start_pos);
      memcpy(*term, start_pos, term_diff_minus);
      (*term)[term_diff_minus]  = '\0';
      *prev_pos                 = curr_pos_minus+1;
      return;
    }
    if (curr_pos_add == NULL && curr_pos_minus == NULL) {
      int prev_idx  = (int)(start_pos - line);
      int term_diff = strlen(line) + 1 - prev_idx;
      memcpy(*term, start_pos, term_diff);
      (*term)[term_diff]  = '\0';
      return;
    }
  }
}

nvars_t get_nvars(const char *fn)
{
  FILE *fh  = fopen(fn,"r");
  // load lines and store data
  const size_t max_line_size  = 1000;
  char *line  = (char *)malloc(max_line_size * sizeof(char));

  // get first line (variables)
  const char comma_splicer  = ',';

  // get number of variables
  nvars_t nvars = 1; // number of variables is number of commata + 1 in first line
  if (fgets(line, max_line_size, fh) != NULL) {
    char *tmp = strchr(line, comma_splicer);
    while (tmp != NULL) {
      // if there is a comma at the end of the line, i.e. strlen(line)-2 (since
      // we have "\0" at the end of the string line, then we do not get another
      // variable
      if ((int)(tmp-line) < strlen(line)-2) {
      nvars++;
      tmp = strchr(tmp+1, comma_splicer);
      } else {
        break;
      }
    }
  } else {
    printf("Bad file format.\n");
    nvars = 0;
  }
  free(line);
  fclose(fh);

  return nvars;
}

gb_t *load_input(const char *fn, const nvars_t nvars, const int ordering,
    mp_cf4_ht_t *ht, const int simplify, const int vb, const int nthrds)
{
  uint64_t fl;

  hash_t i, j;

#if !__GB_HAVE_SSE2
  hash_t k;
#endif
  struct timeval t_load_start;
  if (vb > 1) {
    gettimeofday(&t_load_start, NULL);
  }

  // open file in binary mode and get file size
  FILE	*fh  = fopen(fn,"rb");
  if (fh == NULL) {
    if (vb > 0)
      printf("File not found!\n");
    return NULL;
  } else {
    fseek(fh, 0L, SEEK_END);
    fl  = ftell(fh);
    fclose(fh);
  }

  // get number of lines in file:
  // number of lines - 2 is number of generators in input system
  int nlines  = 0;
  char buf[1000];
  fh  = fopen(fn,"r");
  while(fgets(buf, sizeof(buf), fh) != NULL)
    nlines++;
  fclose(fh);

  fh  = fopen(fn,"r");
  // load lines and store data
  const size_t max_line_size  = 1000;
  char *line  = (char *)malloc(max_line_size * sizeof(char));

  // we already know the number of variables
  // basis->nv  = nvars;

  char *tmp;
  // allocate memory for storing variable names
  char **vnames = (char **)malloc(nvars * sizeof(char *));
  if (fgets(line, max_line_size, fh) != NULL) {
    tmp = line;
  } else {
    printf("Bad file format.\n");
    return NULL;
  }
  for (i=0; i<nvars; ++i) {
    vnames[i]  = get_variable_name(line, &tmp);
  }

  // get second line (modulus)
  mod_t mod = 0;
  if (fgets(line, max_line_size, fh) != NULL) {
    int64_t tmp_mod = atol(line);
    if (tmp_mod > 0) {
      mod  = (mod_t)tmp_mod;
    } else {
      printf("Bad file format.\n");
      return NULL;
    }
  } else {
    printf("Bad file format.\n");
    return NULL;
  }

  // initialize basis with information from above
  gb_t *basis = initialize_basis(ordering, nlines, nvars, vnames, mod, simplify, fl);

  char *prev_pos;
  char *term  = (char *)malloc(200 * sizeof(char));
  int nterms;
#if __GB_HAVE_SSE2
  // for intermediate storage of exp vector, needs to have 128bit size in order
  // to hinder memory corruption when loading in sse vector
  exp_t *exp = (exp_t *)calloc(16, sizeof(exp_t));
#else
  deg_t deg;
#endif
  deg_t max_deg;

  // NOTE: For easier divisibility checks in symbolic preprocessing we put at
  // the first position of basis, i.e. index 0 a NULL element.
  // Thus, basis->load is always one bigger than the actual number of elements
  // in the basis.
  basis->cf[0]  = NULL;
  basis->eh[0]  = NULL;

  // get all remaining lines, i.e. generators
  int cf_tmp  = 0; // temp for coefficient value, possibly coeff is negative.
  int iv_tmp  = 0; // temp for inverse value, possibly coeff is negative.
  coeff_t iv  = 0; //inverse value of lead coeff in order to normalize input
  for (i=1; i<basis->load; ++i) {
    if (fgets(line, max_line_size, fh) != NULL) {
      // get number of terms first
      nterms        = get_number_of_terms(line);
      basis->nt[i]  = nterms;

#if IO_DEBUG
      printf("nterms %d\n",nterms);
#endif

      // allocate memory for all terms
      basis->cf[i]  = (coeff_t *)malloc(nterms * sizeof(coeff_t));
      basis->eh[i]  = (hash_t *)malloc(nterms * sizeof(hash_t));
      prev_pos  = line;
      max_deg   = 0;
      // next: go through line, term by term
      // we do first term differently since we normalize polynomial with lead
      // coefficient
      get_term(line, &prev_pos, &term);
      // get coefficient first
      if (term != NULL) {
        iv_tmp  = (int)strtol(term, NULL, 10);
        // if shortcut notation is used coeff 1 is not written down and strtol
        // boils down to 0. so we adjust this value to 1 again
        if (iv_tmp == 0) {
          if (term[0] == '-') {
            iv_tmp = -1;
          } else {
            iv_tmp = 1;
          }
        }
        while (iv_tmp < 0) {
          iv_tmp  +=  basis->mod;
        }
        iv  = iv_tmp;
        inverse_coefficient(&iv, basis->mod);
        basis->cf[i][0] = 1;
      }
#if __GB_HAVE_SSE2
      /*
      memset(exp, 0, 16 * sizeof(exp_t));
      for (k=0; k<basis->nv; ++k) {
        exp[k]  = ht->exp[ht->load][k];
      }
      ht->ev[ht->load]  = _mm_loadu_si128((exp_v *)exp);
      */
      store_exponent(term, basis, ht);
#else
      deg = 0;
      // now loop over variables of term
      for (k=0; k<basis->nv; ++k) {
        // if we use graded reverse lexicographical order (basis->ord=0) then we
        // store the exponent's entries in reverse order => we can use memcmp
        // when sorting the columns of the gbla matrix
        if (basis->ord == 0)
          deg += ht->exp[ht->load][basis->nv-1-k]  = get_exponent(term, basis->vnames[k]);
        else
          deg += ht->exp[ht->load][k]  = get_exponent(term, basis->vnames[k]);
      }
      // store degree already in hash table
      ht->deg[ht->load] = deg; 
#endif
      // hash exponent and store degree
      max_deg         = max_deg > ht->deg[ht->load] ? max_deg : ht->deg[ht->load];
      basis->eh[i][0] = check_in_hash_table(ht);
#if IO_DEBUG
      printf("cf[%lu] = %u | eh[%lu][%u] = %lu --> %lu\n",i,basis->cf[i][0],i,0,basis->eh[i][0], ht->val[basis->eh[i][0]]);
#endif
      for (j=1; j<nterms; ++j) {
        get_term(line, &prev_pos, &term);
        // get coefficient first
        if (term != NULL) {
          cf_tmp  = (int)strtol(term, NULL, 10);
          if (cf_tmp == 0) {
            if (term[0] == '-') {
              cf_tmp = -1;
            } else {
              cf_tmp = 1;
            }
          }
          while (cf_tmp < 0) {
            cf_tmp  +=  basis->mod;
          }
          basis->cf[i][j] = cf_tmp;
          basis->cf[i][j] = MODP(basis->cf[i][j]*iv,basis->mod);
        }
#if __GB_HAVE_SSE2
      /*
      memset(exp, 0, 16 * sizeof(exp_t));
      for (k=0; k<basis->nv; ++k) {
        exp[k]  = ht->exp[ht->load][k];
      }
      ht->ev[ht->load]  = _mm_loadu_si128((exp_v *)exp);
      */
      store_exponent(term, basis, ht);
#else
        deg = 0;
        // now loop over variables of term
        for (k=0; k<basis->nv; ++k) {
          // if we use graded reverse lexicographical order (basis->ord=0) then we
          // store the exponent's entries in reverse order => we can use memcmp
          // when sorting the columns of the gbla matrix
          //ht->exp[ht->load][k]  = get_exponent(term, basis->vnames[k]);
          if (basis->ord == 0)
            deg += ht->exp[ht->load][basis->nv-1-k]  = get_exponent(term, basis->vnames[k]);
          else
            deg += ht->exp[ht->load][k]  = get_exponent(term, basis->vnames[k]);
        }
        // store degree already in hash table
        ht->deg[ht->load] = deg; 
#endif
        // hash exponent and store degree
        max_deg         = max_deg > ht->deg[ht->load] ? max_deg : ht->deg[ht->load];
        basis->eh[i][j] = check_in_hash_table(ht);
#if IO_DEBUG
        printf("cf[%lu] = %u | eh[%lu][%lu] = %lu --> %lu\n",i,basis->cf[i][j],i,j,basis->eh[i][j], ht->val[basis->eh[i][j]]);
#endif
      }
      // if basis->hom is 0 then we have already found an inhomogeneous
      // polynomial and the system of polynomials is not homogeneous
      if (basis->hom == 1) {
        if (ht->deg[basis->eh[i][0]] == ht->deg[basis->eh[i][basis->nt[i]-1]])
          basis->hom  = 1;
        else
          basis->hom  = 0;
      }
      basis->deg[i] = max_deg;
    }
  }
#if __GB_HAVE_SSE2
  free(exp);
#endif
  free(term);
  free(line);
  fclose(fh);

  return basis;
}

void write_reduced_matrix_to_pbm(mat_t *mat, const char *fn)
{
  ri_t i;
	ri_t m        = mat->nru + mat->nrl;
	ci_t n        = mat->ncl + mat->ncr;
  ri_t min      = n > 512 ? n : 512;
  // min+2 since we need to end line with '\n\0'
	char *buffer  = malloc(min+2 * sizeof(char));

	FILE *fh  = fopen(fn, "wb");

	/*  magic PBM header */
#ifdef __LP64__ /*  64bit machine */
	sprintf(buffer, "P1\n# matrix size(%u, %u)\n%u %u\n", m, n, n, m);
#else /*  32bit machine */
	sprintf(buffer, "P1\n# matrix size(%u, %u)\n%u %u\n", m, n, n, m);
#endif

	fwrite(buffer, sizeof(char), strlen(buffer), fh);
  
  // write top block AB
  for (i=0; i<mat->nru; ++i) {
    write_upper_part_row_to_buffer(buffer, i, mat);
	  fwrite(buffer, sizeof(char), strlen(buffer), fh);
    fflush(fh);
  }
  // write bottom block CD
  for (i=0; i<mat->nrl; ++i) {
    write_lower_part_row_to_buffer(buffer, i, mat);
	  fwrite(buffer, sizeof(char), strlen(buffer), fh);
    fflush(fh);
  }
  fclose(fh);
  free(buffer);
}

void write_matrix_to_pbm(mat_t *mat, const char *fn)
{
  ri_t i;
	ri_t m        = mat->nru + mat->nrl;
	ci_t n        = mat->ncl + mat->ncr;
  ri_t min      = n > 512 ? n : 512;
  // min+2 since we need to end line with '\n\0'
	char *buffer  = malloc(min+2 * sizeof(char));
	FILE *fh  = fopen(fn, "wb");

	/*  magic PBM header */
#ifdef __LP64__ /*  64bit machine */
	sprintf(buffer, "P1\n# matrix size(%u, %u)\n%u %u\n", m, n, n, m);
#else /*  32bit machine */
	sprintf(buffer, "P1\n# matrix size(%u, %u)\n%u %u\n", m, n, n, m);
#endif

	fwrite(buffer, sizeof(char), strlen(buffer), fh);

  // write top block AB
  //for (i=0; i<mat->nru; ++i) {
  for (i=mat->nru; i>0; --i) {
    write_sparse_dense_block_row_to_buffer(buffer, i-1, mat->A, mat->B, mat->cbl,
        mat->cbr, mat->bs);
	  fwrite(buffer, sizeof(char), strlen(buffer), fh);
    fflush(fh);
  }
  // write bottom block CD
  //for (i=0; i<mat->nrl; ++i) {
  for (i=mat->nrl; i>0; --i) {
    write_sparse_dense_block_row_to_buffer(buffer, i-1, mat->C, mat->D, mat->cbl,
        mat->cbr, mat->bs);
	  fwrite(buffer, sizeof(char), strlen(buffer), fh);
    fflush(fh);
  }
  fclose(fh);
  free(buffer);
}

void write_sparse_dense_block_row_to_buffer(char *buffer, const nelts_t idx,
    const sb_fl_t *A, const dbm_fl_t *B, const nelts_t cbl, const nelts_t cbr,
    const bi_t bs)
{
  nelts_t i, j;

  nelts_t nc  = A->ncols + B->ncols;

  memset(buffer, '0', nc);
  buffer[nc]    = '\n';
  buffer[nc+1]  = '\0';
  
  ri_t rbi = idx / bs;
  ri_t rib = idx % bs;

  // row in A, lefthand side
  for (i=0; i<cbl; ++i) {
    if (A->blocks[rbi][i].val != NULL) {
      for (j=0; j<A->blocks[rbi][i].sz[rib]; ++j) {
        buffer[A->ncols-1 - (i*bs +  A->blocks[rbi][i].pos[rib][j])]  = '1';
      }
    }
  }
  // row in B, righthand side
  for (i=0; i<cbr; ++i) {
    if (B->blocks[rbi][i].val != NULL) {
      for (j=0; j<bs; ++j) {
        if (B->blocks[rbi][i].val[(rib*bs) + j] != 0) {
          buffer[A->ncols + i*bs + j]  = '1';
        }
      }
    }
  }
}

void write_upper_part_row_to_buffer(char *buffer, const nelts_t idx,
    const mat_t *mat)
{
  nelts_t i, j;

  nelts_t nc  = mat->ncl + mat->ncr;

  // need inverse index since rows of B has to be written in inverse order
  nelts_t iidx  = mat->nru-1-idx;

  memset(buffer, '0', nc);
  buffer[nc]    = '\n';
  buffer[nc+1]  = '\0';
  
  ri_t rbi = iidx / mat->bs;
  ri_t rib = iidx % mat->bs;

  // row in A, lefthand side
  // has only 1 at diagonal at position idx not iidx!
  buffer[idx] = '1';
  // row in B, righthand side
  for (i=0; i<mat->cbr; ++i) {
    if (mat->B->blocks[rbi][i].val != NULL) {
      for (j=0; j<mat->bs; ++j) {
        if (mat->B->blocks[rbi][i].val[rib*mat->bs + j] != 0) {
          buffer[mat->ncl + i*mat->bs + j]  = '1';
        }
      }
    }
  }
}

void write_lower_part_row_to_buffer(char *buffer, const nelts_t idx,
    const mat_t *mat)
{
  nelts_t i;

  nelts_t nc  = mat->ncl + mat->ncr;

  memset(buffer, '0', nc);
  buffer[nc]    = '\n';
  buffer[nc+1]  = '\0';
  
  // row in C, lefthand side is zero!

  // row in B, righthand side.
  // if idx > D->rank just keep buffer ={0}
  if (idx < mat->DR->rank) {
    if (mat->DR->row[idx]->piv_val != NULL) {
      for (i=0; i<mat->ncr; ++i) {
        if (mat->DR->row[idx]->piv_val[i] != 0) {
          buffer[mat->ncl + i] ='1';
        }
      }
    }
  }
}

void print_basis(const gb_t *basis)
{
  nelts_t i, j;
  nvars_t k;

#if __GB_HAVE_SSE2
  exp_t exp[ht->nev*ht->vl];
#else
  exp_t *exp = NULL;
#endif
  for (k=0; k<basis->nv-1; ++k) {
    printf("%s, ",basis->vnames[k]);
  }
  printf("%s\n",basis->vnames[k]);
  for (i=basis->st; i<basis->load; ++i) {
    if (basis->red[i] == NOT_REDUNDANT) {
      // we do the first term differently, since we do not have a "+" in front of
      // it
      printf("%u", basis->cf[i][0]);
#if __GB_HAVE_SSE2
      for (k=0; k<ht->nev; ++k)
        _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][0]][k]);
#else
      exp = ht->exp[basis->eh[i][0]];
#endif
      for (k=0; k<basis->nv; ++k) {
        for (k=0; k<basis->nv; ++k) {
          if (basis->ord == 0) {
            if (exp[basis->nv-1-k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
            }
          } else {
            if (exp[k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[k]);
            }
          }
        }
      }
      for (j=1; j<basis->nt[i]; ++j) {
        printf("+%u", basis->cf[i][j]);
#if __GB_HAVE_SSE2
        for (k=0; k<ht->nev; ++k)
          _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][j]][k]);
#else
        exp = ht->exp[basis->eh[i][j]];
#endif
        for (k=0; k<basis->nv; ++k) {
          if (basis->ord == 0) {
            if (exp[basis->nv-1-k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
            }
          } else {
            if (exp[k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[k]);
            }
          }
        }
      }
      printf("\n");
    }
  }
}

void print_basis_in_singular_format(const gb_t *basis)
{
  nelts_t i, j;
  nvars_t k;

#if __GB_HAVE_SSE2
  exp_t exp[ht->nev*ht->vl];
#else
  exp_t *exp = NULL;
#endif
  // prints ring
  printf("ring r = %u, (%s", basis->mod, basis->vnames[0]);
  for (k=1; k<basis->nv; ++k)
    printf(",%s", basis->vnames[k]);
  printf("), dp;\r\n");

  // prints input ideal
  printf("ideal i;\r\n");
  for (i=1; i<basis->st; ++i) {
    printf("i[%u]=", i);
    // we do the first term differently, since we do not have a "+" in front of
    // it
    printf("%u", basis->cf[i][0]);
#if __GB_HAVE_SSE2
    for (k=0; k<ht->nev; ++k)
      _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][0]][k]);
#else
    exp = ht->exp[basis->eh[i][0]];
#endif
    for (k=0; k<basis->nv; ++k) {
      if (basis->ord == 0) {
        if (exp[basis->nv-1-k] != 0) {
          printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
        }
      } else {
        if (exp[k] != 0) {
          printf("*%s^%u", basis->vnames[k], exp[k]);
        }
      }
    }
    for (j=1; j<basis->nt[i]; ++j) {
      printf("+%u", basis->cf[i][j]);
#if __GB_HAVE_SSE2
      for (k=0; k<ht->nev; ++k)
        _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][j]][k]);
#else
      exp = ht->exp[basis->eh[i][j]];
#endif
      for (k=0; k<basis->nv; ++k) {
        if (basis->ord == 0) {
          if (exp[basis->nv-1-k] != 0) {
            printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
          }
        } else {
          if (exp[k] != 0) {
            printf("*%s^%u", basis->vnames[k], exp[k]);
          }
        }
      }
    }
    printf(";\r\n");
  }

  // prints groebner basis
  nelts_t ctr = 0;
  printf("ideal g;\r\n");
  for (i=basis->st; i<basis->load; ++i) {
    if (basis->red[i] == NOT_REDUNDANT) {
      ctr++;
      printf("g[%u]=", ctr);
      // we do the first term differently, since we do not have a "+" in front of
      // it
      printf("%u", basis->cf[i][0]);
#if __GB_HAVE_SSE2
      for (k=0; k<ht->nev; ++k)
        _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][0]][k]);
#else
      exp = ht->exp[basis->eh[i][0]];
#endif
      for (k=0; k<basis->nv; ++k) {
        if (basis->ord == 0) {
          if (exp[basis->nv-1-k] != 0) {
            printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
          }
        } else {
          if (exp[k] != 0) {
            printf("*%s^%u", basis->vnames[k], exp[k]);
          }
        }
      }
      for (j=1; j<basis->nt[i]; ++j) {
        printf("+%u", basis->cf[i][j]);
#if __GB_HAVE_SSE2
        for (k=0; k<ht->nev; ++k)
          _mm_storeu_si128((exp_v *)exp + k*ht->vl, ht->ev[basis->eh[i][j]][k]);
#else
        exp = ht->exp[basis->eh[i][j]];
#endif
        for (k=0; k<basis->nv; ++k) {
          if (basis->ord == 0) {
            if (exp[basis->nv-1-k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[basis->nv-1-k]);
            }
          } else {
            if (exp[k] != 0) {
              printf("*%s^%u", basis->vnames[k], exp[k]);
            }
          }
        }
      }
      printf(";\r\n");
    }
  }
}
void inverse_coefficient(coeff_t *x, const coeff_t modulus)
{
  assert(*x);
  if ( *x == 1 ) return ;
  assert((int32_t)modulus > 0);
  int32_t u1 = 1, u2 = 0;
  int32_t v1 = 0, v3 = (int32_t)modulus;
  int32_t u3 = (int32_t)*x, v2 = 1;
  while (v3 != 0) {
    int32_t q  = u3 / v3;
    int32_t t1 = u1 - v1 * q;
    u1  = v1; v1  = t1;

    int32_t t3 = u3 - v3 * q;
    u3  = v3; v3  = t3;

    int32_t t2 = u2 - v2 * q;
    u2  = v2; v2  = t2;
  }
  if (u1 < 0) {
    u1  +=  modulus;
    /* check_inverse(*x,u1,modulus); */
    *x  =   (re_t)u1;
    return;
  }
  if (u1 > (int32_t)modulus) {
    u1  -=  modulus;
    /* check_inverse(*x,u1,modulus); */
    *x  =   (re_t) u1;
    return;
  }
  /* check_inverse(*x,u1,modulus); */
  *x  = (re_t)u1;
  return;
}
