/// This file is part of Hermes2D.
///
/// Hermes2D is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 2 of the License, or
/// (at your option) any later version.
///
/// Hermes2D is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY;without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with Hermes2D. If not, see <http:///www.gnu.org/licenses/>.

#ifndef __H2D_DISCRETE_PROBLEM_H
#define __H2D_DISCRETE_PROBLEM_H

#include "hermes_common.h"
#include "adapt/adapt.h"
#include "graph.h"
#include "forms.h"
#include "weakform/weakform.h"
#include "function/function.h"
#include "neighbor.h"
#include "refinement_selectors/selector.h"
#include "exceptions.h"

namespace Hermes
{
  namespace Hermes2D
  {
    class PrecalcShapeset;

    /// Multimesh neighbors traversal class.
    class NeighborNode
    {
    private:
      NeighborNode(NeighborNode* parent, unsigned int transformation);
      ~NeighborNode();
      void set_left_son(NeighborNode* left_son);
      void set_right_son(NeighborNode* right_son);
      void set_transformation(unsigned int transformation);
      NeighborNode* get_left_son();
      NeighborNode* get_right_son();
      unsigned int get_transformation();
      NeighborNode* parent;
      NeighborNode* left_son;
      NeighborNode* right_son;
      unsigned int transformation;
      template<typename Scalar> friend class DiscreteProblem;
      template<typename Scalar> friend class KellyTypeAdapt;
    };

    /// Discrete problem class.
    ///
    /// This class does assembling into external matrix / vector structures.
    ///
    template<typename Scalar>
    class HERMES_API DiscreteProblem : public DiscreteProblemInterface<Scalar>
    {
    public:
      /// Constructor for multiple components / equations.
      DiscreteProblem(const WeakForm<Scalar>* wf, Hermes::vector<const Space<Scalar> *> spaces);

      /// Constructor for one equation.
      DiscreteProblem(const WeakForm<Scalar>* wf, const Space<Scalar>* space);

      /// Non-parameterized constructor (currently used only in KellyTypeAdapt to gain access to NeighborSearch methods).
      DiscreteProblem();

      /// Destuctor.
      virtual ~DiscreteProblem();

      /// Assembling.
      /// General assembling procedure for nonlinear problems. coeff_vec is the
      /// previous Newton vector. If force_diagonal_block == true, then (zero) matrix
      /// antries are created in diagonal blocks even if corresponding matrix weak
      /// forms do not exist. This is useful if the matrix is later to be merged with
      /// a matrix that has nonzeros in these blocks. The Table serves for optional
      /// weighting of matrix blocks in systems. The parameter add_dir_lift decides
      /// whether Dirichlet lift will be added while coeff_vec is converted into
      /// Solutions.
      void assemble(Scalar* coeff_vec, SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL,
        bool force_diagonal_blocks = false, Table* block_weights = NULL);

      /// Assembling.
      /// Without the matrix.
      void assemble(Scalar* coeff_vec, Vector<Scalar>* rhs = NULL,
        bool force_diagonal_blocks = false, Table* block_weights = NULL);

      /// Light version passing NULL for the coefficient vector. External solutions
      /// are initialized with zeros.
      void assemble(SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL, bool force_diagonal_blocks = false,
        Table* block_weights = NULL);

      /// Light version passing NULL for the coefficient vector. External solutions
      /// are initialized with zeros.
      /// Without the matrix.
      void assemble(Vector<Scalar>* rhs = NULL, bool force_diagonal_blocks = false,
        Table* block_weights = NULL);

      void invalidate_matrix();

      /// Set this problem to Finite Volume.
      void set_fvm();

    protected:
      /// The form will be assembled.
      bool form_to_be_assembled(MatrixForm<Scalar>* form);
      bool form_to_be_assembled(MatrixFormVol<Scalar>* form);
      bool form_to_be_assembled(MatrixFormSurf<Scalar>* form);
      bool form_to_be_assembled(VectorForm<Scalar>* form);
      bool form_to_be_assembled(VectorFormVol<Scalar>* form);
      bool form_to_be_assembled(VectorFormSurf<Scalar>* form);

