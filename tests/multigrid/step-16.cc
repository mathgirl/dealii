/* $Id$ */
/* Author: Guido Kanschat, University of Heidelberg, 2003  */
/*         Baerbel Janssen, University of Heidelberg, 2010 */
/*         Wolfgang Bangerth, Texas A&M University, 2010   */

/*    $Id$       */
/*                                                                */
/*    Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2013 by the deal.II authors                   */
/*                                                                */
/*    This file is subject to QPL and may not be  distributed     */
/*    without copyright and license information. Please refer     */
/*    to the file deal.II/doc/license.html for the  text  and     */
/*    further information on this license.                        */

#include "../tests.h"
#include <deal.II/base/logstream.h>

				 // As discussed in the introduction, most of
				 // this program is copied almost verbatim
				 // from step-6, which itself is only a slight
				 // modification of step-5. Consequently, a
				 // significant part of this program is not
				 // new if you've read all the material up to
				 // step-6, and we won't comment on that part
				 // of the functionality that is
				 // unchanged. Rather, we will focus on those
				 // aspects of the program that have to do
				 // with the multigrid functionality which
				 // forms the new aspect of this tutorial
				 // program.

                                 // @sect3{Include files}

				 // Again, the first few include files
				 // are already known, so we won't
				 // comment on them:
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/function.h>
#include <deal.II/base/logstream.h>
#include <deal.II/base/utilities.h>

#include <deal.II/lac/constraint_matrix.h>
#include <deal.II/lac/vector.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/sparse_matrix.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/precondition.h>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_refinement.h>
#include <deal.II/grid/tria_boundary_lib.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/numerics/vector_tools.h>
#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/error_estimator.h>

				 // These, now, are the include necessary for
				 // the multi-level methods. The first two
				 // declare classes that allow us to enumerate
				 // degrees of freedom not only on the finest
				 // mesh level, but also on intermediate
				 // levels (that's what the MGDoFHandler class
				 // does) as well as allow to access this
				 // information (iterators and accessors over
				 // these cells).
				 //
				 // The rest of the include files deals with
				 // the mechanics of multigrid as a linear
				 // operator (solver or preconditioner).
#include <deal.II/multigrid/mg_dof_handler.h>
#include <deal.II/multigrid/multigrid.h>
#include <deal.II/multigrid/mg_transfer.h>
#include <deal.II/multigrid/mg_tools.h>
#include <deal.II/multigrid/mg_coarse.h>
#include <deal.II/multigrid/mg_smoother.h>
#include <deal.II/multigrid/mg_matrix.h>

				 // This is C++:
#include <fstream>
#include <sstream>

				 // The last step is as in all
				 // previous programs:
using namespace dealii;


                                 // @sect3{The <code>LaplaceProblem</code> class template}

				 // This main class is basically the same
				 // class as in step-6. As far as member
				 // functions is concerned, the only addition
				 // is the <code>assemble_multigrid</code>
				 // function that assembles the matrices that
				 // correspond to the discrete operators on
				 // intermediate levels:
template <int dim>
class LaplaceProblem
{
  public:
    LaplaceProblem (const unsigned int deg);
    void run ();

  private:
    void setup_system ();
    void assemble_system ();
    void assemble_multigrid ();
    void solve ();
    void refine_grid ();
    void output_results (const unsigned int cycle) const;

    Triangulation<dim>   triangulation;
    FE_Q<dim>            fe;
    MGDoFHandler<dim>    mg_dof_handler;

    SparsityPattern      sparsity_pattern;
    SparseMatrix<double> system_matrix;

    ConstraintMatrix     hanging_node_constraints;
    ConstraintMatrix     constraints;

    Vector<double>       solution;
    Vector<double>       system_rhs;

    const unsigned int degree;

