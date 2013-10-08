// This file is part of HermesCommon
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://hpfem.org/.
//
// Hermes2D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
/*! \file newton_solver_nox.h
\brief NOX (nonliner) solver interface.
*/
#ifndef __H2D_NEWTON_SOLVER_NOX_H_
#define __H2D_NEWTON_SOLVER_NOX_H_

#include "linear_matrix_solver.h"
#include "nonlinear_solver.h"
#include "epetra.h"
#if(defined HAVE_NOX && defined HAVE_EPETRA && defined HAVE_TEUCHOS)
#include <NOX.H>
#ifdef _POSIX_C_SOURCE
# undef _POSIX_C_SOURCE  // pyconfig.h included by NOX_Epetra defines it
#endif
#ifdef _XOPEN_SOURCE
# undef _XOPEN_SOURCE  // pyconfig.h included by NOX_Epetra defines it
#endif
#undef interface
#include <NOX_Epetra.H>
#include "exceptions.h"
#include "discrete_problem.h"

namespace Hermes
{
  namespace Hermes2D
  {
    /// \brief discrete problem used in NOX solver
    /// Implents interfaces needed by NOX Epetra
    template <typename Scalar>
    class HERMES_API DiscreteProblemNOX :
      public Hermes::Hermes2D::DiscreteProblem<Scalar>,
      public NOX::Epetra::Interface::Required,
      public NOX::Epetra::Interface::Jacobian,
      public NOX::Epetra::Interface::Preconditioner
    {
    public:
      /// Constructor for multiple components / equations.
      DiscreteProblemNOX(WeakForm<Scalar>* wf, Hermes::vector<SpaceSharedPtr<Scalar> >& spaces);
      /// Constructor for one equation.
      DiscreteProblemNOX(WeakForm<Scalar>* wf, SpaceSharedPtr<Scalar>& space);
      /// Non-parameterized constructor.
      DiscreteProblemNOX();

      /// \brief Setter for preconditioner.
      void set_precond(Teuchos::RCP<Hermes::Preconditioners::EpetraPrecond<Scalar> > &pc);

      /// \brief Accessor for preconditioner.
      Teuchos::RCP<Hermes::Preconditioners::EpetraPrecond<Scalar> > get_precond();

      // that is generated by the Interface class.
      EpetraMatrix<Scalar> *get_jacobian();

      /// \brief Compute and return F.
      virtual bool computeF(const Epetra_Vector &x, Epetra_Vector &f, FillType flag = Residual);

      /// \brief Compute an explicit Jacobian.
      virtual bool computeJacobian(const Epetra_Vector &x, Epetra_Operator &op);

      /// \brief Computes a user supplied preconditioner based on input vector x.
      /// @return true if computation was successful.
      virtual bool computePreconditioner(const Epetra_Vector &x, Epetra_Operator &m,
        Teuchos::ParameterList *precParams = 0);

    private:
      /// \brief Jacobian (optional).
      EpetraMatrix<Scalar> jacobian;
      /// \brief Preconditioner (optional).
      Teuchos::RCP<Hermes::Preconditioners::EpetraPrecond<Scalar> > precond;
    };

    /// \brief Encapsulation of NOX nonlinear solver.
    /// \note complex numbers is not implemented yet
    /// @ingroup solvers
    template <typename Scalar>
    class HERMES_API NewtonSolverNOX : public Hermes::Mixins::Loggable
    {
    private:
      Teuchos::RCP<Teuchos::ParameterList> nl_pars;
    public:
      /// Constructor.
      NewtonSolverNOX(DiscreteProblemNOX<Scalar> *problem);

      virtual ~NewtonSolverNOX();

      /// set time information for time-dependent problems.
      virtual void set_time(double time);
      virtual void set_time_step(double time_step);

      virtual void solve(Scalar* coeff_vec);

      Scalar* get_sln_vector();

      virtual int get_num_iters();
      virtual double get_residual();
      int get_num_lin_iters();
      double get_achieved_tol();

      /// setting of output flags.
      /// \param flags[in] output flags, sum of NOX::Utils::MsgType enum
      /// \par
      ///  %Error = 0, Warning = 0x1, OuterIteration = 0x2, InnerIteration = 0x4,
      ///  Parameters = 0x8, Details = 0x10, OuterIterationStatusTest = 0x20, LinearSolverDetails = 0x40,
      ///  TestDetails = 0x80, StepperIteration = 0x0100, StepperDetails = 0x0200, StepperParameters = 0x0400,
      ///  Debug = 0x01000
      void set_output_flags(int flags);

