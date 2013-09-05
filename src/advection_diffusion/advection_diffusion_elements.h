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
//Header file for Advection Diffusion elements
#ifndef OOMPH_ADV_DIFF_ELEMENTS_HEADER
#define OOMPH_ADV_DIFF_ELEMENTS_HEADER


// Config header generated by autoconfig
#ifdef HAVE_CONFIG_H
  #include <oomph-lib-config.h>
#endif

//OOMPH-LIB headers
#include "../generic/nodes.h"
#include "../generic/Qelements.h"
#include "../generic/oomph_utilities.h"

namespace oomph
{

//=============================================================
/// \short A class for all elements that solve the Advection 
/// Diffusion equations using isoparametric elements.
/// \f[ 
/// \frac{\partial^2 u}{\partial x_i^2} = 
/// Pe w_i(x_k) \frac{\partial u}{\partial x_i} + f(x_j)
/// \f] 
/// This contains the generic maths. Shape functions, geometric
/// mapping etc. must get implemented in derived class.
//=============================================================
template <unsigned DIM>
class AdvectionDiffusionEquations : public virtual FiniteElement
{

public:

 /// \short Function pointer to source function fct(x,f(x)) -- 
 /// x is a Vector! 
 typedef void (*AdvectionDiffusionSourceFctPt)(const Vector<double>& x,
                                               double& f);


 /// \short Function pointer to wind function fct(x,w(x)) -- 
 /// x is a Vector! 
 typedef void (*AdvectionDiffusionWindFctPt)(const Vector<double>& x,
                                             Vector<double>& wind);


 /// \short Constructor: Initialise the Source_fct_pt and Wind_fct_pt 
 /// to null and set (pointer to) Peclet number to default
 AdvectionDiffusionEquations() : Source_fct_pt(0), Wind_fct_pt(0), 
  ALE_is_disabled(false)
  {
   //Set Peclet number to default
   Pe_pt = &Default_peclet_number;
   //Set Peclet Strouhal number to default
   PeSt_pt = &Default_peclet_number;
  }
 
 /// Broken copy constructor
 AdvectionDiffusionEquations(const AdvectionDiffusionEquations& dummy) 
  { 
   BrokenCopy::broken_copy("AdvectionDiffusionEquations");
  } 
 
 /// Broken assignment operator
 void operator=(const AdvectionDiffusionEquations&) 
  {
   BrokenCopy::broken_assign("AdvectionDiffusionEquations");
  }

 /// \short Return the index at which the unknown value
 /// is stored. The default value, 0, is appropriate for single-physics
 /// problems, when there is only one variable, the value that satisfies
 /// the advection-diffusion equation. 
 /// In derived multi-physics elements, this function should be overloaded
 /// to reflect the chosen storage scheme. Note that these equations require
 /// that the unknown is always stored at the same index at each node.
 virtual inline unsigned u_index_adv_diff() const {return 0;}

/// \short du/dt at local node n. 
 /// Uses suitably interpolated value for hanging nodes.
 double du_dt_adv_diff(const unsigned &n) const
  {
   // Get the data's timestepper
   TimeStepper* time_stepper_pt= this->node_pt(n)->time_stepper_pt();

   //Initialise dudt
   double dudt=0.0;
   //Loop over the timesteps, if there is a non Steady timestepper
   if (!time_stepper_pt->is_steady())
    {
     //Find the index at which the variable is stored
     const unsigned u_nodal_index = u_index_adv_diff();

     // Number of timsteps (past & present)
     const unsigned n_time = time_stepper_pt->ntstorage();
     
     for(unsigned t=0;t<n_time;t++)
      {
       dudt += time_stepper_pt->weight(1,t)*nodal_value(t,n,u_nodal_index);
      }
    }
   return dudt;
  }

 /// \short Disable ALE, i.e. assert the mesh is not moving -- you do this
 /// at your own risk!
 void disable_ALE()
  {
   ALE_is_disabled=true;
  }


