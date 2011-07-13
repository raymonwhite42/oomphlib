//LIC// ====================================================================
//LIC// This file forms part of oomph-lib, the object-oriented, 
//LIC// multi-physics finite-element library, available 
//LIC// at http://www.oomph-lib.org.
//LIC// 
//LIC//           Version 0.85. June 9, 2008.
//LIC// 
//LIC// Copyright (C) 2006-2008 Matthias Heil and Andrew Hazel
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
// Driver for axisymmetric two-layer fluid problem.
// Contact line relaxes to equlibrium position.

// Generic oomph-lib header
#include "generic.h"

// Navier-Stokes headers
#include "navier_stokes.h"

// Axisymmetric Navier-Stokes headers
#include "axisym_navier_stokes.h"

// Interface headers
#include "fluid_interface.h"

// Bessel functions
#include "oomph_crbond_bessel.h"

// The mesh
#include "meshes/two_layer_spine_mesh.h"

using namespace std;

using namespace oomph;



//==start_of_namespace===================================================
/// Namespace for physical parameters
//=======================================================================
namespace Global_Physical_Variables
{

 /// Reynolds number
 double Re = 5.0;

 /// Womersley number
 double ReSt = 5.0; // (St = 1)
 
 /// Product of Reynolds number and inverse of Froude number
 double ReInvFr = 5.0; // (Fr = 1)

 /// Capillary number
 double Ca = 0.01;

 /// \short Ratio of viscosity in upper fluid to viscosity in lower
 /// fluid. Reynolds number etc. is based on viscosity in lower fluid.
 double Viscosity_Ratio = 1.0; // Both fluids have equal viscosity

 /// \short Ratio of density in upper fluid to density in lower
 /// fluid. Reynolds number etc. is based on density in lower fluid.
 double Density_Ratio = 1.0; // Both fluids have equal density 

 /// Direction of gravity
 Vector<double> G(3);

} // End of namespace



//==start_of_problem_class===============================================
/// Axisymmetric two fluid interface problem in rectangular domain
//=======================================================================
template<class ELEMENT, class TIMESTEPPER>
class InterfaceProblem : public Problem
{

public:
 
 /// Constructor: Pass the number of elements and the width of the
 /// domain in the r direction. Also pass the number of elements in
 /// the z direction of the bottom (fluid 1) and top (fluid 2) layers,
 /// along with the heights of both layers.
 InterfaceProblem(const unsigned &n_r, const unsigned &n_z1, 
                  const unsigned &n_z2, const double &l_r, 
                  const double &h1, const double &h2);

 /// Destructor (empty)
 ~InterfaceProblem() {}
 
 /// Spine heights/lengths are unknowns in the problem so their values get
 /// corrected during each Newton step. However, changing their value does
 /// not automatically change the nodal positions, so we need to update all
 /// of them here.
 void actions_before_newton_convergence_check()
  {
   mesh_pt()->node_update();
  }

 // Update before solve (empty)
 void actions_before_newton_solve() {}

 /// \short Update after solve can remain empty, because the update 
 /// is performed automatically after every Newton step.
 void actions_after_newton_solve() {}

 /// Set initial conditions: Set all nodal velocities to zero and
 /// initialise the previous velocities to correspond to an impulsive
 /// start
 void set_initial_condition()
  {
   // Determine number of nodes in mesh
   const unsigned n_node = mesh_pt()->nnode();

   // Loop over all nodes in mesh
   for(unsigned n=0;n<n_node;n++)
    {
     // Loop over the three velocity components
     for(unsigned i=0;i<3;i++)
      {
       // Set velocity component i of node n to zero
       mesh_pt()->node_pt(n)->set_value(i,0.0);
      }
    }

   // Initialise the previous velocity values for timestepping
   // corresponding to an impulsive start
   assign_initial_values_impulsive();
   
  } // End of set_initial_condition

 /// \short Access function for the specific mesh
 TwoLayerSpineMesh<ELEMENT,SpineAxisymmetricFluidInterfaceElement<ELEMENT> >* 
 mesh_pt() 
  {
   return dynamic_cast<TwoLayerSpineMesh<ELEMENT,
    SpineAxisymmetricFluidInterfaceElement<ELEMENT> >*>(Problem::mesh_pt());
  }

