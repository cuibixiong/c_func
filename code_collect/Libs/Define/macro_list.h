#define FLAG_LIST(_)       \
   _(InWorklist)                     \
   _(EmittedAtUses)           \
   _(LoopInvariant)             \
   _(Commutative)             \
   _(Movable)                      \
   _(Lowered)                     \
   _(Guard)
 
#define DEFINE_FLAG(flag) flag,
  enum Flag {
      None = 0,
      FLAG_LIST(DEFINE_FLAG)
      Total
   };
#undef DEFINE_FLAG
 
#define FLAG_ACCESSOR(flag) \
int is##flag() {\
   return hasFlags(1 << flag);\
}\
void set##flag() {\
   setFlags(1 << flag);\
}\
void setNot##flag() {\
   removeFlags(1 << flag);\
}
 
FLAG_LIST(FLAG_ACCESSOR)
#undef FLAG_ACCESSOR