 /// \short (Re-)enable ALE, i.e. take possible mesh motion into account
 /// when evaluating the time-derivative. Note: By default, ALE is 
 /// enabled, at the expense of possibly creating unnecessary work 
 /// in problems where the mesh is, in fact, stationary. 
 void enable_ALE()
  {
   ALE_is_disabled=false;
  }

/// \short Number of scalars/fields output by this element. Reimplements
 /// broken virtual function in base class.
 unsigned nscalar_paraview() const
  {
   return DIM+1;
  }

 /// \short Write values of the i-th scalar field at the plot points. Needs 
 /// to be implemented for each new specific element type.
 void scalar_value_paraview(std::ofstream& file_out,
                            const unsigned& i,
                            const unsigned& nplot) const
  {
   //Vector of local coordinates
   Vector<double> s(DIM);

   // Loop over plot points
   unsigned num_plot_points=nplot_points_paraview(nplot);
   for (unsigned iplot=0;iplot<num_plot_points;iplot++)
    {

     // Get local coordinates of plot point
     get_s_plot(iplot,nplot,s);
     
     // Get Eulerian coordinate of plot point
     Vector<double> x(DIM);
     interpolated_x(s,x);

     if(i<DIM) 
      {
       // Get the wind
       Vector<double> wind(DIM);
       
       // Dummy ipt argument needed... ?
       unsigned ipt=0;
       get_wind_adv_diff(ipt,s,x,wind);

       file_out << wind[i] << std::endl;
      }     
     // Advection Diffusion
     else if(i==DIM)
      {
       file_out << interpolated_u_adv_diff(s) << std::endl;
      }
     // Never get here
     else 
      {
       std::stringstream error_stream;
       error_stream 
        << "Advection Diffusion Elements only store " << DIM+1 << " fields "
        << std::endl;
       throw OomphLibError(
        error_stream.str(),
        OOMPH_CURRENT_FUNCTION,
        OOMPH_EXCEPTION_LOCATION);
      }
    }
  }

 /// \short Name of the i-th scalar field. Default implementation
 /// returns V1 for the first one, V2 for the second etc. Can (should!) be
 /// overloaded with more meaningful names in specific elements.
 string scalar_name_paraview(const unsigned& i) const
  {
    // Winds
   if(i<DIM)
    {
     return "Wind "+StringConversion::to_string(i);
    }
   // Advection Diffusion field
   else if(i==DIM) 
    {
     return "Advection Diffusion";
    }
   // Never get here
   else
    {
     std::stringstream error_stream;
     error_stream
      << "Advection Diffusion Elements only store " << DIM+1 << " fields"
      << std::endl;
     throw OomphLibError(
      error_stream.str(),
      OOMPH_CURRENT_FUNCTION,
      OOMPH_EXCEPTION_LOCATION);
     // Dummy return
     return " ";
    }
  }
 
 /// Output with default number of plot points
 void output(std::ostream &outfile) 
  {
   unsigned nplot=5;
   output(outfile,nplot);
  }

 /// \short Output FE representation of soln: x,y,u or x,y,z,u at 
 /// nplot^DIM plot points
 void output(std::ostream &outfile, const unsigned &nplot);


 /// C_style output with default number of plot points
 void output(FILE* file_pt)
  {
   unsigned n_plot=5;
   output(file_pt,n_plot);
  }

 /// \short C-style output FE representation of soln: x,y,u or x,y,z,u at 
 /// n_plot^DIM plot points
 void output(FILE* file_pt, const unsigned &n_plot);


 /// Output exact soln: x,y,u_exact or x,y,z,u_exact at nplot^DIM plot points
 void output_fct(std::ostream &outfile, const unsigned &nplot, 
                 FiniteElement::SteadyExactSolutionFctPt 
                 exact_soln_pt);

 /// \short Output exact soln: x,y,u_exact or x,y,z,u_exact at 
 /// nplot^DIM plot points (dummy time-dependent version to 
 /// keep intel compiler happy)
 virtual void output_fct(std::ostream &outfile, const unsigned &nplot,
                         const double& time, 
  FiniteElement::UnsteadyExactSolutionFctPt exact_soln_pt)
  {
   throw OomphLibError(
    "There is no time-dependent output_fct() for Advection Diffusion elements",
    OOMPH_CURRENT_FUNCTION,
    OOMPH_EXCEPTION_LOCATION);
  }


