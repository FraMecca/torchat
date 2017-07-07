/* raii_mem.h
 * This library consists in a set of macro
 * to ease the usage of the GCC cleanup attribute
 * https://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Variable-Attributes.html#index-g_t_0040code_007bcleanup_007d-attribute-1768
 *
 * It provides facilities for a generic free and for sockets.
 * The macro can be provided with a custom function that has this prototype for a generic type T*
 * static inline void myfun (T** pt);
 * An example follows
 *
struct manychar {
	char *a;
	char *b;
};

static inline void
myfree (struct manychar **ptr)
{
	struct manychar *p = *ptr;
	free (p->a);
	free (p->b);
	free (p);
}
 *
 */

#include <stdlib.h>
#include <unistd.h>

static inline void
autoptr_cleanup_generic_gfree (void *p)
{
  void **pp = (void**)p;
  if (p != NULL) free (*pp); 
  // TODO: use FREE, MALLOC
}

static inline void
socket_close (int *fd)
{
	close (*fd);
}

#define autofree   		__attribute__((cleanup(autoptr_cleanup_generic_gfree)))
#define autoclosesocket      __attribute__((cleanup(socket_close)))
#define customfree(f)   __attribute__((cleanup(f)))