				     // The following three objects are the
				     // only additional member variables,
				     // compared to step-6. They represent the
				     // operators that act on individual
				     // levels of the multilevel hierarchy,
				     // rather than on the finest mesh as do
				     // the objects above.
				     //
				     // To facilitate having objects on each
				     // level of a multilevel hierarchy,
				     // deal.II has the MGLevelObject class
				     // template that provides storage for
				     // objects on each level. What we need
				     // here are matrices on each level, which
				     // implies that we also need sparsity
				     // patterns on each level. As outlined in
				     // the @ref mg_paper, the operators
				     // (matrices) that we need are actually
				     // twofold: one on the interior of each
				     // level, and one at the interface
				     // between each level and that part of
				     // the domain where the mesh is
				     // coarser. In fact, we will need the
				     // latter in two versions: for the
				     // direction from coarse to fine mesh and
				     // from fine to coarse. Fortunately,
				     // however, we here have a self-adjoint
				     // problem for which one of these is the
				     // transpose of the other, and so we only
				     // have to build one; we choose the one
				     // from coarse to fine.
    MGLevelObject<SparsityPattern>       mg_sparsity_patterns;
    MGLevelObject<SparseMatrix<double> > mg_matrices;
    MGLevelObject<SparseMatrix<double> > mg_interface_matrices;
    MGConstrainedDoFs                    mg_constrained_dofs;
};



                                 // @sect3{Nonconstant coefficients}

				 // The implementation of nonconstant
				 // coefficients is copied verbatim
				 // from step-5 and step-6:

template <int dim>
class Coefficient : public Function<dim>
{
  public:
    Coefficient () : Function<dim>() {}

    virtual double value (const Point<dim>   &p,
			  const unsigned int  component = 0) const;

    virtual void value_list (const std::vector<Point<dim> > &points,
			     std::vector<double>            &values,
			     const unsigned int              component = 0) const;
};



template <int dim>
double Coefficient<dim>::value (const Point<dim> &p,
				const unsigned int) const
{
  if (p.square() < 0.5*0.5)
    return 20;
  else
    return 1;
}



template <int dim>
void Coefficient<dim>::value_list (const std::vector<Point<dim> > &points,
				   std::vector<double>            &values,
				   const unsigned int              component) const
{
  const unsigned int n_points = points.size();

  Assert (values.size() == n_points,
	  ExcDimensionMismatch (values.size(), n_points));

  Assert (component == 0,
	  ExcIndexRange (component, 0, 1));

  for (unsigned int i=0; i<n_points; ++i)
    values[i] = Coefficient<dim>::value (points[i]);
}


                                 // @sect3{The <code>LaplaceProblem</code> class implementation}

                                 // @sect4{LaplaceProblem::LaplaceProblem}

				 // The constructor is left mostly
				 // unchanged. We take the polynomial degree
				 // of the finite elements to be used as a
				 // constructor argument and store it in a
				 // member variable.
				 //
				 // By convention, all adaptively refined
				 // triangulations in deal.II never change by
				 // more than one level across a face between
				 // cells. For our multigrid algorithms,
				 // however, we need a slightly stricter
				 // guarantee, namely that the mesh also does
				 // not change by more than refinement level
				 // across vertices that might connect two
				 // cells. In other words, we must prevent the
				 // following situation:
				 //
				 // @image html limit_level_difference_at_vertices.png ""
				 //
				 // This is achieved by passing the
				 // Triangulation::limit_level_difference_at_vertices
				 // flag to the constructor of the
				 // triangulation class.
template <int dim>
LaplaceProblem<dim>::LaplaceProblem (const unsigned int degree)
		:
		triangulation (Triangulation<dim>::
			       limit_level_difference_at_vertices),
		fe (degree),
		mg_dof_handler (triangulation),
		degree(degree)
{}



                                 // @sect4{LaplaceProblem::setup_system}

				 // The following function extends what the
				 // corresponding one in step-6 did. The top
				 // part, apart from the additional output,
				 // does the same:
