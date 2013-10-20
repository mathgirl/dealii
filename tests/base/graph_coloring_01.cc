// ---------------------------------------------------------------------
// $Id: graph_coloring_01.cc 30045 2013-07-18 19:18:00Z maier $
//
// Copyright (C) 2003 - 2013 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------



// Check that graph coloring works on uniform mesh with a uniform polynomial
// order.


#include "../tests.h"
#include <fstream>
#include <vector>

#include <deal.II/base/graph_coloring.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/grid/tria.h>

template <int dim>
std::vector<types::global_dof_index> get_conflict_indices_cfem(
    typename DoFHandler<dim>::active_cell_iterator const &it)
{
  std::vector<types::global_dof_index> local_dof_indices(it->get_fe().dofs_per_cell);
  it->get_dof_indices(local_dof_indices);

  return local_dof_indices;
}

template <int dim>
void check()
{
  // Create the Triangulation and the DoFHandler
  Triangulation<dim> triangulation;
  GridGenerator::hyper_cube(triangulation);
  triangulation.refine_global(2);
  FE_Q<dim> fe(2);
  DoFHandler<dim> dof_handler(triangulation);
  dof_handler.distribute_dofs(fe);

  // Create the coloring
  typename DoFHandler<dim>::active_cell_iterator cell(dof_handler.begin_active());
  std::vector<std::vector<typename DoFHandler<dim>::active_cell_iterator> > coloring(
      graph_coloring::make_graph_coloring(cell,dof_handler.end(),
      std_cxx1x::function<std::vector<types::global_dof_index> (typename 
        DoFHandler<dim>::active_cell_iterator const &)> (&get_conflict_indices_cfem<dim>)));      

  // Output the coloring
  for (unsigned int color=0; color<coloring.size(); ++color)
  {
    deallog<<"Color: "<<color<<std::endl;
    for (unsigned int i=0; i<coloring[color].size(); ++i)
      for (unsigned int j=0; j<dim; ++j)
        deallog<<coloring[color][i]->center()[j]<<" ";
    deallog<<std::endl;
  }
}

int main()
{
  std::ofstream logfile("output");
  deallog<<std::setprecision(4);
  deallog<<std::fixed;
  deallog.attach(logfile);
  deallog.depth_console(0);

  check<2> ();

  return 0;
}