 /// Get error against and norm of exact solution
 void compute_error(std::ostream &outfile, 
                    FiniteElement::SteadyExactSolutionFctPt 
                    exact_soln_pt, double& error, double& norm);


 /// Dummy, time dependent error checker
 void compute_error(std::ostream &outfile, 
                    FiniteElement::UnsteadyExactSolutionFctPt 
                    exact_soln_pt,
                    const double& time, double& error, double& norm)
  {
   throw OomphLibError(
    "No time-dependent compute_error() for Advection Diffusion elements",
    OOMPH_CURRENT_FUNCTION,
    OOMPH_EXCEPTION_LOCATION);
  }


 /// Access function: Pointer to source function
 AdvectionDiffusionSourceFctPt& source_fct_pt() {return Source_fct_pt;}


 /// Access function: Pointer to source function. Const version
 AdvectionDiffusionSourceFctPt source_fct_pt() const {return Source_fct_pt;}


 /// Access function: Pointer to wind function
 AdvectionDiffusionWindFctPt& wind_fct_pt() {return Wind_fct_pt;}


 /// Access function: Pointer to wind function. Const version
 AdvectionDiffusionWindFctPt wind_fct_pt() const {return Wind_fct_pt;}

 /// Peclet number
 const double &pe() const {return *Pe_pt;}

 /// Pointer to Peclet number
 double* &pe_pt() {return Pe_pt;}

 /// Peclet number multiplied by Strouhal number
 const double &pe_st() const {return *PeSt_pt;}

 /// Pointer to Peclet number multipled by Strouha number
 double* &pe_st_pt() {return PeSt_pt;}

 /// \short Get source term at (Eulerian) position x. This function is
 /// virtual to allow overloading in multi-physics problems where
 /// the strength of the source function might be determined by
 /// another system of equations 
 inline virtual void get_source_adv_diff(const unsigned& ipt,
                                         const Vector<double>& x,
                                         double& source) const
  {
   //If no source function has been set, return zero
   if(Source_fct_pt==0) {source = 0.0;}
   else
    {
     // Get source strength
     (*Source_fct_pt)(x,source);
    }
  }

 /// \short Get wind at (Eulerian) position x and/or local coordinate s. 
 /// This function is
 /// virtual to allow overloading in multi-physics problems where
 /// the wind function might be determined by
 /// another system of equations 
 inline virtual void get_wind_adv_diff(const unsigned& ipt,
                                       const Vector<double> &s,
                                       const Vector<double>& x,
                                       Vector<double>& wind) const
  {
   //If no wind function has been set, return zero
   if(Wind_fct_pt==0)
    {
     for(unsigned i=0;i<DIM;i++) {wind[i]= 0.0;}
    }
   else
    {
     // Get wind
     (*Wind_fct_pt)(x,wind);
    }
  }

 /// Get flux: \f$\mbox{flux}[i] = \mbox{d}u / \mbox{d}x_i \f$
 void get_flux(const Vector<double>& s, Vector<double>& flux) const
  {
   //Find out how many nodes there are in the element
   unsigned n_node = nnode();
   
   //Get the nodal index at which the unknown is stored
   unsigned u_nodal_index = u_index_adv_diff();

   //Set up memory for the shape and test functions
   Shape psi(n_node);
   DShape dpsidx(n_node,DIM);
 
   //Call the derivatives of the shape and test functions
   dshape_eulerian(s,psi,dpsidx);
     
   //Initialise to zero
   for(unsigned j=0;j<DIM;j++) {flux[j] = 0.0;}
   
   // Loop over nodes
   for(unsigned l=0;l<n_node;l++) 
    {
     //Loop over derivative directions
     for(unsigned j=0;j<DIM;j++)
      {                               
       flux[j] += nodal_value(l,u_nodal_index)*dpsidx(l,j);
      }
    }
  }

 
 /// Add the element's contribution to its residual vector (wrapper)
 void fill_in_contribution_to_residuals(Vector<double> &residuals)
  {
   //Call the generic residuals function with flag set to 0 and using
   //a dummy matrix
   fill_in_generic_residual_contribution_adv_diff(
    residuals,GeneralisedElement::Dummy_matrix,
    GeneralisedElement::Dummy_matrix,0);
  }

 
 /// \short Add the element's contribution to its residual vector and 
 /// the element Jacobian matrix (wrapper)
 void fill_in_contribution_to_jacobian(Vector<double> &residuals,
                                   DenseMatrix<double> &jacobian)
  {
   //Call the generic routine with the flag set to 1
   fill_in_generic_residual_contribution_adv_diff(
    residuals,jacobian,GeneralisedElement::Dummy_matrix,1);
  }
 

