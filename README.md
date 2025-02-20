## T-Tex Kernel Implementation
* We utilize linux ioctl calls to communicate between OpenMP runtime and the kernel module
* Kprobe handlers are used to probe into linux context-switch function

### Compile

``git clone https://github.com/mittalswastik/ttex_kernel`` \
``sudo apt install linux-headers-$(uname -r)``

run ``make clean`` \
``make``

### Install
run  ``sudo insmod ttex_kernel_update.ko``

### Testing T-Tex without Kernel Timer Crediting
* Comment the code in [pre](https://github.com/mittalswastik/ttex_kernel/blob/37347adfc352b278f10d4a3f534d865d9d573d9b/ttex_kernel_update.c#L364) and [post](https://github.com/mittalswastik/ttex_kernel/blob/37347adfc352b278f10d4a3f534d865d9d573d9b/ttex_kernel_update.c#L402) handler
  * Can also remove the context-switch kprobe [handler](https://github.com/mittalswastik/ttex_kernel/blob/37347adfc352b278f10d4a3f534d865d9d573d9b/ttex_kernel_update.c#L472)
* Execute [multiphase](https://github.com/mittalswastik/ttex_benchmark) T-Tex security again to generate new timing information

### Point To Note

Current implementation works with kernel header file stdatomic.h in Linux-5.40-150, which is not available with upgraded linux versions. Upgraded versions use linux/atomic.h but just updating this would not work as it has different atomic types than the ones used.