template <int dim>
void LaplaceProblem<dim>::setup_system ()
{
  mg_dof_handler.distribute_dofs (fe);

				   // Here we output not only the
				   // degrees of freedom on the finest
				   // level, but also in the
				   // multilevel structure
  deallog << "Number of degrees of freedom: "
	  << mg_dof_handler.n_dofs();

  for (unsigned int l=0;l<triangulation.n_levels();++l)
    deallog << "   " << 'L' << l << ": "
	    << mg_dof_handler.n_dofs(l);
  deallog  << std::endl;

  sparsity_pattern.reinit (mg_dof_handler.n_dofs(),
			   mg_dof_handler.n_dofs(),
			   mg_dof_handler.max_couplings_between_dofs());
  DoFTools::make_sparsity_pattern (
    static_cast<const DoFHandler<dim>&>(mg_dof_handler),
    sparsity_pattern);

  solution.reinit (mg_dof_handler.n_dofs());
  system_rhs.reinit (mg_dof_handler.n_dofs());

				   // But it starts to be a wee bit different
				   // here, although this still doesn't have
				   // anything to do with multigrid
				   // methods. step-6 took care of boundary
				   // values and hanging nodes in a separate
				   // step after assembling the global matrix
				   // from local contributions. This works,
				   // but the same can be done in a slightly
				   // simpler way if we already take care of
				   // these constraints at the time of copying
				   // local contributions into the global
				   // matrix. To this end, we here do not just
				   // compute the constraints do to hanging
				   // nodes, but also due to zero boundary
				   // conditions. Both kinds of constraints
				   // can be put into the same object
				   // (<code>constraints</code>), and we will
				   // use this set of constraints later on to
				   // help us copy local contributions
				   // correctly into the global linear system
				   // right away, without the need for a later
				   // clean-up stage:
  constraints.clear ();
  hanging_node_constraints.clear ();
  DoFTools::make_hanging_node_constraints (mg_dof_handler, constraints);
  DoFTools::make_hanging_node_constraints (mg_dof_handler, hanging_node_constraints);
  typename FunctionMap<dim>::type      dirichlet_boundary;
  ZeroFunction<dim>                    homogeneous_dirichlet_bc (1);
  dirichlet_boundary[0] = &homogeneous_dirichlet_bc;
  MappingQ1<dim> mapping;
  VectorTools::interpolate_boundary_values (mapping,
                                            mg_dof_handler,
					    dirichlet_boundary,
					    constraints);
  constraints.close ();
  hanging_node_constraints.close ();
  constraints.condense (sparsity_pattern);
  sparsity_pattern.compress();
  system_matrix.reinit (sparsity_pattern);

  mg_constrained_dofs.clear();
  mg_constrained_dofs.initialize(mg_dof_handler, dirichlet_boundary);
				   // Now for the things that concern the
				   // multigrid data structures. First, we
				   // resize the multi-level objects to hold
				   // matrices and sparsity patterns for every
				   // level. The coarse level is zero (this is
				   // mandatory right now but may change in a
				   // future revision). Note that these
				   // functions take a complete, inclusive
				   // range here (not a starting index and
				   // size), so the finest level is
				   // <code>n_levels-1</code>.  We first have
				   // to resize the container holding the
				   // SparseMatrix classes, since they have to
				   // release their SparsityPattern before the
				   // can be destroyed upon resizing.
  const unsigned int n_levels = triangulation.n_levels();

  mg_interface_matrices.resize(0, n_levels-1);
  mg_interface_matrices.clear ();
  mg_matrices.resize(0, n_levels-1);
  mg_matrices.clear ();
  mg_sparsity_patterns.resize(0, n_levels-1);

				   // Now, we have to provide a matrix on each
				   // level. To this end, we first use the
				   // MGTools::make_sparsity_pattern function
				   // to first generate a preliminary
				   // compressed sparsity pattern on each
				   // level (see the @ref Sparsity module for
				   // more information on this topic) and then
				   // copy it over to the one we really
				   // want. The next step is to initialize
				   // both kinds of level matrices with these
				   // sparsity patterns.
				   //
				   // It may be worth pointing out that the
				   // interface matrices only have entries for
				   // degrees of freedom that sit at or next
				   // to the interface between coarser and
				   // finer levels of the mesh. They are
				   // therefore even sparser than the matrices
				   // on the individual levels of our
				   // multigrid hierarchy. If we were more
				   // concerned about memory usage (and
				   // possibly the speed with which we can
				   // multiply with these matrices), we should
				   // use separate and different sparsity
				   // patterns for these two kinds of
				   // matrices.
  for (unsigned int level=0; level<n_levels; ++level)
    {
      CompressedSparsityPattern csp;
      csp.reinit(mg_dof_handler.n_dofs(level),
		 mg_dof_handler.n_dofs(level));
      MGTools::make_sparsity_pattern(mg_dof_handler, csp, level);

      mg_sparsity_patterns[level].copy_from (csp);

      mg_matrices[level].reinit(mg_sparsity_patterns[level]);
      mg_interface_matrices[level].reinit(mg_sparsity_patterns[level]);
    }
}


                                 // @sect4{LaplaceProblem::assemble_system}

				 // The following function assembles the
				 // linear system on the finesh level of the
				 // mesh. It is almost exactly the same as in
				 // step-6, with the exception that we don't
				 // eliminate hanging nodes and boundary
				 // values after assembling, but while copying
				 // local contributions into the global
				 // matrix. This is not only simpler but also
				 // more efficient for large problems.