 /// Doc the solution
 void doc_solution(DocInfo &doc_info);
 
 /// Do unsteady run up to maximum time t_max with given timestep dt
 void unsteady_run(const double &t_max, const double &dt);
 

private:
 
 /// Deform the mesh/free surface to a prescribed function
 void deform_free_surface(const double &epsilon, const double &k)
  {
   // Initialise Bessel functions (only need the first!)
   double j0, j1, y0, y1, j0p, j1p, y0p, y1p;

   // Determine number of spines in mesh
   const unsigned n_spine = mesh_pt()->nspine();
   
   // Loop over spines in mesh
   for(unsigned i=0;i<n_spine;i++)
    {
     // Determine r coordinate of spine
     double r_value = mesh_pt()->boundary_node_pt(0,i)->x(0);    

     // Get Bessel functions J_0(kr), J_1(kr), Y_0(kr), Y_1(kr)
     // and their derivatives
     CRBond_Bessel::bessjy01a(k*r_value,j0,j1,y0,y1,j0p,j1p,y0p,y1p);

     // Set spine height
     mesh_pt()->spine_pt(i)->height() = 1.0 + epsilon*j0;

    } // End of loop over spines
   
   // Update nodes in bulk mesh
   mesh_pt()->node_update();

  } // End of deform_free_surface
 
 /// Fix pressure in element e at pressure dof pdof and set to pvalue
 void fix_pressure(const unsigned &e, const unsigned &pdof, 
                   const double &pvalue)
  {
   // Fix the pressure at that element
   dynamic_cast<ELEMENT*>(mesh_pt()->element_pt(e))->
                          fix_pressure(pdof,pvalue);
  }
 
 /// Trace file
 ofstream Trace_file;

 /// Width of domain
 double Lr;

}; // End of problem class