      /// \name linear solver setters
      ///@{
      ///Determine the iterative technique used in the solve. The following options are valid:
      /// - "GMRES" - Restarted generalized minimal residual (default).
      /// - "CG" - Conjugate gradient.
      /// - "CGS" - Conjugate gradient squared.
      /// - "TFQMR" - Transpose-free quasi-minimal reasidual.
      /// - "BiCGStab" - Bi-conjugate gradient with stabilization.
      /// - "LU" - Sparse direct solve (single processor only).
      void set_ls_type(const char *type);
      /// maximum number of iterations in the linear solve.
      void set_ls_max_iters(int iters);
      /// Tolerance used by AztecOO to determine if an iterative linear solve has converged.
      void set_ls_tolerance(double tolerance);
      /// When using restarted GMRES this sets the maximum size of the Krylov subspace.
      void set_ls_sizeof_krylov_subspace(int size);
      ///@}

      /// \name convergence params
      /// @{
      /// Type of norm
      /// - NOX::Abstract::Vector::OneNorm \f[ \|x\| = \sum_{i = 1}^n \| x_i \| \f]
      /// - NOX::Abstract::Vector::TwoNorm \f[ \|x\| = \sqrt{\sum_{i = 1}^n \| x_i^2 \|} \f]
      /// - NOX::Abstract::Vector::MaxNorm \f[ \|x\| = \max_i \| x_i \| \f]
      void set_norm_type(NOX::Abstract::Vector::NormType type);
      /// Determines whether to scale the norm by the problem size.
      /// - Scaled - scale
      /// - Unscaled - don't scale
      void set_scale_type(NOX::StatusTest::NormF::ScaleType type);
      /// Maximum number of nonlinear solver iterations.
      void set_conv_iters(int iters);
      /// Absolute tolerance.
      void set_conv_abs_resid(double resid);
      /// Relatice tolerance (scaled by initial guess).
      void set_conv_rel_resid(double resid);
      /// disable absolute tolerance \todo is this needed?
      void disable_abs_resid();
      /// disable relative tolerance \todo is this needed?
      void disable_rel_resid();
      /// Update (change of solution) tolerance.
      void set_conv_update(double update);
      /// Convergence test based on the weighted root mean square norm of the solution update between iterations.
      /// \param[in] rtol denotes the relative error tolerance, specified via rtol in the constructor
      /// \param[in] atol denotes the absolution error tolerance, specified via atol in the constructor.
      void set_conv_wrms(double rtol, double atol);
      ///@}

      ///Preconditioner Reuse Policy. Sets how and when the preconditioner should be computed.
      ///This flag supports native Aztec, Ifpack and ML preconditioners.
      /// \param[in] pc_reuse
      /// - "Rebuild" - The "Rebuild" option always completely destroys and then rebuilds the preconditioner each time a linear solve is requested.
      /// - "Reuse" - The group/linear solver will not recompute the preconditioner even if the group's solution vector changes.
      /// It just blindly reuses what has been constructed. This turns off control of preconditioner recalculation.
      /// This is a dangerous condition but can really speed up the computations if the user knows what they are doing. We don't recommend users trying this.
      /// - "Recompute" - Recomputes the preconditioner, but will try to efficiently reuse any objects that don't need to be destroyed.
      /// How efficient the "Recompute" option is depends on the type of preconditioner.
      /// For example if we are using ILU from the Ifpack library, we would like to not destroy and reallocate the graph each solve.
      /// With this option, we tell Ifpack to reuse the graph from last time -
      /// e.g the sparisty pattern has not changed between applications of the preconditioner. (default)
      void set_precond_reuse(const char * pc_reuse);
      /// Max Age Of Preconditioner
      /// \param[in] max_age If the "Preconditioner Reuse Policy" is set to "Reuse",
      /// this integer tells the linear system how many times to reuse the preconditioner before rebuilding it. (default 999)
      void set_precond_max_age(int max_age);
      /// Set user defined preconditioner
      /// \param[in] pc preconditioner
      virtual void set_precond(Hermes::Preconditioners::EpetraPrecond<Scalar> &pc);
      /// Set preconditioner
      /// \param[in] pc name of preconditioner
      /// - "None" - No preconditioning. (default)
      /// - "AztecOO" - AztecOO internal preconditioner.
      /// - "New Ifpack" - Ifpack internal preconditioner.
      /// - "ML" - Multi level preconditioner
      virtual void set_precond(const char *pc);

      Scalar* sln_vector;
      DiscreteProblemNOX<Scalar> *dp;

    protected:
      int num_iters;
      double residual;
      int num_lin_iters;
      double achieved_tol;

      // convergence params
      struct conv_t
      {
        int max_iters;
        double abs_resid;
        double rel_resid;
        NOX::Abstract::Vector::NormType norm_type;
        NOX::StatusTest::NormF::ScaleType stype;
        double update;
        double wrms_rtol;
        double wrms_atol;
      } conv;

      struct conv_flag_t
      {
        unsigned absresid:1;
        unsigned relresid:1;
        unsigned wrms:1;
        unsigned update:1;
      } conv_flag;
    };
  }
}
#endif
#endif