 /// Add the element's contribution to its residuals vector,
 /// jacobian matrix and mass matrix
 void fill_in_contribution_to_jacobian_and_mass_matrix(
  Vector<double> &residuals, DenseMatrix<double> &jacobian, 
  DenseMatrix<double> &mass_matrix)
  {
   //Call the generic routine with the flag set to 2
   fill_in_generic_residual_contribution_adv_diff(residuals,
                                                  jacobian,mass_matrix,2);
  }


 /// Return FE representation of function value u(s) at local coordinate s
 inline double interpolated_u_adv_diff(const Vector<double> &s) const
  {
   //Find number of nodes
   unsigned n_node = nnode();

   //Get the nodal index at which the unknown is stored
   unsigned u_nodal_index = u_index_adv_diff();

   //Local shape function
   Shape psi(n_node);

   //Find values of shape function
   shape(s,psi);

   //Initialise value of u
   double interpolated_u = 0.0;

   //Loop over the local nodes and sum
   for(unsigned l=0;l<n_node;l++) 
    {
     interpolated_u += nodal_value(l,u_nodal_index)*psi[l];
    }

   return(interpolated_u);
  }


 ///\short Return derivative of u at point s with respect to all data
 ///that can affect its value.
 ///In addition, return the global equation numbers corresponding to the
 ///data. This is virtual so that it can be overloaded in the
 ///refineable version
 virtual void dinterpolated_u_adv_diff_ddata(
  const Vector<double> &s, Vector<double> &du_ddata,
  Vector<unsigned> &global_eqn_number)
  {
   //Find number of nodes
   const unsigned n_node = nnode();

   //Get the nodal index at which the unknown is stored
   const unsigned u_nodal_index = u_index_adv_diff();

   //Local shape function
   Shape psi(n_node);

   //Find values of shape function
   shape(s,psi);

   //Find the number of dofs associated with interpolated u
   unsigned n_u_dof=0;
   for(unsigned l=0;l<n_node;l++) 
    {
     int global_eqn = this->node_pt(l)->eqn_number(u_nodal_index);
     //If it's positive add to the count
     if (global_eqn >=0) {++n_u_dof;}
    }
     
   //Now resize the storage schemes
   du_ddata.resize(n_u_dof,0.0); 
   global_eqn_number.resize(n_u_dof,0);
   
   //Loop over the nodes again and set the derivatives
   unsigned count=0;
   for(unsigned l=0;l<n_node;l++)
    {
     //Get the global equation number
     int global_eqn=this->node_pt(l)->eqn_number(u_nodal_index);
     //If it's positive
     if (global_eqn >= 0)
      {
       //Set the global equation number
       global_eqn_number[count] = global_eqn;
       //Set the derivative with respect to the unknown
       du_ddata[count] = psi[l];
       //Increase the counter
       ++count;
      }
    }
  }


 /// \short Self-test: Return 0 for OK
 unsigned self_test();

protected:


 /// \short Shape/test functions and derivs w.r.t. to global coords at 
 /// local coord. s; return  Jacobian of mapping
 virtual double dshape_and_dtest_eulerian_adv_diff(const Vector<double> &s, 
                                                   Shape &psi, 
                                                   DShape &dpsidx, 
                                                   Shape &test, 
                                                   DShape &dtestdx) const=0;

 /// \short Shape/test functions and derivs w.r.t. to global coords at 
 /// integration point ipt; return  Jacobian of mapping
 virtual double dshape_and_dtest_eulerian_at_knot_adv_diff(
  const unsigned &ipt, 
  Shape &psi, 
  DShape &dpsidx,
  Shape &test, 
  DShape &dtestdx) 
  const=0;

