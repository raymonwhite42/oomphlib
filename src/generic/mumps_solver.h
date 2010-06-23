//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC//           Version 0.90. August 3, 2009.
//LIC// 
//LIC// Copyright (C) 2006-2009 Matthias Heil and Andrew Hazel
//LIC// 
//LIC// This library is free software; you can redistribute it and/or
//LIC// modify it under the terms of the GNU Lesser General Public
//LIC// License as published by the Free Software Foundation; either
//LIC// version 2.1 of the License, or (at your option) any later version.
//LIC// 
//LIC// This library is distributed in the hope that it will be useful,
//LIC// but WITHOUT ANY WARRANTY; without even the implied warranty of
//LIC// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//LIC// Lesser General Public License for more details.
//LIC// 
//LIC// You should have received a copy of the GNU Lesser General Public
//LIC// License along with this library; if not, write to the Free Software
//LIC// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
//LIC// 02110-1301  USA.
//LIC// 
//LIC// The authors may be contacted at oomph-lib@maths.man.ac.uk.
//LIC// 
//LIC//====================================================================
//This is the header file for the C-wrapper functions for the HSL MA42 
//frontal solver

//Include guards to prevent multiple inclusions of the header
#ifndef MUMPS_SOLVER_HEADER
#define MUMPS_SOLVER_HEADER


// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
  #include <oomph-lib-config.h>
#endif

#include<stack>

#include "linear_solver.h"
#include "preconditioner.h"

namespace oomph
{




//====================================================================
/// \short Namespace for pool of fortran mumps solvers
//====================================================================
namespace MumpsSolverPool 
{

 ///Stack containing the IDs of available mumps solvers
 extern std::stack<int> Available_solver_ids;
 
 /// Bool indicating that the pool has been set up
 extern bool Pool_has_been_setup;
 
 /// Default max. number of mumps solvers
 extern int Max_n_solvers;
 
 /// Get new solver from pool
 extern void get_new_solver_id(unsigned& solver_id);
 
 /// Return solver to pool
 extern void return_solver(const unsigned& solver_id);

 /// \short Setup namespace -- specify the max. number of solver
 /// instantiations required.
 extern void setup(const unsigned& max_n_solvers); 

}




//====================================================================
/// \short  Wrapper to Mumps solver
//====================================================================
class MumpsSolver : public LinearSolver
{
 
  public:

 /// \short Constructor: Call setup
 MumpsSolver();
 
 /// Broken copy constructor
 MumpsSolver(const MumpsSolver& dummy) 
  { 
   BrokenCopy::broken_copy("MumpsSolver");
  } 
 
 /// Broken assignment operator
 void operator=(const MumpsSolver&) 
  {
   BrokenCopy::broken_assign("MumpsSolver");
  }

 ///Destructor: Cleanup
 ~MumpsSolver();
 
 /// Overload disable resolve so that it cleans up memory too
 void disable_resolve()
  {
   LinearSolver::disable_resolve();
   clean_up_memory();
  }
 
 /// \short Solver: Takes pointer to problem and returns the results Vector
 /// which contains the solution of the linear system defined by
 /// the problem's fully assembled Jacobian and residual Vector.
 void solve(Problem* const &problem_pt, DoubleVector &result);


 /// \short Linear-algebra-type solver: Takes pointer to a matrix and rhs 
 /// vector and returns the solution of the linear system. Problem pointer 
 /// defaults to NULL and can be omitted. The function returns the global 
 /// result Vector.
 /// Note: if Delete_matrix_data is true the function 
 /// matrix_pt->clean_up_memory() will be used to wipe the matrix data.
 void solve(DoubleMatrixBase* const &matrix_pt,
            const DoubleVector &rhs,
            DoubleVector &result);
 
 /// \short Resolve the system defined by the last assembled Jacobian
 /// and the specified rhs vector if resolve has been enabled.
 /// Note: returns the global result Vector.
 void resolve(const DoubleVector &rhs, DoubleVector &result);

 /// Return the Doc_stats flag
 bool& doc_stats()
  {
   return Doc_stats;
  }

 /// \short Returns the time taken to assemble the Jacobian matrix and 
 /// residual vector
 double jacobian_setup_time()
  {
   return Jacobian_setup_time;
  }

 /// \short Return the time taken to solve the linear system (needs to be 
 /// overloaded for each linear solver)
 virtual double linear_solver_solution_time()
  {
   return Solution_time;
  }


 /// \short Return the flag that decides if we're actually solving the
 /// system or just assembling the Jacobian and the rhs.
 /// (Used only for timing runs, obviously)
 bool &suppress_solve() 
  {
   return Suppress_solve;
  }
 