      // Return scaling coefficient.
      double block_scaling_coeff(MatrixForm<Scalar>* form);
      
      /// The current stage contains DG forms.
      void is_DG_stage();

      /// Get the number of unknowns.
      int get_num_dofs();

      /// Get info about presence of a matrix.
      bool is_matrix_free();

      // GET functions.
      /// Get pointer to n-th space.
      const Space<Scalar>* get_space(int n);

      /// Get the weak forms.
      const WeakForm<Scalar>* get_weak_formulation();

      /// Get all spaces as a Hermes::vector.
      Hermes::vector<const Space<Scalar>*> get_spaces();

      /// This is different from H3D.
      PrecalcShapeset* get_pss(int n);

      /// Preassembling.
      /// Precalculate matrix sparse structure.
      /// If force_diagonal_block == true, then (zero) matrix
      /// antries are created in diagonal blocks even if corresponding matrix weak
      /// forms do not exist. This is useful if the matrix is later to be merged with
      /// a matrix that has nonzeros in these blocks. The Table serves for optional
      /// weighting of matrix blocks in systems.
      void create_sparse_structure();
      void create_sparse_structure(SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs = NULL);

      /// Initializes psss.
      void init_psss();

      /// Initializes refmaps.
      void init_refmaps();
      
      /// Initializes u_ext.
      void init_u_ext(Scalar* coeff_vec);
      
      /// De-initializes u_ext.
      void deinit_u_ext();

      /// De-initializes psss.
      void deinit_psss();

      /// De-initializes refmaps.
      void deinit_refmaps();

      /// Initialize a state, returns a non-NULL Element.
      Element* init_state();
      void init_surface_state();

      /// Set the special handling of external functions of Runge-Kutta methods, including information how many spaces were there in the original problem.
      inline void set_RK(int original_spaces_count) { this->RungeKutta = true; RK_original_spaces_count = original_spaces_count; }

      /// Assemble one stage.
      void assemble_one_stage();

      /// Assemble one state.
      void assemble_one_state();
      
      /// Adjusts order to refmaps.
      void adjust_order_to_refmaps(Form<Scalar> *form, int& order, Hermes::Ord* o);

      /// Matrix volumetric forms - calculate the integration order.
      int calc_order_matrix_form(MatrixForm<Scalar>* mfv);

      /// Matrix volumetric forms - assemble the form.
      void assemble_matrix_form(MatrixForm<Scalar>* form, int order, Func<double>** base_fns, Func<double>** test_fns);

      /// Vector volumetric forms - calculate the integration order.
      int calc_order_vector_form(VectorForm<Scalar>* mfv);

      /// Vector volumetric forms - assemble the form.
      void assemble_vector_form(VectorForm<Scalar>* form, int order, Func<double>** test_fns);


      /// \ingroup Helper methods inside {calc_order_*, assemble_*}
      /// Init geometry, jacobian * weights, return the number of integration points.
      int init_geometry_points(RefMap* reference_mapping, int order);
      int init_surface_geometry_points(RefMap* reference_mapping, int& order);
      
      /// \ingroup Helper methods inside {calc_order_*, assemble_*}
      /// Calculates orders for external functions.
      void init_ext_orders(Form<Scalar> *form, Func<Hermes::Ord>** oi, ExtData<Hermes::Ord>* oext);
      /// \ingroup Helper methods inside {calc_order_*, assemble_*}
      /// Cleans up after init_ext_orders.
      void deinit_ext_orders(Form<Scalar> *form, Func<Hermes::Ord>** oi, ExtData<Hermes::Ord>* oext);

      /// \ingroup Helper methods inside {calc_order_*, assemble_*}
      /// Calculates external functions.
      void init_ext(Form<Scalar> *form, Func<Scalar>** u_ext, ExtData<Scalar>* ext, int order);
      /// \ingroup Helper methods inside {calc_order_*, assemble_*}
      /// Cleans up after init_ext.
      void deinit_ext(Form<Scalar> *form, Func<Scalar>** u_ext, ExtData<Scalar>* ext);

      
      /// Init function. Common code for the constructors.
      void init();