template <int dim>
void LaplaceProblem<dim>::assemble_system ()
{
  const QGauss<dim>  quadrature_formula(degree+1);

  FEValues<dim> fe_values (fe, quadrature_formula,
			   update_values    |  update_gradients |
			   update_quadrature_points  |  update_JxW_values);

  const unsigned int   dofs_per_cell = fe.dofs_per_cell;
  const unsigned int   n_q_points    = quadrature_formula.size();

  FullMatrix<double>   cell_matrix (dofs_per_cell, dofs_per_cell);
  Vector<double>       cell_rhs (dofs_per_cell);

  std::vector<types::global_dof_index> local_dof_indices (dofs_per_cell);

  const Coefficient<dim> coefficient;
  std::vector<double>    coefficient_values (n_q_points);

  typename MGDoFHandler<dim>::active_cell_iterator
    cell = mg_dof_handler.begin_active(),
    endc = mg_dof_handler.end();
  for (; cell!=endc; ++cell)
    {
      cell_matrix = 0;
      cell_rhs = 0;

      fe_values.reinit (cell);

      coefficient.value_list (fe_values.get_quadrature_points(),
			      coefficient_values);

      for (unsigned int q_point=0; q_point<n_q_points; ++q_point)
	for (unsigned int i=0; i<dofs_per_cell; ++i)
	  {
	    for (unsigned int j=0; j<dofs_per_cell; ++j)
	      cell_matrix(i,j) += (coefficient_values[q_point] *
				   fe_values.shape_grad(i,q_point) *
				   fe_values.shape_grad(j,q_point) *
				   fe_values.JxW(q_point));

	    cell_rhs(i) += (fe_values.shape_value(i,q_point) *
			    1.0 *
			    fe_values.JxW(q_point));
	  }

      cell->get_dof_indices (local_dof_indices);
      constraints.distribute_local_to_global (cell_matrix, cell_rhs,
					      local_dof_indices,
					      system_matrix, system_rhs);
    }
}


                                 // @sect4{LaplaceProblem::assemble_multigrid}

				 // The next function is the one that builds
				 // the linear operators (matrices) that
				 // define the multigrid method on each level
				 // of the mesh. The integration core is the
				 // same as above, but the loop below will go
				 // over all existing cells instead of just
				 // the active ones, and the results must be
				 // entered into the correct matrix. Note also
				 // that since we only do multi-level
				 // preconditioning, no right-hand side needs
				 // to be assembled here.
				 //
				 // Before we go there, however, we have to
				 // take care of a significant amount of book
				 // keeping:
template <int dim>
void LaplaceProblem<dim>::assemble_multigrid ()
{
  QGauss<dim>  quadrature_formula(1+degree);

  FEValues<dim> fe_values (fe, quadrature_formula,
			   update_values   | update_gradients |
			   update_quadrature_points | update_JxW_values);

  const unsigned int   dofs_per_cell   = fe.dofs_per_cell;
  const unsigned int   n_q_points      = quadrature_formula.size();

  FullMatrix<double>   cell_matrix (dofs_per_cell, dofs_per_cell);

  std::vector<types::global_dof_index> local_dof_indices (dofs_per_cell);

  const Coefficient<dim> coefficient;
  std::vector<double>    coefficient_values (n_q_points);

				   // Next a few things that are specific to
				   // building the multigrid data structures
				   // (since we only need them in the current
				   // function, rather than also elsewhere, we
				   // build them here instead of the
				   // <code>setup_system</code>
				   // function). Some of the following may be
				   // a bit obscure if you're not familiar
				   // with the algorithm actually implemented
				   // in deal.II to support multilevel
				   // algorithms on adaptive meshes; if some
				   // of the things below seem strange, take a
				   // look at the @ref mg_paper.
				   //
				   // Our first job is to identify those
				   // degrees of freedom on each level that
				   // are located on interfaces between
				   // adaptively refined levels, and those
				   // that lie on the interface but also on
				   // the exterior boundary of the domain. As
				   // in many other parts of the library, we
				   // do this by using boolean masks,
				   // i.e. vectors of booleans each element of
				   // which indicates whether the
				   // corresponding degree of freedom index is
				   // an interface DoF or not:
  std::vector<std::vector<bool> > interface_dofs 
    = mg_constrained_dofs.get_refinement_edge_indices ();
  std::vector<std::vector<bool> > boundary_interface_dofs
    = mg_constrained_dofs.get_refinement_edge_boundary_indices ();


				   // The indices just identified will later
				   // be used to impose zero boundary
				   // conditions for the operator that we will
				   // apply on each level. On the other hand,
				   // we also have to impose zero boundary
				   // conditions on the external boundary of
				   // each level. So let's identify these
				   // nodes as well (this time as a set of
				   // degrees of freedom, rather than a
				   // boolean mask; the reason for this being
				   // that we will not need fast tests whether
				   // a certain degree of freedom is in the
				   // boundary list, though we will need such
				   // access for the interface degrees of
				   // freedom further down below):

				   // The third step is to construct
				   // constraints on all those degrees of
				   // freedom: their value should be zero
				   // after each application of the level
				   // operators. To this end, we construct
				   // ConstraintMatrix objects for each level,
				   // and add to each of these constraints for
				   // each degree of freedom. Due to the way
				   // the ConstraintMatrix stores its data,
				   // the function to add a constraint on a
				   // single degree of freedom and force it to
				   // be zero is called
				   // Constraintmatrix::add_line(); doing so
				   // for several degrees of freedom at once
				   // can be done using
				   // Constraintmatrix::add_lines():
  std::vector<ConstraintMatrix> boundary_constraints (triangulation.n_levels());
  std::vector<ConstraintMatrix> boundary_interface_constraints (triangulation.n_levels());
  for (unsigned int level=0; level<triangulation.n_levels(); ++level)
    {
      boundary_constraints[level].add_lines (interface_dofs[level]);
      boundary_constraints[level].add_lines (mg_constrained_dofs.get_boundary_indices()[level]);
      boundary_constraints[level].close ();

      boundary_interface_constraints[level]
	.add_lines (boundary_interface_dofs[level]);
      boundary_interface_constraints[level].close ();
    }

				   // Now that we're done with most of our
				   // preliminaries, let's start the
				   // integration loop. It looks mostly like
				   // the loop in
				   // <code>assemble_system</code>, with two
				   // exceptions: (i) we don't need a right
				   // han side, and more significantly (ii) we
				   // don't just loop over all active cells,
				   // but in fact all cells, active or
				   // not. Consequently, the correct iterator
				   // to use is MGDoFHandler::cell_iterator
				   // rather than
				   // MGDoFHandler::active_cell_iterator. Let's
				   // go about it:
  typename MGDoFHandler<dim>::cell_iterator cell = mg_dof_handler.begin(),
					    endc = mg_dof_handler.end();

  for (; cell!=endc; ++cell)
    {
      cell_matrix = 0;
      fe_values.reinit (cell);

      coefficient.value_list (fe_values.get_quadrature_points(),
			      coefficient_values);

      for (unsigned int q_point=0; q_point<n_q_points; ++q_point)
	for (unsigned int i=0; i<dofs_per_cell; ++i)
	  for (unsigned int j=0; j<dofs_per_cell; ++j)
	    cell_matrix(i,j) += (coefficient_values[q_point] *
				 fe_values.shape_grad(i,q_point) *
				 fe_values.shape_grad(j,q_point) *
				 fe_values.JxW(q_point));

				       // The rest of the assembly is again
				       // slightly different. This starts with
				       // a gotcha that is easily forgotten:
				       // The indices of global degrees of
				       // freedom we want here are the ones
				       // for current level, not for the
				       // global matrix. We therefore need the
				       // function
				       // MGDoFAccessorLLget_mg_dof_indices,
				       // not MGDoFAccessor::get_dof_indices
				       // as used in the assembly of the
				       // global system:
      cell->get_mg_dof_indices (local_dof_indices);

				       // Next, we need to copy local
				       // contributions into the level
				       // objects. We can do this in the same
				       // way as in the global assembly, using
				       // a constraint object that takes care
				       // of constrained degrees (which here
				       // are only boundary nodes, as the
				       // individual levels have no hanging
				       // node constraints). Note that the
				       // <code>boundary_constraints</code>
				       // object makes sure that the level
				       // matrices contains no contributions
				       // from degrees of freedom at the
				       // interface between cells of different
				       // refinement level.
      boundary_constraints[cell->level()]
	.distribute_local_to_global (cell_matrix,
				     local_dof_indices,
				     mg_matrices[cell->level()]);

				       // The next step is again slightly more
				       // obscure (but explained in the @ref
				       // mg_paper): We need the remainder of
				       // the operator that we just copied
				       // into the <code>mg_matrices</code>
				       // object, namely the part on the
				       // interface between cells at the
				       // current level and cells one level
				       // coarser. This matrix exists in two
				       // directions: for interior DoFs (index
				       // $i$) of the current level to those
				       // sitting on the interface (index
				       // $j$), and the other way around. Of
				       // course, since we have a symmetric
				       // operator, one of these matrices is
				       // the transpose of the other.
				       //
				       // The way we assemble these matrices
				       // is as follows: since the are formed
				       // from parts of the local
				       // contributions, we first delete all
				       // those parts of the local
				       // contributions that we are not
				       // interested in, namely all those
				       // elements of the local matrix for
				       // which not $i$ is an interface DoF
				       // and $j$ is not. The result is one of
				       // the two matrices that we are
				       // interested in, and we then copy it
				       // into the
				       // <code>mg_interface_matrices</code>
				       // object. The
				       // <code>boundary_interface_constraints</code>
				       // object at the same time makes sure
				       // that we delete contributions from
				       // all degrees of freedom that are not
				       // only on the interface but also on
				       // the external boundary of the domain.
				       //
				       // The last part to remember is how to
				       // get the other matrix. Since it is
				       // only the transpose, we will later
				       // (in the <code>solve()</code>
				       // function) be able to just pass the
				       // transpose matrix where necessary.
      for (unsigned int i=0; i<dofs_per_cell; ++i)
	for (unsigned int j=0; j<dofs_per_cell; ++j)
	  if( !(interface_dofs[cell->level()][local_dof_indices[i]]==true &&
		interface_dofs[cell->level()][local_dof_indices[j]]==false))
	    cell_matrix(i,j) = 0;

      boundary_interface_constraints[cell->level()]
	.distribute_local_to_global (cell_matrix,
				     local_dof_indices,
				     mg_interface_matrices[cell->level()]);
    }
}



                                 // @sect4{LaplaceProblem::solve}

				 // This is the other function that is
				 // significantly different in support of the
				 // multigrid solver (or, in fact, the
				 // preconditioner for which we use the
				 // multigrid method).
				 //
				 // Let us start out by setting up two of the
				 // components of multilevel methods: transfer
				 // operators between levels, and a solver on
				 // the coarsest level. In finite element
				 // methods, the transfer operators are
				 // derived from the finite element function
				 // spaces involved and can often be computed
				 // in a generic way independent of the
				 // problem under consideration. In that case,
				 // we can use the MGTransferPrebuilt class
				 // that, given the constraints on the global
				 // level and an MGDoFHandler object computes
				 // the matrices corresponding to these
				 // transfer operators.
				 //
				 // The second part of the following lines
				 // deals with the coarse grid solver. Since
				 // our coarse grid is very coarse indeed, we
				 // decide for a direct solver (a Householder
				 // decomposition of the coarsest level
				 // matrix), even if its implementation is not
				 // particularly sophisticated. If our coarse
				 // mesh had many more cells than the five we
				 // have here, something better suited would
				 // obviously be necessary here.
