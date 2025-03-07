#ifndef IHDG_SWE_PROBLEM_HPP
#define IHDG_SWE_PROBLEM_HPP

#include "problem/SWE/swe_definitions.hpp"
#include "problem/SWE/problem_input/swe_inputs.hpp"
#include "problem/SWE/discretization_RKDG/numerical_fluxes/rkdg_swe_numerical_fluxes.hpp"

#include "simulation/stepper/implicit_stepper.hpp"
#include "simulation/writer.hpp"
#include "simulation/discretization.hpp"

#include "problem/SWE/problem_stabilization_parameters/swe_stabilization_params.hpp"

#include "boundary_conditions/ihdg_swe_boundary_conditions.hpp"
#include "dist_boundary_conditions/ihdg_swe_distributed_boundary_conditions.hpp"
#include "interface_specializations/ihdg_swe_interface_specializations.hpp"

#include "problem/SWE/problem_data_structure/swe_data.hpp"
#include "problem/SWE/problem_data_structure/swe_edge_data.hpp"
#include "problem/SWE/problem_data_structure/swe_global_data.hpp"

#include "problem/SWE/problem_input/swe_inputs.hpp"
#include "problem/SWE/problem_parser/swe_parser.hpp"
#include "problem/SWE/problem_preprocessor/swe_preprocessor.hpp"
#include "problem/SWE/problem_postprocessor/swe_postprocessor.hpp"

#include "geometry/mesh_definitions.hpp"
#include "geometry/mesh_skeleton_definitions.hpp"
#include "preprocessor/mesh_metadata.hpp"

#include "problem/SWE/problem_preprocessor/swe_pre_create_intface.hpp"
#include "problem/SWE/problem_preprocessor/swe_pre_create_bound.hpp"
#include "problem/SWE/problem_preprocessor/swe_pre_create_dbound.hpp"
#include "problem/SWE/problem_preprocessor/swe_pre_create_edge_intface.hpp"
#include "problem/SWE/problem_preprocessor/swe_pre_create_edge_bound.hpp"
#include "problem/SWE/problem_preprocessor/swe_pre_create_edge_dbound.hpp"

namespace SWE {
namespace IHDG {
struct Problem {
    using ProblemInputType   = SWE::Inputs;
    using ProblemStepperType = ImplicitStepper;
    using ProblemWriterType  = Writer<Problem>;
    using ProblemParserType  = SWE::Parser;

    using ProblemDataType       = SWE::Data;
    using ProblemEdgeDataType   = SWE::EdgeData;
    using ProblemGlobalDataType = SWE::GlobalData;

    using ProblemInterfaceTypes = Geometry::InterfaceTypeTuple<Data, ISP::Internal, ISP::Levee>;
    using ProblemBoundaryTypes =
        Geometry::BoundaryTypeTuple<Data, BC::Land, BC::Tide, BC::Flow, BC::Function, BC::Outflow>;
    using ProblemDistributedBoundaryTypes =
        Geometry::DistributedBoundaryTypeTuple<Data, DBC::Distributed, DBC::DistributedLevee>;

    using ProblemEdgeInterfaceTypes = Geometry::EdgeInterfaceTypeTuple<EdgeData, ProblemInterfaceTypes>::Type;
    using ProblemEdgeBoundaryTypes  = Geometry::EdgeBoundaryTypeTuple<EdgeData, ProblemBoundaryTypes>::Type;
    using ProblemEdgeDistributedTypes =
        Geometry::EdgeDistributedTypeTuple<EdgeData, ProblemDistributedBoundaryTypes>::Type;

    using ProblemMeshType = Geometry::MeshType<Data,
                                               std::tuple<ISP::Internal, ISP::Levee>,
                                               std::tuple<BC::Land, BC::Tide, BC::Flow, BC::Function, BC::Outflow>,
                                               std::tuple<DBC::Distributed, DBC::DistributedLevee>>::Type;

    using ProblemMeshSkeletonType = Geometry::MeshSkeletonType<
        EdgeData,
        Geometry::InterfaceTypeTuple<Data, ISP::Internal, ISP::Levee>,
        Geometry::BoundaryTypeTuple<Data, BC::Land, BC::Tide, BC::Flow, BC::Function, BC::Outflow>,
        Geometry::DistributedBoundaryTypeTuple<Data, DBC::Distributed, DBC::DistributedLevee>>::Type;

    using ProblemDiscretizationType = HDGDiscretization<Problem>;

    // preprocessor kernels
    static void initialize_problem_parameters(ProblemInputType& problem_specific_input) {
        SWE::initialize_problem_parameters(problem_specific_input);
    }

    static void preprocess_mesh_data(InputParameters<ProblemInputType>& input) { SWE::preprocess_mesh_data(input); }

    // helpers to create communication
    static constexpr uint n_communications = SWE::IHDG::n_communications;

