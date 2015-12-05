/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision: 2.10 $
 ***********************************************************************EHEADER*/




/******************************************************************************
 *
 *
 *****************************************************************************/

#include "headers.h"

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxData data structure
 *--------------------------------------------------------------------------*/

typedef struct
{
   void                   *relax_data;
   void                   *rb_relax_data;
   int                     relax_type;
   double                  jacobi_weight;

} hypre_PFMGRelaxData;

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxCreate
 *--------------------------------------------------------------------------*/

void *
hypre_PFMGRelaxCreate( MPI_Comm  comm )
{
   hypre_PFMGRelaxData *pfmg_relax_data;

   pfmg_relax_data = hypre_CTAlloc(hypre_PFMGRelaxData, 1);
   (pfmg_relax_data -> relax_data) = hypre_PointRelaxCreate(comm);
   (pfmg_relax_data -> rb_relax_data) = hypre_RedBlackGSCreate(comm);
   (pfmg_relax_data -> relax_type) = 0;        /* Weighted Jacobi */
   (pfmg_relax_data -> jacobi_weight) = 0.0;

   return (void *) pfmg_relax_data;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxDestroy
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxDestroy( void *pfmg_relax_vdata )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  ierr = 0;

   if (pfmg_relax_data)
   {
      hypre_PointRelaxDestroy(pfmg_relax_data -> relax_data);
      hypre_RedBlackGSDestroy(pfmg_relax_data -> rb_relax_data);
      hypre_TFree(pfmg_relax_data);
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelax
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelax( void               *pfmg_relax_vdata,
                 hypre_StructMatrix *A,
                 hypre_StructVector *b,
                 hypre_StructVector *x                )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int          relax_type = (pfmg_relax_data -> relax_type);
   int          constant_coefficient= hypre_StructMatrixConstantCoefficient(A);
   int          ierr = 0;

   if (constant_coefficient==1) hypre_StructVectorClearBoundGhostValues( b );
   switch(relax_type)
   {
      case 0:
      case 1:
         ierr = hypre_PointRelax((pfmg_relax_data -> relax_data), A, b, x);
         break;
      case 2:
      case 3:
         if (constant_coefficient)
         {
            ierr = hypre_RedBlackConstantCoefGS((pfmg_relax_data -> rb_relax_data), 
                                                 A, b, x);
         }
         else
         {
            ierr = hypre_RedBlackGS((pfmg_relax_data -> rb_relax_data), A, b, x);
         }
          
         break;
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetup
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetup( void               *pfmg_relax_vdata,
                      hypre_StructMatrix *A,
                      hypre_StructVector *b,
                      hypre_StructVector *x                )
{
   hypre_PFMGRelaxData *pfmg_relax_data  = pfmg_relax_vdata;
   int                  relax_type       = (pfmg_relax_data -> relax_type);
   double               jacobi_weight    = (pfmg_relax_data -> jacobi_weight); 
   int                  ierr;

   switch(relax_type)
   {
      case 0:
      case 1:
         ierr = hypre_PointRelaxSetup((pfmg_relax_data -> relax_data),
                                      A, b, x);
         break;
      case 2:
      case 3:
         ierr = hypre_RedBlackGSSetup((pfmg_relax_data -> rb_relax_data),
                                      A, b, x);
         break;
   }

   if (relax_type==1)
   {
      hypre_PointRelaxSetWeight(pfmg_relax_data -> relax_data, jacobi_weight);
   }
   
   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetType
 *--------------------------------------------------------------------------*/
/* All the weight-related code in this function should be migrated to
 * hypre_PFMGRelaxSetup */
int
hypre_PFMGRelaxSetType( void  *pfmg_relax_vdata,
                        int    relax_type       )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   void                *relax_data = (pfmg_relax_data -> relax_data);
   int                  ierr = 0;

   (pfmg_relax_data -> relax_type) = relax_type;

   switch(relax_type)
   {
      case 0: /* Jacobi */
      {
         hypre_Index  stride;
         hypre_Index  indices[1];

         hypre_PointRelaxSetWeight(relax_data, 1.0);
         hypre_PointRelaxSetNumPointsets(relax_data, 1);

         hypre_SetIndex(stride, 1, 1, 1);
         hypre_SetIndex(indices[0], 0, 0, 0);
         hypre_PointRelaxSetPointset(relax_data, 0, 1, stride, indices);
      }
      break;

      case 2: /* Red-Black Gauss-Seidel */
      case 3: /* Red-Black Gauss-Seidel (non-symmetric) */
      break;
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetJacobiWeight
 *--------------------------------------------------------------------------*/
/* Presently this should be called before hypre_PFMGRelaxSetType.
 * If we move all the weight-related code in hypre_PFMGRelaxSetType
 * to hypre_PFMGRelaxSetup, then this restriction will go away */
int
hypre_PFMGRelaxSetJacobiWeight(void  *pfmg_relax_vdata,
                               double weight) 
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;

  (pfmg_relax_data -> jacobi_weight)    = weight;

   return hypre_error_flag;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetPreRelax
 *--------------------------------------------------------------------------*/
int
hypre_PFMGRelaxSetPreRelax( void  *pfmg_relax_vdata )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  relax_type = (pfmg_relax_data -> relax_type);
   int                  ierr = 0;

   switch(relax_type)
   {
      case 1: /* Weighted Jacobi */
      case 0: /* Jacobi */
         break;

      case 2: /* Red-Black Gauss-Seidel */
         hypre_RedBlackGSSetStartRed((pfmg_relax_data -> rb_relax_data));
         break;

      case 3: /* Red-Black Gauss-Seidel (non-symmetric) */
         hypre_RedBlackGSSetStartRed((pfmg_relax_data -> rb_relax_data));
         break;
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetPostRelax
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetPostRelax( void  *pfmg_relax_vdata )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  relax_type = (pfmg_relax_data -> relax_type);
   int                  ierr = 0;

   switch(relax_type)
   {
      case 1: /* Weighted Jacobi */
      case 0: /* Jacobi */
         break;

      case 2: /* Red-Black Gauss-Seidel */
         hypre_RedBlackGSSetStartBlack((pfmg_relax_data -> rb_relax_data));
         break;

      case 3: /* Red-Black Gauss-Seidel (non-symmetric) */
         hypre_RedBlackGSSetStartRed((pfmg_relax_data -> rb_relax_data));
         break;
   }

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetTol
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetTol( void   *pfmg_relax_vdata,
                       double  tol              )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  ierr = 0;

   ierr = hypre_PointRelaxSetTol((pfmg_relax_data -> relax_data), tol);
   ierr = hypre_RedBlackGSSetTol((pfmg_relax_data -> rb_relax_data), tol);

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetMaxIter
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetMaxIter( void  *pfmg_relax_vdata,
                           int    max_iter         )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  ierr = 0;

   ierr = hypre_PointRelaxSetMaxIter((pfmg_relax_data -> relax_data),
                                     max_iter);
   ierr = hypre_RedBlackGSSetMaxIter((pfmg_relax_data -> rb_relax_data),
                                     max_iter);

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetZeroGuess
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetZeroGuess( void  *pfmg_relax_vdata,
                             int    zero_guess       )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  ierr = 0;

   ierr = hypre_PointRelaxSetZeroGuess((pfmg_relax_data -> relax_data),
                                       zero_guess);
   ierr = hypre_RedBlackGSSetZeroGuess((pfmg_relax_data -> rb_relax_data),
                                       zero_guess);

   return ierr;
}

/*--------------------------------------------------------------------------
 * hypre_PFMGRelaxSetTempVec
 *--------------------------------------------------------------------------*/

int
hypre_PFMGRelaxSetTempVec( void               *pfmg_relax_vdata,
                           hypre_StructVector *t                )
{
   hypre_PFMGRelaxData *pfmg_relax_data = pfmg_relax_vdata;
   int                  ierr = 0;

   ierr = hypre_PointRelaxSetTempVec((pfmg_relax_data -> relax_data), t);

   return ierr;
}

