// RUN: %clang -### --target=amdgcn--amdhsa -x assembler -mcpu=kaveri %s 2>&1 | FileCheck -check-prefix=AS_LINK %s
// RUN: %clang -### -g --target=amdgcn--amdhsa -mcpu=kaveri -nogpulib %s 2>&1 | FileCheck -check-prefix=DWARF_VER %s
// RUN: %clang -### --target=amdgcn-amd-amdpal -x assembler -mcpu=kaveri %s 2>&1 | FileCheck -check-prefix=AS_LINK %s
// RUN: %clang -### -g --target=amdgcn-amd-amdpal -mcpu=kaveri %s 2>&1 | FileCheck -check-prefix=DWARF_VER %s
// RUN: %clang -### --target=amdgcn-mesa-mesa3d -x assembler -mcpu=kaveri %s 2>&1 | FileCheck -check-prefix=AS_LINK %s
// RUN: %clang -### -g --target=amdgcn-mesa-mesa3d -mcpu=kaveri %s 2>&1 | FileCheck -check-prefix=DWARF_VER %s

// AS_LINK: "-cc1as"
// AS_LINK: ld.lld{{.*}} "--no-undefined" "-shared"

// DWARF_VER: "-dwarf-version=5"

// RUN: %clang -### --target=amdgcn--amdhsa -x assembler \
// RUN:  -Wl,--unresolved-symbols=ignore-all %s 2>&1 | FileCheck -check-prefix=AS_LINK_UR %s
// RUN: %clang -### --target=amdgcn--amdhsa -x assembler \
// RUN:  -Xlinker --unresolved-symbols=ignore-all %s 2>&1 | FileCheck -check-prefix=AS_LINK_UR %s

// AS_LINK_UR: "-cc1as"
// AS_LINK_UR: ld.lld{{.*}} "--no-undefined"{{.*}} "--unresolved-symbols=ignore-all"

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=gfx90a:xnack+:sramecc- -nogpulib \
// RUN:   -L. -flto -fconvergent-functions %s 2>&1 | FileCheck -check-prefix=LTO %s
// LTO: clang{{.*}}"-flto=full"{{.*}}"-fconvergent-functions"
// LTO: ld.lld{{.*}}"-plugin-opt=mcpu=gfx90a"{{.*}}"-plugin-opt=-mattr=-sramecc,+xnack"{{.*}}

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=gfx90a:xnack+:sramecc- -nogpulib \
// RUN:   -L. -fconvergent-functions %s 2>&1 | FileCheck -check-prefix=MCPU %s
// MCPU: ld.lld{{.*}}"-plugin-opt=mcpu=gfx90a"{{.*}}"-plugin-opt=-mattr=-sramecc,+xnack"{{.*}}

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=gfx906 -nogpulib \
// RUN:   -fuse-ld=ld %s 2>&1 | FileCheck -check-prefixes=LD %s
// LD: ld.lld

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=gfx906 -nogpulib \
// RUN:   -r %s 2>&1 | FileCheck -check-prefixes=RELO %s
// RELO-NOT: -shared

// RUN: %clang -target amdgcn-amd-amdhsa -march=gfx90a -stdlib -startfiles \
// RUN:   -nogpulib -nogpuinc -### %s 2>&1 | FileCheck -check-prefix=STARTUP %s
// STARTUP: ld.lld{{.*}}"-lc" "-lm" "{{.*}}crt1.o"

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=gfx906 %s 2>&1 | FileCheck -check-prefix=ROCM %s
// ROCM-NOT: -mlink-builtin-bitcode

// RUN: %clang -### --target=amdgcn-amd-amdhsa -mcpu=fiji -x ir %s \
// RUN:  --rocm-device-lib-path=%S/Inputs/rocm/amdgcn/bitcode 2>&1 \
// RUN: | FileCheck -check-prefix=DEVICE-LIBS %s
// DEVICE-LIBS: "-mlink-builtin-bitcode" "[[ROCM_PATH:.+]]ockl.bc"