    static std::vector<uint> comm_buffer_offsets(std::vector<uint>& begin_index, uint ngp) {
        std::vector<uint> offset(SWE::IHDG::n_communications);

        offset[CommTypes::baryctr_coord]    = begin_index[CommTypes::baryctr_coord];
        offset[CommTypes::init_global_prob] = begin_index[CommTypes::init_global_prob];
        offset[CommTypes::baryctr_state]    = begin_index[CommTypes::baryctr_state];

        begin_index[CommTypes::baryctr_coord] += 2;
        begin_index[CommTypes::init_global_prob] +=
            1 + (SWE::n_variables + 1) * ngp;                           // global dof, q_at_gp, bath_at_gp
        begin_index[CommTypes::baryctr_state] += SWE::n_variables + 1;  // + w/d state

        return offset;
    }

    template <typename RawBoundaryType>
    static void create_interfaces(std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
                                  ProblemMeshType& mesh,
                                  ProblemInputType& input,
                                  ProblemWriterType& writer) {
        SWE::create_interfaces<SWE::IHDG::Problem>(raw_boundaries, mesh, input, writer);
    }

    template <typename RawBoundaryType>
    static void create_boundaries(std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
                                  ProblemMeshType& mesh,
                                  ProblemInputType& input,
                                  ProblemWriterType& writer) {
        SWE::create_boundaries<SWE::IHDG::Problem>(raw_boundaries, mesh, input, writer);
    }

    template <typename RawBoundaryType>
    static void create_distributed_boundaries(
        std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
        ProblemMeshType&,
        ProblemInputType& problem_input,
        std::tuple<>&,
        ProblemWriterType&) {}

    template <typename RawBoundaryType, typename Communicator>
    static void create_distributed_boundaries(
        std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
        ProblemMeshType& mesh,
        ProblemInputType& input,
        Communicator& communicator,
        ProblemWriterType& writer) {
        // *** //
        SWE::create_distributed_boundaries<SWE::IHDG::Problem>(raw_boundaries, mesh, input, communicator, writer);
    }

    static void create_edge_interfaces(ProblemMeshType& mesh,
                                       ProblemMeshSkeletonType& mesh_skeleton,
                                       ProblemWriterType& writer) {
        SWE::create_edge_interfaces<SWE::IHDG::Problem>(mesh, mesh_skeleton, writer);
    }

    static void create_edge_boundaries(ProblemMeshType& mesh,
                                       ProblemMeshSkeletonType& mesh_skeleton,
                                       ProblemWriterType& writer) {
        SWE::create_edge_boundaries<SWE::IHDG::Problem>(mesh, mesh_skeleton, writer);
    }

    static void create_edge_distributeds(ProblemMeshType& mesh,
                                         ProblemMeshSkeletonType& mesh_skeleton,
                                         ProblemWriterType& writer) {
        SWE::create_edge_distributeds<SWE::IHDG::Problem>(mesh, mesh_skeleton, writer);
    }

    template <typename ProblemType>
    static void preprocessor_serial(HDGDiscretization<ProblemType>& discretization,
                                    typename ProblemType::ProblemGlobalDataType& global_data,
                                    const ProblemStepperType& stepper,
                                    const typename ProblemType::ProblemInputType& problem_specific_input);

    template <template <typename> class OMPISimUnitType, typename ProblemType>
    static void preprocessor_ompi(std::vector<std::unique_ptr<OMPISimUnitType<ProblemType>>>& sim_units,
                                  typename ProblemType::ProblemGlobalDataType& global_data,
                                  const ProblemStepperType& stepper,
                                  const uint begin_sim_id,
                                  const uint end_sim_id);

    template <typename ProblemType>
    static void initialize_global_problem_serial(HDGDiscretization<ProblemType>& discretization,
                                                 const ProblemStepperType& stepper,
                                                 uint& global_dof_offset);

    template <typename ProblemType>
    static void initialize_global_problem_parallel_pre_send(HDGDiscretization<ProblemType>& discretization,
                                                            const ProblemStepperType& stepper,
                                                            uint& global_dof_offset);

    template <typename ProblemType>
    static void initialize_global_problem_parallel_finalize_pre_send(HDGDiscretization<ProblemType>& discretization,
                                                                     uint global_dof_offset);

    template <typename ProblemType>
    static void initialize_global_problem_parallel_post_receive(HDGDiscretization<ProblemType>& discretization,
                                                                std::vector<uint>& global_dof_indx);

    // processor kernels
    template <typename ProblemType>
    static void step_serial(HDGDiscretization<ProblemType>& discretization,
                            typename ProblemType::ProblemGlobalDataType& global_data,
                            ProblemStepperType& stepper,
                            typename ProblemType::ProblemWriterType& writer,
                            typename ProblemType::ProblemParserType& parser);

    template <typename ProblemType>
    static void stage_serial(HDGDiscretization<ProblemType>& discretization,
                             typename ProblemType::ProblemGlobalDataType& global_data,
                             ProblemStepperType& stepper);