      DiscontinuousFunc<Hermes::Ord>* init_ext_fn_ord(NeighborSearch<Scalar>* ns, MeshFunction<Scalar>* fu);

      /// Calculates integration order for DG matrix forms.
      int calc_order_dg_matrix_form(MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
        PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
        bool neighbor_supp_u, bool neighbor_supp_v, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u);

      /// Assemble DG forms.
      void assemble_DG_forms(Stage<Scalar>& stage,
        SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
        Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, Hermes::vector<Solution<Scalar>*>& u_ext,
        int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat,
        int isurf, Element** e, Element* trav_base, Element* rep_element);

      /// Assemble one DG neighbor.
      void assemble_DG_one_neighbor(bool edge_processed, unsigned int neighbor_i, Stage<Scalar>& stage,
        SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
        Hermes::vector<PrecalcShapeset *>& spss, Hermes::vector<RefMap *>& refmap, std::map<unsigned int, PrecalcShapeset *> npss,
        std::map<unsigned int, PrecalcShapeset *> nspss, std::map<unsigned int, RefMap *> nrefmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext,
        int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat,
        int isurf, Element** e, Element* trav_base, Element* rep_element);

      /// Assemble DG matrix forms.
      void assemble_DG_matrix_forms(Stage<Scalar>& stage,
        SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
        Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, std::map<unsigned int, PrecalcShapeset*> npss,
        std::map<unsigned int, PrecalcShapeset*> nspss, std::map<unsigned int, RefMap*> nrefmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext,
        int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat,
        int isurf, Element** e, Element* trav_base, Element* rep_element);

      /// Assemble DG vector forms.
      void assemble_DG_vector_forms(Stage<Scalar>& stage,
        SparseMatrix<Scalar>* mat, Vector<Scalar>* rhs, bool force_diagonal_blocks, Table* block_weights,
        Hermes::vector<PrecalcShapeset*>& spss, Hermes::vector<RefMap*>& refmap, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, Hermes::vector<Solution<Scalar>*>& u_ext,
        int marker, Hermes::vector<AsmList<Scalar>*>& al, bool bnd, SurfPos& surf_pos, Hermes::vector<bool>& nat,
        int isurf, Element** e, Element* trav_base, Element* rep_element);

      /// Evaluates DG matrix forms on an edge between elements identified by ru_actual, rv.
      Scalar eval_dg_form(MatrixFormSurf<Scalar>* mfs, Hermes::vector<Solution<Scalar>*> u_ext,
        PrecalcShapeset* fu, PrecalcShapeset* fv, RefMap* ru_central, RefMap* ru_actual, RefMap* rv,
        bool neighbor_supp_u, bool neighbor_supp_v,
        SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_u, int neighbor_index_v);

      /// Calculates integration order for DG vector forms.
      int calc_order_dg_vector_form(VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
        PrecalcShapeset* fv, RefMap* ru, SurfPos* surf_pos,
        LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v);

      /// Evaluates DG vector forms on an edge between elements identified by ru_actual, rv.
      Scalar eval_dg_form(VectorFormSurf<Scalar>* vfs, Hermes::vector<Solution<Scalar>*> u_ext,
        PrecalcShapeset* fv, RefMap* rv,
        SurfPos* surf_pos, LightArray<NeighborSearch<Scalar>*>& neighbor_searches, int neighbor_index_v);

      /// Initialize orders of external functions for DG forms.
      ExtData<Hermes::Ord>* init_ext_fns_ord(Hermes::vector<MeshFunction<Scalar>*> &ext,
        LightArray<NeighborSearch<Scalar>*>& neighbor_searches);

      /// Initialize external functions for DG forms.
      ExtData<Scalar>* init_ext_fns(Hermes::vector<MeshFunction<Scalar>*> &ext,
        LightArray<NeighborSearch<Scalar>*>& neighbor_searches,
        int order);

      /// Uses assembling_caches to get (possibly cached) precalculated shapeset function values.
      Func<double>* get_fn(PrecalcShapeset* fu, RefMap* rm, const int order);

      /// Uses assembling_caches to get (possibly chached) dummy function for calculation of the integration order.
      Func<Hermes::Ord>* get_fn_ord(const int order);

