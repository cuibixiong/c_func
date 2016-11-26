#define STATIC_ZERO_ASSERT(condition)(sizeof(struct { int:-!(condition); }))
#define STATIC_NULL_ASSERT(condition)((void *)STATIC_ZERO_ASSERT(condition)) 
