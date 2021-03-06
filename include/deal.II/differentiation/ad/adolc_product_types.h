// ---------------------------------------------------------------------
//
// Copyright (C) 2016 - 2017 by the deal.II authors
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

#ifndef dealii_differentiation_ad_adolc_product_types_h
#define dealii_differentiation_ad_adolc_product_types_h

#include <deal.II/base/config.h>

#ifdef DEAL_II_WITH_ADOLC

#include <deal.II/base/template_constraints.h>

#include <adolc/adouble.h> // Taped double
#include <adolc/adtl.h>    // Tapeless double

DEAL_II_NAMESPACE_OPEN


/* -------------- Adol-C taped (Differentiation::AD::NumberTypes::adolc_taped) -------------- */


namespace internal
{

  template <>
  struct ProductTypeImpl<adouble,adouble>
  {
    typedef adouble type;
  };

  // Typedefs for "adub"s are all thats necessary to ensure that no temporary Adol-C types
  // "adub" are created when a scalar product is performed.
  // If this is not done, then intermediate tensors are filled with unconstructable types.
  template <>
  struct ProductTypeImpl<adub,adouble>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<adouble,adub>
  {
    typedef adouble type;
  };

  /* --- Double --- */

  template <>
  struct ProductTypeImpl<double,adouble>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<adouble,double>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<double,adub>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<adub,double>
  {
    typedef adouble type;
  };

  /* --- Float --- */

  template <>
  struct ProductTypeImpl<float,adouble>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<adouble,float>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<float,adub>
  {
    typedef adouble type;
  };

  template <>
  struct ProductTypeImpl<adub,float>
  {
    typedef adouble type;
  };

  /* --- Complex double --- */

  template <>
  struct ProductTypeImpl<std::complex<double> ,std::complex<adouble> >
  {
    typedef std::complex<adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adouble>, std::complex<double> >
  {
    typedef std::complex<adouble> type;
  };

  template <>
  struct ProductTypeImpl< std::complex<adouble>, std::complex<adouble> >
  {
    typedef std::complex<adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adub> ,std::complex<adouble> >
  {
    typedef std::complex<adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adouble>, std::complex<adub> >
  {
    typedef std::complex<adouble> type;
  };

  /* --- Complex float --- */

  template <>
  struct ProductTypeImpl<std::complex<float> ,std::complex<adouble> >
  {
    typedef std::complex<adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adouble>, std::complex<float> >
  {
    typedef std::complex<adouble> type;
  };

}

template <>
struct EnableIfScalar<adouble>
{
  typedef adouble type;
};

template <>
struct EnableIfScalar<std::complex<adouble> >
{
  typedef std::complex<adouble> type;
};


template <>
struct EnableIfScalar<adub>
{
  typedef adouble type;
};


template <>
struct EnableIfScalar<std::complex<adub> >
{
  typedef std::complex<adouble> type;
};


/* -------------- Adol-C tapeless (Differentiation::AD::NumberTypes::adolc_tapeless) -------------- */


namespace internal
{

  /* --- Double --- */

  template <>
  struct ProductTypeImpl<double,adtl::adouble>
  {
    typedef adtl::adouble type;
  };

  template <>
  struct ProductTypeImpl<adtl::adouble,double>
  {
    typedef adtl::adouble type;
  };

  template <>
  struct ProductTypeImpl<adtl::adouble,adtl::adouble>
  {
    typedef adtl::adouble type;
  };

  /* --- Float --- */

  template <>
  struct ProductTypeImpl<float,adtl::adouble>
  {
    typedef adtl::adouble type;
  };

  template <>
  struct ProductTypeImpl<adtl::adouble,float>
  {
    typedef adtl::adouble type;
  };

  /* --- Complex double --- */

  template <>
  struct ProductTypeImpl<std::complex<double>,std::complex<adtl::adouble> >
  {
    typedef std::complex<adtl::adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adtl::adouble>,std::complex<double> >
  {
    typedef std::complex<adtl::adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adtl::adouble>,std::complex<adtl::adouble> >
  {
    typedef std::complex<adtl::adouble> type;
  };

  /* --- Complex float --- */

  template <>
  struct ProductTypeImpl<std::complex<float>,std::complex<adtl::adouble> >
  {
    typedef std::complex<adtl::adouble> type;
  };

  template <>
  struct ProductTypeImpl<std::complex<adtl::adouble>,std::complex<float> >
  {
    typedef std::complex<adtl::adouble> type;
  };

}


template <>
struct EnableIfScalar<adtl::adouble>
{
  typedef adtl::adouble type;
};


template <>
struct EnableIfScalar<std::complex<adtl::adouble> >
{
  typedef std::complex<adtl::adouble> type;
};


DEAL_II_NAMESPACE_CLOSE

#endif // DEAL_II_WITH_ADOLC

#endif
