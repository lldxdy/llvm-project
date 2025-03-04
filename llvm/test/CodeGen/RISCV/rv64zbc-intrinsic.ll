; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=riscv64 -mattr=+zbc -verify-machineinstrs < %s \
; RUN:   | FileCheck %s -check-prefix=RV64ZBC

declare i64 @llvm.riscv.clmulr.i64(i64 %a, i64 %b)

define i64 @clmul64r(i64 %a, i64 %b) nounwind {
; RV64ZBC-LABEL: clmul64r:
; RV64ZBC:       # %bb.0:
; RV64ZBC-NEXT:    clmulr a0, a0, a1
; RV64ZBC-NEXT:    ret
  %tmp = call i64 @llvm.riscv.clmulr.i64(i64 %a, i64 %b)
  ret i64 %tmp
}

declare i32 @llvm.riscv.clmulr.i32(i32 %a, i32 %b)

define signext i32 @clmul32r(i32 signext %a, i32 signext %b) nounwind {
; RV64ZBC-LABEL: clmul32r:
; RV64ZBC:       # %bb.0:
; RV64ZBC-NEXT:    slli a1, a1, 32
; RV64ZBC-NEXT:    srli a1, a1, 32
; RV64ZBC-NEXT:    slli a0, a0, 32
; RV64ZBC-NEXT:    srli a0, a0, 32
; RV64ZBC-NEXT:    clmul a0, a0, a1
; RV64ZBC-NEXT:    srli a0, a0, 31
; RV64ZBC-NEXT:    sext.w a0, a0
; RV64ZBC-NEXT:    ret
  %tmp = call i32 @llvm.riscv.clmulr.i32(i32 %a, i32 %b)
  ret i32 %tmp
}
