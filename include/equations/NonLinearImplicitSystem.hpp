/*=========================================================================

 Program: FEMUS
 Module: NonLinearImplicitSystem
 Authors: Simone Bnà
 
 Copyright (c) FEMTTU
 All rights reserved. 

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __nonlinear_implicit_system_h_
#define __nonlinear_implicit_system_h_

//----------------------------------------------------------------------------
// includes :
//----------------------------------------------------------------------------
#include "LinearImplicitSystem.hpp"

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------


class NonLinearImplicitSystem : public LinearImplicitSystem {

public:

/** Constructor.  Optionally initializes required data structures. */
  NonLinearImplicitSystem (MultiLevelProblem& ml_probl, const std::string& name, const unsigned int number);
  
  virtual ~NonLinearImplicitSystem();
  
  /** The type of the parent. */
  typedef LinearImplicitSystem Parent;
  
  /** Solves the system. */
  virtual void solve ();
  
  /** Clear all the data structures associated with the system. */
  virtual void clear();

  /** Init the system PDE structures */
  virtual void init();
   
  /** Returns  the number of iterations taken for the most recent nonlinear solve. */
  unsigned int n_nonlinear_iterations() const { return _n_nonlinear_iterations; }

  /** Returns the final residual for the nonlinear system solve. */
  double final_nonlinear_residual() const { return _final_nonlinear_residual; }
  
  /** Set the max number of non-linear iterations for the nonlinear system solve. */
  void SetMaxNumberOfNonLinearIterations(unsigned int max_nonlin_it) {_n_max_nonlinear_iterations = max_nonlin_it;};
  
  /** Checks for the non the linear convergence */
  bool CheckConvergence(const char pdename[], const unsigned gridn);
    
protected:
  
   /** The number of nonlinear iterations required to solve the nonlinear system R(x)=0.  */
  unsigned int _n_nonlinear_iterations;

  /** The final residual for the nonlinear system R(x) */
  double _final_nonlinear_residual;
  
  /** The max number of non-linear iterations */
  unsigned int _n_max_nonlinear_iterations;

  /** The max non linear tolerance **/
  double _max_nonlinear_convergence_tolerance;
  
  
private:

  void CreateSystemPDEStructure();
  
};


#endif