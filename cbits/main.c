/* Almost a verbatim copy of the reference implementation. */

#include "siphash.h"
#include <stddef.h>
#include <stdio.h>


int main () {
  printf("\n hello world \n");
  uint64_t v[4] = {0,0,0,0};
  hashable_siphash_init(4,5,v);
  for(int i = 0; i < 4; i++) {
      printf("%i, %ld \n", i, v[i]);
  }
  hashable_siphash_compression(1,v,"a",0,1);
  printf("\n after compression \n ");
  for(int i = 0; i < 4; i++) {
      printf("%i, %ld \n", i, v[i]);
  }
};