template <int dim>
void LaplaceProblem<dim>::solve ()
{
  MGTransferPrebuilt<Vector<double> > mg_transfer(hanging_node_constraints, mg_constrained_dofs);
  mg_transfer.build_matrices(mg_dof_handler);

  FullMatrix<double> coarse_matrix;
  coarse_matrix.copy_from (mg_matrices[0]);
  MGCoarseGridHouseholder<> coarse_grid_solver;
  coarse_grid_solver.initialize (coarse_matrix);
  
  typedef PreconditionSOR<SparseMatrix<double> > Smoother;
  GrowingVectorMemory<>   vector_memory;
  MGSmootherRelaxation<SparseMatrix<double>, Smoother, Vector<double> >
    mg_smoother;
  mg_smoother.initialize(mg_matrices);
  mg_smoother.set_steps(2);
  mg_smoother.set_symmetric(true);

  MGMatrix<> mg_matrix(&mg_matrices);
  MGMatrix<> mg_interface_up(&mg_interface_matrices);
  MGMatrix<> mg_interface_down(&mg_interface_matrices);

  Multigrid<Vector<double> > mg(mg_dof_handler,
				mg_matrix,
				coarse_grid_solver,
				mg_transfer,
				mg_smoother,
				mg_smoother);
  mg.set_edge_matrices(mg_interface_down, mg_interface_up);

  PreconditionMG<dim, Vector<double>, MGTransferPrebuilt<Vector<double> > >
  preconditioner(mg_dof_handler, mg, mg_transfer);

  SolverControl solver_control (1000, 1e-12);
  SolverCG<>    cg (solver_control);

  solution = 0;

  cg.solve (system_matrix, solution, system_rhs,
	    preconditioner);
  constraints.distribute (solution);

  deallog << "   " << solver_control.last_step()
	    << " CG iterations needed to obtain convergence."
	    << std::endl;
}



                                 // @sect4{Postprocessing}

				 // The following two functions postprocess a
				 // solution once it is computed. In
				 // particular, the first one refines the mesh
				 // at the beginning of each cycle while the
				 // second one outputs results at the end of
				 // each such cycle. The functions are almost
				 // unchanged from those in step-6, with the
				 // exception of two minor differences: The
				 // KellyErrorEstimator::estimate function
				 // wants an argument of type DoFHandler, not
				 // MGDoFHandler, and so we have to cast from
				 // derived to base class; and we generate
				 // output in VTK format, to use the more
				 // modern visualization programs available
				 // today compared to those that were
				 // available when step-6 was written.
