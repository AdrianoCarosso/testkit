#include "config.h"

//#include "arm7_9_common.h"
//#include "arm.h"
//#include "arm_jtag.h"

#include "embeddedice.h"
#include "register.h"

//#define container_ofx(ptr, type, member) ({                     \
//        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
//        (type *)( (char *)__mptr - offsetof(type,member) );     \
//        })


int main(int argc, char* argv[])
{
    /*volatile*/ struct arm7_9_common * x ;
    /*volatile*/ struct arm7_9_common * y ;

    struct target *target = NULL ;

    x = container_of(target->arch_info, struct arm7_9_common, armv4_5_common) ;
    y = target_to_arm7_9(target) ;
    printf("x=%p\n", x);
    printf("y=%p\n", y);
    return(0) ;
}