//==start_of_constructor==================================================
/// Constructor for two fluid interface problem
//========================================================================
template<class ELEMENT, class TIMESTEPPER>
InterfaceProblem<ELEMENT,TIMESTEPPER>::
InterfaceProblem(const unsigned &n_r, const unsigned &n_z1,
                 const unsigned &n_z2, const double &l_r,
                 const double& h1, const double &h2) : Lr(l_r)
{

 // Allocate the timestepper (this constructs the time object as well)
 add_time_stepper_pt(new TIMESTEPPER); 

 // Build and assign mesh (the "false" boolean flag tells the mesh
 // constructor that the domain is not periodic in r)
 Problem::mesh_pt() = new TwoLayerSpineMesh<ELEMENT,
  SpineAxisymmetricFluidInterfaceElement<ELEMENT> >
  (n_r,n_z1,n_z2,l_r,h1,h2,false,time_stepper_pt());

 // --------------------------------------------
 // Set the boundary conditions for this problem
 // --------------------------------------------

 // All nodes are free by default -- just pin the ones that have
 // Dirichlet conditions here

 // Determine number of mesh boundaries
 const unsigned n_boundary = mesh_pt()->nboundary();

 // Loop over mesh boundaries
 for(unsigned b=0;b<n_boundary;b++)
  {
   // Determine number of nodes on boundary b
   const unsigned n_node = mesh_pt()->nboundary_node(b);

   // Loop over nodes on boundary b
   for (unsigned n=0;n<n_node;n++)
    {
     // Pin radial and azimuthal velocities on all boundaries
     // (no slip/penetration)
     mesh_pt()->boundary_node_pt(b,n)->pin(0);
     mesh_pt()->boundary_node_pt(b,n)->pin(2);

     // Pin axial velocity on top (b=2) and bottom (b=0) boundaries
     // (no penetration). Because we have a slippery outer wall we do
     // NOT pin the axial velocity on this boundary (b=1); similarly,
     // we do not pin the axial velocity on the symmetry boundary (b=3).
     if(b==0 || b==2)
      {
       mesh_pt()->boundary_node_pt(b,n)->pin(1);
      }
    } // End of loop over nodes on boundary b
  } // End of loop over mesh boundaries

 // Determine total number of nodes in mesh
 const unsigned n_node = mesh_pt()->nnode();

 // Pin all azimuthal velocities throughout the bulk of the domain
 for(unsigned n=0;n<n_node;n++)
  {
   mesh_pt()->node_pt(n)->pin(2);
  }

 // Fix zeroth pressure value in element 0 to 0.0.
 fix_pressure(0,0,0.0);
 
 // ----------------------------------------------------------------
 // Complete the problem setup to make the elements fully functional
 // ----------------------------------------------------------------

 // Determine number of bulk elements in lower fluid
 const unsigned n_lower = mesh_pt()->nlower();

 // Loop over bulk elements in lower fluid
 for(unsigned e=0;e<n_lower;e++)
  {
   // Upcast from GeneralisedElement to the present element
   ELEMENT *el_pt = dynamic_cast<ELEMENT*>(mesh_pt()->
                                           lower_layer_element_pt(e));

   // Set the Reynolds number
   el_pt->re_pt() = &Global_Physical_Variables::Re;

   // Set the Womersley number
   el_pt->re_st_pt() = &Global_Physical_Variables::ReSt;

   // Set the product of the Reynolds number and the inverse of the
   // Froude number
   el_pt->re_invfr_pt() = &Global_Physical_Variables::ReInvFr;

   // Set the direction of gravity
   el_pt->g_pt() = &Global_Physical_Variables::G;

   // Assign the time pointer
   el_pt->time_pt() = time_pt();

  } // End of loop over bulk elements in lower fluid

 // Determine number of bulk elements in upper fluid
 const unsigned n_upper = mesh_pt()->nupper();
 
 // Loop over bulk elements in upper fluid 
 for(unsigned e=0;e<n_upper;e++)
  {
   // Upcast from GeneralisedElement to the present element
   ELEMENT *el_pt = dynamic_cast<ELEMENT*>(mesh_pt()->
                                           upper_layer_element_pt(e));

   // Set the Reynolds number
   el_pt->re_pt() = &Global_Physical_Variables::Re;

   // Set the Womersley number
   el_pt->re_st_pt() = &Global_Physical_Variables::ReSt;

   // Set the product of the Reynolds number and the inverse of the
   // Froude number
   el_pt->re_invfr_pt() = &Global_Physical_Variables::ReInvFr;

   // Set the direction of gravity
   el_pt->g_pt() = &Global_Physical_Variables::G;

   // Set the viscosity ratio
   el_pt->viscosity_ratio_pt() = &Global_Physical_Variables::Viscosity_Ratio;

   // Set the density ratio
   el_pt->density_ratio_pt() = &Global_Physical_Variables::Density_Ratio;

   // Assign the time pointer
   el_pt->time_pt() = time_pt();

  } // End of loop over bulk elements in upper fluid

 // Determine number of 1D interface elements in mesh
 unsigned n_interface_element = mesh_pt()->ninterface_element();

 // Loop over interface elements
 for(unsigned e=0;e<n_interface_element;e++)
  {
   // Upcast from GeneralisedElement to the present element
   SpineAxisymmetricFluidInterfaceElement<ELEMENT>* el_pt = 
    dynamic_cast<SpineAxisymmetricFluidInterfaceElement<ELEMENT>*>
    (mesh_pt()->interface_element_pt(e));

   // Set the Capillary number
   el_pt->ca_pt() = &Global_Physical_Variables::Ca;

  } // End of loop over interface elements

 // Setup equation numbering scheme
 cout << "Number of equations: " << assign_eqn_numbers() << std::endl;

} // End of constructor



//==start_of_doc_solution=================================================
/// Doc the solution
//========================================================================
template<class ELEMENT, class TIMESTEPPER>
void InterfaceProblem<ELEMENT,TIMESTEPPER>::doc_solution(DocInfo &doc_info)
{ 

 // Output the time
 cout << "Time is now " << time_pt()->time() << std::endl;
 
 // Determine number of 1D interface elements in mesh
 //const unsigned n_interface_element = mesh_pt()->ninterface_element();
 
 // Calculate left contact angle in degrees
 const double contact_angle_left = 0.0; // hierher
//   dynamic_cast<SpineAxisymmetricFluidInterfaceElement<ELEMENT>*>(
//    mesh_pt()->interface_element_pt(0))->
//   actual_contact_angle_left()*180.0/MathematicalConstants::Pi;

 // Calculate right contact angle in degrees
 const double contact_angle_right = 0.0; // hierher
//   dynamic_cast<SpineAxisymmetricFluidInterfaceElement<ELEMENT>*>(
//    mesh_pt()->interface_element_pt(n_interface_element-1))->
//   actual_contact_angle_right()*180.0/MathematicalConstants::Pi;
 
 // Document in trace file
 Trace_file << time_pt()->time() << " "
            << mesh_pt()->spine_pt(0)->height() << " "
            << contact_angle_left << " "
            << contact_angle_right << std::endl;

 ofstream some_file;
 char filename[100];
 
 // Set number of plot points (in each coordinate direction)
 const unsigned npts = 5;
 
 // Open solution output file
 sprintf(filename,"%s/soln%i.dat",doc_info.directory().c_str(),
         doc_info.number());
 some_file.open(filename);

 // Output solution to file
 mesh_pt()->output(some_file,npts);

 // Close solution output file
 some_file.close();

} // End of doc_solution



