// RUN: llvm-mc -triple=amdgcn -mcpu=gfx1250 -show-encoding %s | FileCheck -check-prefix=GFX12 %s

//===----------------------------------------------------------------------===//
// A VOPD instruction can use one or more literals,
// provided that they are identical.
//===----------------------------------------------------------------------===//

// LITERAL

v_dual_mul_f32      v11, v1, v2                  ::  v_dual_mul_f32      v10, 0x24681357, v5
// GFX12: encoding: [0x01,0x05,0xc6,0xc8,0xff,0x0a,0x0a,0x0b,0x57,0x13,0x68,0x24]

// LITERAL*2

v_dual_mul_f32      v11, 0x24681357, v2          ::  v_dual_mul_f32      v10, 0x24681357, v5
// GFX12: encoding: [0xff,0x04,0xc6,0xc8,0xff,0x0a,0x0a,0x0b,0x57,0x13,0x68,0x24]

// LITERAL + KIMM

v_dual_add_f32      v5, 0xaf123456, v2           ::  v_dual_fmaak_f32     v6, v3, v1, 0xaf123456 ;
// GFX12: encoding: [0xff,0x04,0x02,0xc9,0x03,0x03,0x06,0x05,0x56,0x34,0x12,0xaf]

// KIMM + LITERAL

v_dual_fmamk_f32    v122, v74, 0xa0172923, v161  ::  v_dual_lshlrev_b32   v247, 0xa0172923, v99
// GFX12: encoding: [0x4a,0x43,0xa3,0xc8,0xff,0xc6,0xf6,0x7a,0x23,0x29,0x17,0xa0]

// KIMM*2

v_dual_fmamk_f32    v122, 0xdeadbeef, 0xdeadbeef, v161 ::  v_dual_fmamk_f32  v123, 0xdeadbeef, 0xdeadbeef, v162
// GFX12: encoding: [0xff,0x42,0x85,0xc8,0xff,0x44,0x7b,0x7a,0xef,0xbe,0xad,0xde]

//===----------------------------------------------------------------------===//
// A VOPD instruction can use 2 scalar operands,
// but implicit VCC must be counted in.
//===----------------------------------------------------------------------===//

// 2 different SGPRs

v_dual_mul_f32      v0, s1, v2                   ::  v_dual_mul_f32       v3, s4, v5
// GFX12: encoding: [0x01,0x04,0xc6,0xc8,0x04,0x0a,0x02,0x00]

// SGPR + LITERAL

v_dual_fmaak_f32    v122, s74, v161, 2.741       ::  v_dual_max_i32       v247, v160, v98
// GFX12: encoding: [0x4a,0x42,0x6f,0xc8,0xa0,0xc5,0xf6,0x7a,0x8b,0x6c,0x2f,0x40]

v_dual_mov_b32      v247, v160                   ::  v_dual_fmaak_f32     v122, s74, v161, 2.741
// GFX12: encoding: [0xa0,0x01,0x02,0xca,0x4a,0x42,0x7b,0xf7,0x8b,0x6c,0x2f,0x40]

// SGPR*2 + LITERAL

v_dual_fmaak_f32    v122, s74, v161, 2.741       ::  v_dual_max_i32       v247, s74, v98
// GFX12: encoding: [0x4a,0x42,0x6f,0xc8,0x4a,0xc4,0xf6,0x7a,0x8b,0x6c,0x2f,0x40]

// SGPR + LITERAL*2

v_dual_fmaak_f32    v122, s74, v161, 2.741       ::  v_dual_fmamk_f32     v3, v6, 2.741, v1
// GFX12: encoding: [0x4a,0x42,0x45,0xc8,0x06,0x03,0x02,0x7a,0x8b,0x6c,0x2f,0x40]

// SGPR*2 + LITERAL*2

v_dual_fmaak_f32    v122, s74, v161, 2.741       ::  v_dual_fmamk_f32     v3, s74, 2.741, v1
// GFX12: encoding: [0x4a,0x42,0x45,0xc8,0x4a,0x02,0x02,0x7a,0x8b,0x6c,0x2f,0x40]

// LITERAL + VCC

v_dual_fmaak_f32    v122, v0, v161, 2.741       ::  v_dual_cndmask_b32   v1, v2, v3
// GFX12: encoding: [0x00,0x43,0x53,0xc8,0x02,0x07,0x00,0x7a,0x8b,0x6c,0x2f,0x40]

// LITERAL*2 + VCC

v_dual_fmaak_f32    v122, v0, v161, 2.741       ::  v_dual_cndmask_b32   v1, 2.741, v3
// GFX12: encoding: [0x00,0x43,0x53,0xc8,0xff,0x06,0x00,0x7a,0x8b,0x6c,0x2f,0x40]

// LITERAL*2 + VCC*2

v_dual_cndmask_b32  v255, 0xbabe, v2             ::  v_dual_cndmask_b32   v6, 0xbabe, v3
// GFX12: encoding: [0xff,0x04,0x52,0xca,0xff,0x06,0x06,0xff,0xbe,0xba,0x00,0x00]

// SGPR*2 + VCC

v_dual_add_f32      v255, s105, v2               ::  v_dual_cndmask_b32   v6, s105, v3
// GFX12: encoding: [0x69,0x04,0x12,0xc9,0x69,0x06,0x06,0xff]

// SGPR*2 + VCC*2

v_dual_cndmask_b32  v255, s1, v2                 ::  v_dual_cndmask_b32   v6, s1, v3
// GFX12: encoding: [0x01,0x04,0x52,0xca,0x01,0x06,0x06,0xff]

// VCC*2

v_dual_add_f32      v255, vcc_lo, v2             ::  v_dual_cndmask_b32   v6, v1, v3
// GFX12: encoding: [0x6a,0x04,0x12,0xc9,0x01,0x07,0x06,0xff]

//===----------------------------------------------------------------------===//
// A VOPD OpY mov_b32 instruction uses SRC2 source-cache if OpX is also mov_b32
//===----------------------------------------------------------------------===//

v_dual_mov_b32      v2, v5                       ::  v_dual_mov_b32       v3, v1
// GFX12: encoding: [0x05,0x01,0x10,0xca,0x01,0x01,0x02,0x02]

//===----------------------------------------------------------------------===//
// SRCX0 and SRCY0 may use the same bank if they are using the same VGPR; same for
// VSRCX1 and VSRCY1.
//===----------------------------------------------------------------------===//

v_dual_add_f32 v2, v2, v5 :: v_dual_mul_f32 v3, v2, v5
// GFX12: encoding: [0x02,0x0b,0x06,0xc9,0x02,0x0b,0x02,0x02]
