
#include	"plugins.h"
#include    "gc_malloc.h"

block	optimize_map_unary(target t, block ob);
block	optimize_map_reduce(target t, block ob);
block	optimize_common_factor(target t, block ob);
block	optimize_redef_dim(target t, block ob) ;

block optimize_block(target t, block ob){
    //ob = optimize_common_factor(target t, block ob);
    ob = optimize_redef_dim(t, ob);
    ob = optimize_map_unary(t, ob);
    ob = optimize_map_reduce(t, ob);
    return ob;
}