    template <template <typename> class OMPISimUnitType, typename ProblemType>
    static void step_ompi(std::vector<std::unique_ptr<OMPISimUnitType<ProblemType>>>& sim_units,
                          typename ProblemType::ProblemGlobalDataType& global_data,
                          ProblemStepperType& stepper,
                          const uint begin_sim_id,
                          const uint end_sim_id);

    template <template <typename> class OMPISimUnitType, typename ProblemType>
    static void stage_ompi(std::vector<std::unique_ptr<OMPISimUnitType<ProblemType>>>& sim_units,
                           typename ProblemType::ProblemGlobalDataType& global_data,
                           ProblemStepperType& stepper,
                           const uint begin_sim_id,
                           const uint end_sim_id);

    /* init interation begin */

    template <typename ProblemType>
    static void init_iteration(const ProblemStepperType& stepper, HDGDiscretization<ProblemType>& discretization);

    template <typename ElementType>
    static void init_volume_kernel(const ProblemStepperType& stepper, ElementType& elt);

    template <typename ElementType>
    static void init_source_kernel(const ProblemStepperType& stepper, ElementType& elt);

    template <typename InterfaceType>
    static void init_interface_kernel(const ProblemStepperType& stepper, InterfaceType& intface);

    template <typename BoundaryType>
    static void init_boundary_kernel(const ProblemStepperType& stepper, BoundaryType& bound);

    template <typename DistributedBoundaryType>
    static void init_distributed_boundary_kernel(const ProblemStepperType& stepper, DistributedBoundaryType& dbound);

    template <typename EdgeInterfaceType>
    static void init_edge_interface_kernel(const ProblemStepperType& stepper, EdgeInterfaceType& edge_int);

    template <typename EdgeBoundaryType>
    static void init_edge_boundary_kernel(const ProblemStepperType& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeDistributedType>
    static void init_edge_distributed_kernel(const ProblemStepperType& stepper, EdgeDistributedType& edge_dbound);

    /* init interation end */

    /* local step begin */

    template <typename ElementType>
    static void local_volume_kernel(const ProblemStepperType& stepper, ElementType& elt);

    template <typename ElementType>
    static void local_source_kernel(const ProblemStepperType& stepper, ElementType& elt);

    template <typename InterfaceType>
    static void local_interface_kernel(const ProblemStepperType& stepper, InterfaceType& intface);

    template <typename BoundaryType>
    static void local_boundary_kernel(const ProblemStepperType& stepper, BoundaryType& bound);

    template <typename DistributedBoundaryType>
    static void local_distributed_boundary_kernel(const ProblemStepperType& stepper, DistributedBoundaryType& dbound);

    template <typename EdgeInterfaceType>
    static void local_edge_interface_kernel(const ProblemStepperType& stepper, EdgeInterfaceType& edge_int);

    template <typename EdgeBoundaryType>
    static void local_edge_boundary_kernel(const ProblemStepperType& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeDistributedType>
    static void local_edge_distributed_kernel(const ProblemStepperType& stepper, EdgeDistributedType& edge_dbound);

    /* local step end */

    /* global step begin */

    template <typename EdgeInterfaceType>
    static void global_edge_interface_kernel(const ProblemStepperType& stepper, EdgeInterfaceType& edge_int);

    template <typename EdgeBoundaryType>
    static void global_edge_boundary_kernel(const ProblemStepperType& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeDistributedType>
    static void global_edge_distributed_kernel(const ProblemStepperType& stepper, EdgeDistributedType& edge_bound);

    template <typename ProblemType>
    static bool serial_solve_global_problem(HDGDiscretization<ProblemType>& discretization,
                                            typename ProblemType::ProblemGlobalDataType& global_data,
                                            const ProblemStepperType& stepper);

    template <template <typename> class OMPISimUnitType, typename ProblemType>
    static bool ompi_solve_global_problem(std::vector<std::unique_ptr<OMPISimUnitType<ProblemType>>>& sim_units,
                                          typename ProblemType::ProblemGlobalDataType& global_data,
                                          const ProblemStepperType& stepper,
                                          const uint begin_sim_id,
                                          const uint end_sim_id);

    /* global step end */

    // writing output kernels
    template <typename MeshType>
    static void write_VTK_data(MeshType& mesh, std::ofstream& raw_data_file) {
        SWE::write_VTK_data(mesh, raw_data_file);
    }

    template <typename MeshType>
    static void write_VTU_data(MeshType& mesh, std::ofstream& raw_data_file) {
        SWE::write_VTU_data(mesh, raw_data_file);
    }

    template <typename MeshType>
    static void write_modal_data(const ProblemStepperType& stepper, MeshType& mesh, const std::string& output_path) {
        SWE::write_modal_data(stepper, mesh, output_path);
    }

    template <typename ElementType>
    static double compute_residual_L2(const ProblemStepperType& stepper, ElementType& elt) {
        return SWE::compute_residual_L2(stepper, elt);
    }

    template <typename GlobalDataType>
    static void finalize_simulation(GlobalDataType& global_data) {
#ifdef HAS_PETSC
        global_data.destroy();
#endif
    }
};
}
}

#endif