template <int dim>
void LaplaceProblem<dim>::refine_grid ()
{
  Vector<float> estimated_error_per_cell (triangulation.n_active_cells());

  KellyErrorEstimator<dim>::estimate (static_cast<DoFHandler<dim>&>(mg_dof_handler),
				      QGauss<dim-1>(3),
				      typename FunctionMap<dim>::type(),
				      solution,
				      estimated_error_per_cell);
  GridRefinement::refine_and_coarsen_fixed_number (triangulation,
						   estimated_error_per_cell,
						   0.3, 0.03);
  triangulation.execute_coarsening_and_refinement ();
}



template <int dim>
void LaplaceProblem<dim>::output_results (const unsigned int cycle) const
{
  DataOut<dim> data_out;

  data_out.attach_dof_handler (mg_dof_handler);
  data_out.add_data_vector (solution, "solution");
  data_out.build_patches ();

  std::ostringstream filename;
  filename << "solution-"
	   << cycle
	   << ".vtk";

//  std::ofstream output (filename.str().c_str());
//  data_out.write_vtk (output);
}


                                 // @sect4{LaplaceProblem::run}

				 // Like several of the functions above, this
				 // is almost exactly a copy of of the
				 // corresponding function in step-6. The only
				 // difference is the call to
				 // <code>assemble_multigrid</code> that takes
				 // care of forming the matrices on every
				 // level that we need in the multigrid
				 // method.