 /// \short Returns Delete_matrix_data flag. MumpsSolver needs its own copy 
 /// of the input matrix, therefore a copy must be made if any matrix 
 /// used with this solver is to be preserved. If the input matrix can be 
 /// deleted the flag can be set to true to reduce the amount of memory 
 /// required, and the matrix data will be wiped using its clean_up_memory()
 /// function.  Default value is false.
 bool &delete_matrix_data()
  {
   return Delete_matrix_data;
  }
  
 /// \short Do the factorisation stage
 /// Note: if Delete_matrix_data is true the function 
 /// matrix_pt->clean_up_memory() will be used to wipe the matrix data.
 void factorise(DoubleMatrixBase* const &matrix_pt);
  
 /// \short Do the backsubstitution for mumps solver
 /// Note: returns the global result Vector.
 void backsub(const DoubleVector &rhs,
              DoubleVector &result);
 
 /// Clean up the memory allocated by the mumps solver
 void clean_up_memory(); 

 /// Set scaling factor for workspace (defaults is 2)
 void set_workspace_scaling_factor(const unsigned& s);

 /// \short Default factor for workspace -- static so it can be overwritten
 /// globally.
 static int Default_workspace_scaling_factor;

  private:

 /// Jacobian setup time
 double Jacobian_setup_time;

 /// Solution time
 double Solution_time;

 /// Suppress solve?
 bool Suppress_solve;

 /// Set to true for MumpsSolver to output statistics (false by default).
 bool Doc_stats; 
  
 /// \short Delete_matrix_data flag. MumpsSolver needs its own copy 
 /// of the input matrix, therefore a copy must be made if any matrix 
 /// used with this solver is to be preserved. If the input matrix can be 
 /// deleted the flag can be set to true to reduce the amount of memory 
 /// required, and the matrix data will be wiped using its clean_up_memory()
 /// function. Default value is false.
 bool Delete_matrix_data;

 /// Solver ID in pool
 unsigned Solver_ID_in_pool;

};
 


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////



//====================================================================
/// An interface to allow Mumps to be used as an (exact) Preconditioner
//====================================================================
class MumpsPreconditioner : public Preconditioner
{
 public:

 /// Constructor.
 MumpsPreconditioner()
  {}
 
 /// Destructor.
 ~MumpsPreconditioner()
  {}
 
  /// Broken copy constructor.
  MumpsPreconditioner(const MumpsPreconditioner&)
  {
   BrokenCopy::broken_copy("MumpsPreconditioner");
  }


  /// Broken assignment operator.
  void operator=(const MumpsPreconditioner&)
  {
   BrokenCopy::broken_assign("MumpsPreconditioner");
  }
  
  /// \short Function to set up a preconditioner for the linear
  /// system defined by matrix_pt. This function must be called
  /// before using preconditioner_solve.
  /// Note: matrix_pt must point to an object of class
  /// CRDoubleMatrix or CCDoubleMatrix
  void setup(Problem* problem_pt, DoubleMatrixBase* matrix_pt)
  {
   oomph_info << "Setting up Mumps (exact) preconditioner" 
              << std::endl;
   
   // Wipe previously allocated memory
   Solver.clean_up_memory();
   
   if (dynamic_cast<DistributableLinearAlgebraObject*>(matrix_pt) != 0)
    {
     this->distribution_pt()
      ->build(dynamic_cast<DistributableLinearAlgebraObject*>(matrix_pt)
              ->distribution_pt());
    }
   else
    {
     this->distribution_pt()->build(problem_pt->communicator_pt(),
                                    matrix_pt->nrow(),false);
    }
   Solver.doc_time() = false;
   Solver.distribution_pt()->build(this->distribution_pt());
   Solver.factorise(matrix_pt);
    
  }
  
  /// \short Function applies Mumps to vector r for (exact) 
  /// preconditioning, this requires a call to setup(...) first.
  void preconditioner_solve(const DoubleVector &r, DoubleVector &z)
  {
   Solver.resolve(r, z);
  }
  

  /// \short Clean up memory -- forward the call to the version in
  /// Mumps in its LinearSolver incarnation.
  void clean_up_memory()
  {
   Solver.clean_up_memory();
  }
  
  /// Enable documentation of time
  bool& doc_time() 
   {
    return Solver.doc_time();
   }
  
  private:
  
  /// \short the Mumps solver emplyed by this preconditioner
  MumpsSolver Solver;
};

}

#endif