//==start_of_unsteady_run=================================================
/// Perform run up to specified time t_max with given timestep dt
//========================================================================
template<class ELEMENT, class TIMESTEPPER>
void InterfaceProblem<ELEMENT,TIMESTEPPER>::
unsteady_run(const double &t_max, const double &dt)
{

 // Set value of epsilon
 const double epsilon = 0.2;
 
 // Set value of k in Bessel function J_0(kr)
 const double k = 7.0156; // Value of the first zero of J_1(k)

 // Deform the mesh/free surface
 deform_free_surface(epsilon, k);

 // Initialise DocInfo object
 DocInfo doc_info;

 // Set output directory
 doc_info.set_directory("RESLT");

 // Initialise counter for solutions
 doc_info.number()=0;
 
 // Open trace file
 char filename[100];   
 sprintf(filename,"%s/trace.dat",doc_info.directory().c_str());
 Trace_file.open(filename);
 
 // Initialise trace file
 Trace_file << "time" << ", "
            << "edge spine height" << ", "
            << "contact angle left" << ", "
            << "contact angle right" << ", " << std::endl;

 // Initialise timestep
 initialise_dt(dt);

 // Set initial conditions
 set_initial_condition();
 
 // Determine number of timesteps
 const unsigned n_timestep = unsigned(t_max/dt);
 
 // Doc initial solution
 doc_solution(doc_info);
 
 // Increment counter for solutions 
 doc_info.number()++;
 
 // Timestepping loop
 for(unsigned t=1;t<=n_timestep;t++)
  {
   // Output current timestep to screen
   cout << "\nTimestep " << t << " of " << n_timestep << std::endl;
   
   // Take one fixed timestep
   unsteady_newton_solve(dt);

   // Doc solution
   doc_solution(doc_info);

   // Increment counter for solutions 
   doc_info.number()++;

  } // End of timestepping loop  }

} // End of unsteady_run


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


//==start_of_main======================================================
/// Driver code for two fluid axisymmetric interface problem
//=====================================================================
int main(int argc, char* argv[]) 
{

 // Store command line arguments
 CommandLineArgs::setup(argc,argv);

 /// Maximum time
 double t_max = 0.8;

 /// Duration of timestep
 const double dt = 0.005;

 // If we are doing validation run, use smaller number of timesteps
 if(CommandLineArgs::Argc>1) { t_max = 0.01; }

 // Number of elements in radial (r) direction
 const unsigned n_r = 16;
   
 // Number of elements in axial (z) direction in bottom fluid (fluid 1)
 const unsigned n_z1 = 12;
   
 // Number of elements in axial (z) direction in top fluid (fluid 2)
 const unsigned n_z2 = 12;

 // Width of domain
 const double l_r = 1.0;

 // Height of bottom fluid layer
 const double h1 = 1.0;
   
 // Height of top fluid layer
 const double h2 = 1.0;

 // Set direction of gravity (vertically downwards)
 Global_Physical_Variables::G[0] = 0.0;
 Global_Physical_Variables::G[1] = -1.0;
 Global_Physical_Variables::G[2] = 0.0;

 // Set up the spine test problem with AxisymmetricQCrouzeixRaviartElements,
 // using the BDF<2> timestepper
 InterfaceProblem<SpineElement<AxisymmetricQCrouzeixRaviartElement >,BDF<2> >
  problem(n_r,n_z1,n_z2,l_r,h1,h2);
   
 // Run the unsteady simulation
 problem.unsteady_run(t_max,dt);

} // End of main