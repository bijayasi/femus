#ifndef __precondtype__
#define __precondtype__

enum PreconditionerType {
  IDENTITY_PRECOND =0,
  JACOBI_PRECOND,
  BLOCK_JACOBI_PRECOND,
  SOR_PRECOND,
  SSOR_PRECOND,
  EISENSTAT_PRECOND,
  ASM_PRECOND,
  CHOLESKY_PRECOND,
  ICC_PRECOND,
  ILU_PRECOND,
  LU_PRECOND,
  USER_PRECOND,
  SHELL_PRECOND,
  AMG_PRECOND,
  INVALID_PRECONDITIONER,
  MG_PRECOND,
  SLU_PRECOND,
  MLU_PRECOND,
  MCC_PRECOND
};



#endif