      /// Initialize all caches.
      void init_cache();

      /// Deinitialize all caches.
      void delete_cache();

      /// Deinitialize a single geometry cache.
      void delete_single_geom_cache(int order);

      /// Initialize neighbors.
      void init_neighbors(LightArray<NeighborSearch<Scalar>*>& neighbor_searches, const Stage<Scalar>& stage, const int& isurf);

      /// Initialize the tree for traversing multimesh neighbors.
      void build_multimesh_tree(NeighborNode* root, LightArray<NeighborSearch<Scalar>*>& neighbor_searches);

      /// Recursive insertion function into the tree.
      void insert_into_multimesh_tree(NeighborNode* node, unsigned int* transformations, unsigned int transformation_count);

      /// Return a global (unified list of central element transformations representing the neighbors on the union mesh.
      Hermes::vector<Hermes::vector<unsigned int>*> get_multimesh_neighbors_transformations(NeighborNode* multimesh_tree);

      /// Traverse the multimesh tree. Used in the function get_multimesh_neighbors_transformations().
      void traverse_multimesh_tree(NeighborNode* node, Hermes::vector<Hermes::vector<unsigned int>*>& running_transformations);

      /// Update the NeighborSearch according to the multimesh tree.
      void update_neighbor_search(NeighborSearch<Scalar>* ns, NeighborNode* multimesh_tree);

      /// Finds a node in the multimesh tree that corresponds to the array transformations, with the length of transformation_count,
      /// starting to look for it in the NeighborNode node.
      NeighborNode* find_node(unsigned int* transformations, unsigned int transformation_count, NeighborNode* node);

      /// Updates the NeighborSearch ns according to the subtree of NeighborNode node.
      /// Returns 0 if no neighbor was deleted, -1 otherwise.
      unsigned int update_ns_subtree(NeighborSearch<Scalar>* ns, NeighborNode* node, unsigned int ith_neighbor);

      /// Traverse the multimesh subtree. Used in the function update_ns_subtree().
      void traverse_multimesh_subtree(NeighborNode* node, Hermes::vector<Hermes::vector<unsigned int>*>& running_central_transformations,
        Hermes::vector<Hermes::vector<unsigned int>*>& running_neighbor_transformations, const typename NeighborSearch<Scalar>::NeighborEdgeInfo& edge_info, const int& active_edge, const int& mode);

      /// Returns the matrix_buffer of the size n.
      Scalar** get_matrix_buffer(int n);

      /// Matrix structure as well as spaces and weak formulation is up-to-date.
      bool is_up_to_date();

      /// Minimum identifier of the meshes used in DG assembling in one stage.
      unsigned int min_dg_mesh_seq;

      /// Weak formulation.
      const WeakForm<Scalar>* wf;

      /// Seq number of the WeakForm.
      int wf_seq;

      /// Space instances for all equations in the system.
      Hermes::vector<const Space<Scalar>*> spaces;
      Hermes::vector<unsigned int> spaces_first_dofs;

      /// Seq numbers of Space instances in spaces.
      int* sp_seq;

      /// Number of DOFs of all Space instances in spaces.
      int ndof;

      /// Element usage flag: iempty[i] == true if the current state does not posses an active element in the i-th space.
      Hermes::vector<bool> isempty;

      /// Instance of the class Geom used in the calculation of integration order.
      Geom<Hermes::Ord> geom_ord;

      /// Fake weight used in the calculation of integration order.
      static double fake_wt;

      /// If the problem has only constant test functions, there is no need for order calculation,
      /// which saves time.
      bool is_fvm;

      Scalar** matrix_buffer;///< buffer for holding square matrix (during assembling)

      int matrix_buffer_dim;///< dimension of the matrix held by 'matrix_buffer'

      /// Matrix structure can be reused.
      /// If other conditions apply.
      bool have_matrix;

      /// PrecalcShapeset instances for the problem (as many as equations in the system).
      PrecalcShapeset** pss;

      /// Geometry cache.
      Geom<double>* geometry_cache[g_max_quad + 1 + 4* g_max_quad + 4];

