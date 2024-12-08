//
//  SimpleRtlBT.c
//  SimpleRtlBT
//
//  Created by laobamac on 2024/12/8.
//

#include <mach/mach_types.h>

kern_return_t SimpleRtlBT_start(kmod_info_t * ki, void *d);
kern_return_t SimpleRtlBT_stop(kmod_info_t *ki, void *d);

kern_return_t SimpleRtlBT_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t SimpleRtlBT_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
