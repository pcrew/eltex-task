
#ifndef MACRO_MAGIC_H
#define MACRO_MAGIC_H

#define array_size(a)	(sizeof(a)/ sizeof(a[0]))\

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define   ALIGN(x, a)	\
	__ALIGN_KERNEL((x), (a))

#define __ALIGN_KERNEL(x, a)	\
	__ALIGN_KERNEL_MASK((x), (typeof(x))(a) - 1)

#define __ALIGN_KERNEL_MASK(x, mask)	\
	  (((x) + (mask)) & ~(mask))	

#endif