 /// \short Add the element's contribution to its residual vector only 
 /// (if flag=and/or element  Jacobian matrix 
 virtual void fill_in_generic_residual_contribution_adv_diff(
  Vector<double> &residuals, DenseMatrix<double> &jacobian, 
  DenseMatrix<double> &mass_matrix, unsigned flag); 
  
 /// Pointer to global Peclet number
 double *Pe_pt;

 /// Pointer to global Peclet number multiplied by Strouhal number
 double *PeSt_pt;

 /// Pointer to source function:
 AdvectionDiffusionSourceFctPt Source_fct_pt;
 
 /// Pointer to wind function:
 AdvectionDiffusionWindFctPt Wind_fct_pt;
 
 /// \short Boolean flag to indicate if ALE formulation is disabled when 
 /// time-derivatives are computed. Only set to true if you're sure
 /// that the mesh is stationary.
 bool ALE_is_disabled;

  private:

 /// Static default value for the Peclet number
 static double Default_peclet_number;
 
  
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



//======================================================================
/// \short QAdvectionDiffusionElement elements are 
/// linear/quadrilateral/brick-shaped Advection Diffusion elements with 
/// isoparametric interpolation for the function.
//======================================================================
template <unsigned DIM, unsigned NNODE_1D>
 class QAdvectionDiffusionElement : public virtual QElement<DIM,NNODE_1D>,
 public virtual AdvectionDiffusionEquations<DIM>
{

private:

 /// \short Static array of ints to hold number of variables at 
 /// nodes: Initial_Nvalue[n]
 static const unsigned Initial_Nvalue;
 
  public:


 ///\short  Constructor: Call constructors for QElement and 
 /// Advection Diffusion equations
 QAdvectionDiffusionElement() : QElement<DIM,NNODE_1D>(), 
  AdvectionDiffusionEquations<DIM>()
  { }

 /// Broken copy constructor
 QAdvectionDiffusionElement(const QAdvectionDiffusionElement<DIM,NNODE_1D>& 
                            dummy) 
  { 
   BrokenCopy::broken_copy("QAdvectionDiffusionElement");
  } 
 
 /// Broken assignment operator
 void operator=(const QAdvectionDiffusionElement<DIM,NNODE_1D>&) 
  {
   BrokenCopy::broken_assign("QAdvectionDiffusionElement");
  }

 /// \short  Required  # of `values' (pinned or dofs) 
 /// at node n
 inline unsigned required_nvalue(const unsigned &n) const 
  {return Initial_Nvalue;}

 /// \short Output function:  
 ///  x,y,u   or    x,y,z,u
 void output(std::ostream &outfile)
  {AdvectionDiffusionEquations<DIM>::output(outfile);}

 /// \short Output function:  
 ///  x,y,u   or    x,y,z,u at n_plot^DIM plot points
 void output(std::ostream &outfile, const unsigned &n_plot)
  {AdvectionDiffusionEquations<DIM>::output(outfile,n_plot);}


 /// \short C-style output function:  
 ///  x,y,u   or    x,y,z,u
 void output(FILE* file_pt)
  {
   AdvectionDiffusionEquations<DIM>::output(file_pt);
  }

 ///  \short C-style output function:  
 ///   x,y,u   or    x,y,z,u at n_plot^DIM plot points
 void output(FILE* file_pt, const unsigned &n_plot)
  {
   AdvectionDiffusionEquations<DIM>::output(file_pt,n_plot);
  }

 /// \short Output function for an exact solution:
 ///  x,y,u_exact   or    x,y,z,u_exact at n_plot^DIM plot points
 void output_fct(std::ostream &outfile, const unsigned &n_plot,
                 FiniteElement::SteadyExactSolutionFctPt 
                 exact_soln_pt)
  {AdvectionDiffusionEquations<DIM>::output_fct(outfile,n_plot,exact_soln_pt);}


 /// \short Output function for a time-dependent exact solution.
 ///  x,y,u_exact   or    x,y,z,u_exact at n_plot^DIM plot points
 /// (Calls the steady version)
 void output_fct(std::ostream &outfile, const unsigned &n_plot,
                 const double& time,
                 FiniteElement::UnsteadyExactSolutionFctPt 
                 exact_soln_pt)
  {
   AdvectionDiffusionEquations<DIM>::
    output_fct(outfile,n_plot,time,exact_soln_pt);
  }


protected:

 /// Shape, test functions & derivs. w.r.t. to global coords. Return Jacobian.
 inline double dshape_and_dtest_eulerian_adv_diff(const Vector<double> &s, 
                                                  Shape &psi, 
                                                  DShape &dpsidx, 
                                                  Shape &test, 
                                                  DShape &dtestdx) const;
 
 /// \short Shape, test functions & derivs. w.r.t. to global coords. at
 /// integration point ipt. Return Jacobian.
 inline double dshape_and_dtest_eulerian_at_knot_adv_diff(const unsigned& ipt,
                                                          Shape &psi, 
                                                          DShape &dpsidx, 
                                                          Shape &test,
                                                          DShape &dtestdx) 
  const;

};

//Inline functions:


//======================================================================
/// \short Define the shape functions and test functions and derivatives
/// w.r.t. global coordinates and return Jacobian of mapping.
///
/// Galerkin: Test functions = shape functions
//======================================================================
template<unsigned DIM, unsigned NNODE_1D>
double QAdvectionDiffusionElement<DIM,NNODE_1D>::
 dshape_and_dtest_eulerian_adv_diff(const Vector<double> &s,
                                    Shape &psi, 
                                    DShape &dpsidx,
                                    Shape &test, 
                                    DShape &dtestdx) const
{
 //Call the geometrical shape functions and derivatives  
 double J = this->dshape_eulerian(s,psi,dpsidx);

 //Loop over the test functions and derivatives and set them equal to the
 //shape functions
 for(unsigned i=0;i<NNODE_1D;i++)
  {
   test[i] = psi[i]; 
   for(unsigned j=0;j<DIM;j++)
    {
     dtestdx(i,j) = dpsidx(i,j);
    }
  }

 //Return the jacobian
 return J;
}



//======================================================================
/// Define the shape functions and test functions and derivatives
/// w.r.t. global coordinates and return Jacobian of mapping.
///
/// Galerkin: Test functions = shape functions
//======================================================================
template<unsigned DIM, unsigned NNODE_1D>
double QAdvectionDiffusionElement<DIM,NNODE_1D>::
 dshape_and_dtest_eulerian_at_knot_adv_diff(
 const unsigned &ipt,
 Shape &psi, 
 DShape &dpsidx,
 Shape &test, 
 DShape &dtestdx) const
{
 //Call the geometrical shape functions and derivatives  
 double J = this->dshape_eulerian_at_knot(ipt,psi,dpsidx);

 //Set the test functions equal to the shape functions (pointer copy)
 test = psi;
 dtestdx = dpsidx;

 //Return the jacobian
 return J;
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////



//=======================================================================
/// \short Face geometry for the QAdvectionDiffusionElement elements: 
/// The spatial dimension of the face elements is one lower than that 
/// of the bulk element but they have the same number of points along 
/// their 1D edges.
//=======================================================================
template<unsigned DIM, unsigned NNODE_1D>
class FaceGeometry<QAdvectionDiffusionElement<DIM,NNODE_1D> >: 
 public virtual QElement<DIM-1,NNODE_1D>
{

  public:
 
 /// \short Constructor: Call the constructor for the
 /// appropriate lower-dimensional QElement
 FaceGeometry() : QElement<DIM-1,NNODE_1D>() {}

};



////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


//=======================================================================
/// Face geometry for the 1D QAdvectionDiffusion elements: Point elements
//=======================================================================
template<unsigned NNODE_1D>
class FaceGeometry<QAdvectionDiffusionElement<1,NNODE_1D> >: 
 public virtual PointElement
{

  public:
 
 /// \short Constructor: Call the constructor for the
 /// appropriate lower-dimensional QElement
 FaceGeometry() : PointElement() {}

};

}

#endif
