#define STATIC_ZERO_ASSERT(condition)(sizeof(struct { int:-!(condition); }))
#define STATIC_NULL_ASSERT(condition)((void *)STATIC_ZERO_ASSERT(condition)) 

/////////////////////////////////////////
#define JOIN( X , Y ) JOIN_AGIN( X, Y )
#define JOIN_AGIN(X,Y) X##Y
 
#define STATIC_ASSERT(e) \
          typedef char JOIN(assert_failed_at_line,__LINE___) [(e)?1:-1]

/////////////////////////////////////////
