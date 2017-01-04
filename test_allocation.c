#include "my_alloc.c"
#include <stdio.h>

int main ()
{
  int ** array = alloc_2d_int(10, 10);
  free(array[0]);
  free(array);
  return 0;
}
