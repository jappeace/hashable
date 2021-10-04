/* Almost a verbatim copy of the reference implementation. */

#include "siphash.h"
#include <stddef.h>
#include <stdio.h>


int main () {
  printf("\n hello world \n");
  uint64_t v[4] = {0,0,0,0};
  hashable_siphash_init(4,5,v);
  hashable_siphash_compression(1,v,"AB",0,2);
  uint64_t res2 = hashable_siphash_finalize(4, v);

  hashable_siphash_init(4,5,v);
  hashable_siphash_compression(1,v,"A",0,1);
  hashable_siphash_compression(1,v,"B",0,1);
  uint64_t res1 = hashable_siphash_finalize(4, v);

  printf("%d == %d D", res2, res1);
};