template <int dim>
void LaplaceProblem<dim>::run ()
{
  for (unsigned int cycle=0; cycle<8; ++cycle)
    {
      deallog << "Cycle " << cycle << ':' << std::endl;

      if (cycle == 0)
	{
	  GridGenerator::hyper_ball (triangulation);

	  static const HyperBallBoundary<dim> boundary;
	  triangulation.set_boundary (0, boundary);

	  triangulation.refine_global (1);
	}
      else
	refine_grid ();


      deallog << "   Number of active cells:       "
		<< triangulation.n_active_cells()
		<< std::endl;

      setup_system ();

      deallog << "   Number of degrees of freedom: "
		<< mg_dof_handler.n_dofs()
		<< " (by level: ";
      for (unsigned int level=0; level<triangulation.n_levels(); ++level)
	deallog << mg_dof_handler.n_dofs(level)
		  << (level == triangulation.n_levels()-1
		      ? ")" : ", ");
      deallog << std::endl;

      assemble_system ();
      assemble_multigrid ();

      solve ();
//      output_results (cycle);
    }
}


				 // @sect3{The main() function}
				 //
				 // This is again the same function as
				 // in step-6:
int main ()
{
  std::ofstream logfile("step-16/output");
  deallog << std::setprecision(4);
  deallog.attach(logfile);
  deallog.depth_console(0);
  deallog.threshold_double(1.e-10);

  try
    {
      deallog.depth_console (0);

      LaplaceProblem<2> laplace_problem(1);
      laplace_problem.run ();
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      std::cerr << "Exception on processing: " << std::endl
		<< exc.what() << std::endl
		<< "Aborting!" << std::endl
		<< "----------------------------------------------------"
		<< std::endl;

      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      std::cerr << "Unknown exception!" << std::endl
		<< "Aborting!" << std::endl
		<< "----------------------------------------------------"
		<< std::endl;
      return 1;
    }

  return 0;
}