      /// Jacobian * weights cache.
      double* jacobian_x_weights_cache[g_max_quad + 1 + 4* g_max_quad + 4];

      /// There is a matrix form set on DG_INNER_EDGE area or not.
      bool DG_matrix_forms_present;

      /// There is a vector form set on DG_INNER_EDGE area or not.
      bool DG_vector_forms_present;

      /// Turn on Runge-Kutta specific handling of external functions.
      bool RungeKutta;

      /// Number of spaces in the original problem in a Runge-Kutta method.
      int RK_original_spaces_count;

      /// Storing assembling info.
      Stage<Scalar>* current_stage;
      SparseMatrix<Scalar>* current_mat;
      Vector<Scalar>* current_rhs;
      bool current_force_diagonal_blocks;
      Table* current_block_weights;
      Traverse::State* current_state;
      int current_isurf;
      Hermes::vector<RefMap *> current_refmap;
      Hermes::vector<PrecalcShapeset *> current_spss;
      Hermes::vector<Solution<Scalar>*> current_u_ext;
      Hermes::vector<AsmList<Scalar>*> current_al;

      Quad2D* quad;
      
      void set_quad_2d(Quad2D* quad);


      /// Class handling various caches used in assembling.
      class AssemblingCaches
      {
      public:
        /// Basic constructor.
        AssemblingCaches();

        /// Basic destructor.
        ~AssemblingCaches();

        /// Key for caching precalculated shapeset values on transformed elements with constant
        /// jacobians.
        struct KeyConst
        {
          int index;
          int order;
#ifdef _MSC_VER
          UINT64 sub_idx;
#else
          unsigned int sub_idx;
#endif
          int shapeset_type;
          double inv_ref_map[2][2];
#ifdef _MSC_VER
          KeyConst(int index, int order, UINT64 sub_idx, int shapeset_type, double2x2* inv_ref_map);
#else
          KeyConst(int index, int order, unsigned int sub_idx, int shapeset_type, double2x2* inv_ref_map);
#endif
        };

        /// Functor that compares two above keys (needed e.g. to create a std::map indexed by these keys);
        struct CompareConst
        {
          bool operator()(KeyConst a, KeyConst b) const;
        };

        /// PrecalcShapeset stored values for Elements with constant jacobian of the reference mapping for triangles.
        std::map<KeyConst, Func<double>* , CompareConst> const_cache_fn_triangles;

        /// PrecalcShapeset stored values for Elements with constant jacobian of the reference mapping for quads.
        std::map<KeyConst, Func<double>* , CompareConst> const_cache_fn_quads;

        /// The same setup for elements with non-constant jacobians.
        /// This cache is deleted with every change of the state in assembling.
        struct KeyNonConst
        {
          int index;
          int order;
#ifdef _MSC_VER
          UINT64 sub_idx;
#else
          unsigned int sub_idx;
#endif
          int shapeset_type;
#ifdef _MSC_VER
          KeyNonConst(int index, int order, UINT64 sub_idx, int shapeset_type);
#else
          KeyNonConst(int index, int order, unsigned int sub_idx, int shapeset_type);
#endif
        };

        /// Functor that compares two above keys (needed e.g. to create a std::map indexed by these keys);
        struct CompareNonConst
        {
          bool operator()(KeyNonConst a, KeyNonConst b) const;
        };

        /// PrecalcShapeset stored values for Elements with non-constant jacobian of the reference mapping for triangles.
        std::map<KeyNonConst, Func<double>* , CompareNonConst> cache_fn_triangles;

        /// PrecalcShapeset stored values for Elements with non-constant jacobian of the reference mapping for quads.
        std::map<KeyNonConst, Func<double>* , CompareNonConst> cache_fn_quads;

        LightArray<Func<Hermes::Ord>*> cache_fn_ord;
      };

      /// An AssemblingCaches instance for this instance of DiscreteProblem.
      AssemblingCaches assembling_caches;

      template<typename T> friend class KellyTypeAdapt;
      template<typename T> friend class NewtonSolver;
      template<typename T> friend class PicardSolver;
      template<typename T> friend class RungeKutta;
    };
  }
}
#endif
