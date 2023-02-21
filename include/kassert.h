#ifndef __KASSERT_H__
#define __KASSERT_H__

#include <defs.h>
#include <log.h>

#define assert(_Expression) \
	(void)((!!(_Expression)) or \
	       (panic("Assertion failed: %s", #_Expression)))


#endif /* !__KASSERT_H__ */