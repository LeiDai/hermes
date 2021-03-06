// This file is part of Hermes2D.
//
// Hermes2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D.  If not, see <http://www.gnu.org/licenses/>.

#include "projections/ogprojection.h"
#include "space.h"
#include "solver/linear_solver.h"

namespace Hermes
{
  namespace Hermes2D
  {
    template<typename Scalar>
    void OGProjection<Scalar>::project_internal(SpaceSharedPtr<Scalar> space, WeakFormSharedPtr<Scalar> wf, Scalar* target_vec)
    {
      // Sanity check.
      if (wf == nullptr)
        throw Hermes::Exceptions::NullException(1);
      if (target_vec == nullptr)
        throw Exceptions::NullException(2);

      // Initialize linear solver.
      Hermes::Hermes2D::LinearSolver<Scalar> linear_solver(wf, space);
      linear_solver.set_verbose_output(false);

      // Perform Newton iteration.
      linear_solver.solve();

      memcpy(target_vec, linear_solver.get_sln_vector(), space->get_num_dofs() * sizeof(Scalar));
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(SpaceSharedPtr<Scalar> space,
      MatrixFormVol<Scalar>* custom_projection_jacobian,
      VectorFormVol<Scalar>* custom_projection_residual,
      Scalar* target_vec)
    {
      // Define projection weak form.
      WeakFormSharedPtr<Scalar> proj_wf(new WeakForm<Scalar>(1));
      proj_wf->add_matrix_form(custom_projection_jacobian);
      proj_wf->add_vector_form(custom_projection_residual);

      // Call the main function.
      project_internal(space, proj_wf, target_vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(SpaceSharedPtr<Scalar> space,
      MatrixFormVol<Scalar>* custom_projection_jacobian,
      VectorFormVol<Scalar>* custom_projection_residual,
      MeshFunctionSharedPtr<Scalar> target_sln)
    {
      // Calculate the coefficient vector.
      Scalar* target_vec = malloc_with_check<Scalar>(space->get_num_dofs());

      project_global(space, custom_projection_jacobian, custom_projection_residual, target_vec);

      // Translate coefficient vector into a Solution.
      Solution<Scalar>::vector_to_solution(target_vec, space, target_sln);

      // Clean up.
      free_with_check(target_vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(const std::vector<SpaceSharedPtr<Scalar> > spaces,
      const std::vector<MatrixFormVol<Scalar>*>& custom_projection_jacobians,
      const std::vector<VectorFormVol<Scalar>*>& custom_projection_residuals,
      Scalar* target_vec)
    {
      // Sanity checks.
      if (target_vec == nullptr) throw Exceptions::NullException(3);
      // Sanity checks.
      Helpers::check_length(custom_projection_jacobians, spaces);
      Helpers::check_length(custom_projection_residuals, spaces);

      int start_index = 0, spaces_size = spaces.size();
      for (int i = 0; i < spaces_size; i++)
      {
        project_global(spaces[i], custom_projection_jacobians[i], custom_projection_residuals[i], target_vec + start_index);
        start_index += spaces[i]->get_num_dofs();
      }
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(const std::vector<SpaceSharedPtr<Scalar> > spaces,
      const std::vector<MatrixFormVol<Scalar>*>& custom_projection_jacobians,
      const std::vector<VectorFormVol<Scalar>*>& custom_projection_residuals,
      const std::vector<MeshFunctionSharedPtr<Scalar> >& target_slns)
    {
      // Sanity checks.
      Helpers::check_length(target_slns, spaces);
      Helpers::check_length(custom_projection_jacobians, spaces);
      Helpers::check_length(custom_projection_residuals, spaces);

      int spaces_size = spaces.size();
      for (int i = 0; i < spaces_size; i++)
        project_global(spaces[i], custom_projection_jacobians[i], custom_projection_residuals[i], target_slns[i]);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(SpaceSharedPtr<Scalar> space,
      MeshFunctionSharedPtr<Scalar> source_meshfn, Hermes::Algebra::Vector<Scalar>* target_vec,
      NormType proj_norm)
    {
      if (target_vec->get_size() != space->get_num_dofs())
        throw Exceptions::ValueException("target_vec->size", target_vec->get_size(), space->get_num_dofs());

      Scalar* vec = malloc_with_check<Scalar>(target_vec->get_size());
      project_global(space, source_meshfn, vec, proj_norm);
      target_vec->set_vector(vec);
      free_with_check(vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(SpaceSharedPtr<Scalar> space,
      MeshFunctionSharedPtr<Scalar> source_meshfn, Scalar* target_vec,
      NormType proj_norm)
    {
      // Sanity checks.
      if (target_vec == nullptr)
        throw Exceptions::NullException(3);

      // If projection norm is not provided, set it
      // to match the type of the space.
      NormType norm = HERMES_UNSET_NORM;
      if (proj_norm == HERMES_UNSET_NORM)
      {
        SpaceType space_type = space->get_type();
        switch (space_type)
        {
        case HERMES_H1_SPACE: norm = HERMES_H1_NORM; break;
        case HERMES_HCURL_SPACE: norm = HERMES_HCURL_NORM; break;
        case HERMES_HDIV_SPACE: norm = HERMES_HDIV_NORM; break;
        case HERMES_L2_SPACE: norm = HERMES_L2_NORM; break;
        case HERMES_L2_MARKERWISE_CONST_SPACE: norm = HERMES_L2_NORM; break;
        default: throw Hermes::Exceptions::Exception("Unknown space type in OGProjection<Scalar>::project_global().");
        }
      }
      else norm = proj_norm;

      // Define temporary projection weak form.
      WeakFormSharedPtr<Scalar> proj_wf(new WeakForm<Scalar>(1));
      proj_wf->set_verbose_output(false);
      proj_wf->set_ext(source_meshfn);
      // Add Jacobian.
      proj_wf->add_matrix_form(new MatrixDefaultNormFormVol<Scalar>(0, 0, norm));
      // Add Residual.
      proj_wf->add_vector_form(new VectorDefaultNormFormVol<Scalar>(0, norm));

      // Call main function.
      project_internal(space, proj_wf, target_vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(SpaceSharedPtr<Scalar> space,
      MeshFunctionSharedPtr<Scalar> source_sln, MeshFunctionSharedPtr<Scalar> target_sln,
      NormType proj_norm)
    {
      if (proj_norm == HERMES_UNSET_NORM)
      {
        SpaceType space_type = space->get_type();
        switch (space_type)
        {
        case HERMES_H1_SPACE: proj_norm = HERMES_H1_NORM; break;
        case HERMES_HCURL_SPACE: proj_norm = HERMES_HCURL_NORM; break;
        case HERMES_HDIV_SPACE: proj_norm = HERMES_HDIV_NORM; break;
        case HERMES_L2_SPACE: proj_norm = HERMES_L2_NORM; break;
        case HERMES_L2_MARKERWISE_CONST_SPACE: proj_norm = HERMES_L2_NORM; break;
        default: throw Hermes::Exceptions::Exception("Unknown space type in OGProjection<Scalar>::project_global().");
        }
      }

      // Calculate the coefficient vector.
      Scalar* target_vec = malloc_with_check<Scalar>(space->get_num_dofs());
      project_global(space, source_sln, target_vec, proj_norm);

      // Translate coefficient vector into a Solution.
      Solution<Scalar>::vector_to_solution(target_vec, space, target_sln);

      // Clean up.
      free_with_check(target_vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(std::vector<SpaceSharedPtr<Scalar> > spaces, std::vector<MeshFunctionSharedPtr<Scalar> > source_slns,
      Scalar* target_vec, std::vector<NormType> proj_norms)
    {
      // Sanity checks.
      Helpers::check_length(source_slns, spaces);
      if (target_vec == nullptr)
        throw Exceptions::NullException(3);
      if (!proj_norms.empty())
        Helpers::check_length(proj_norms, spaces);

      int start_index = 0, spaces_size = spaces.size();
      for (int i = 0; i < spaces_size; i++)
      {
        if (proj_norms.empty())
          project_global(spaces[i], source_slns[i], target_vec + start_index, HERMES_UNSET_NORM);
        else
          project_global(spaces[i], source_slns[i], target_vec + start_index, proj_norms[i]);
        start_index += spaces[i]->get_num_dofs();
      }
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(std::vector<SpaceSharedPtr<Scalar> > spaces, std::vector<MeshFunctionSharedPtr<Scalar> > source_slns,
      Hermes::Algebra::Vector<Scalar>* target_vec, std::vector<NormType> proj_norms)
    {
      if (target_vec->get_size() != Space<Scalar>::get_num_dofs(spaces))
        throw Exceptions::ValueException("target_vec->size", target_vec->get_size(), Space<Scalar>::get_num_dofs(spaces));

      Scalar* vec = malloc_with_check<Scalar>(target_vec->get_size());
      project_global(spaces, source_slns, vec, proj_norms);
      target_vec->set_vector(vec);
      free_with_check(vec);
    }

    template<typename Scalar>
    void OGProjection<Scalar>::project_global(std::vector<SpaceSharedPtr<Scalar> > spaces, std::vector<MeshFunctionSharedPtr<Scalar> > source_slns,
      std::vector<MeshFunctionSharedPtr<Scalar> > target_slns, std::vector<NormType> proj_norms, bool delete_old_meshes)
    {
      int n = spaces.size();

      // Sanity checks.
      if (n != source_slns.size())
        throw Exceptions::LengthException(1, 2, n, source_slns.size());
      if (n != target_slns.size())
        throw Exceptions::LengthException(1, 2, n, target_slns.size());
      if (!proj_norms.empty() && n != proj_norms.size())
        throw Exceptions::LengthException(1, 5, n, proj_norms.size());

      for (int i = 0; i < n; i++)
      {
        if (proj_norms.empty())
          project_global(spaces[i], source_slns[i], target_slns[i], HERMES_UNSET_NORM);
        else
          project_global(spaces[i], source_slns[i], target_slns[i], proj_norms[i]);
      }
    }

    template class HERMES_API OGProjection < double > ;
    template class HERMES_API OGProjection < std::complex<double> > ;
  }
}