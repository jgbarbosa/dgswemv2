#ifndef IHDG_SWE_PROBLEM_HPP
#define IHDG_SWE_PROBLEM_HPP

#include "simulation/stepper/rk_stepper.hpp"
#include "simulation/writer.hpp"
#include "simulation/discretization.hpp"

#include "problem/SWE/swe_definitions.hpp"

#include "boundary_conditions/ihdg_swe_boundary_conditions.hpp"
#include "dist_boundary_conditions/ihdg_swe_distributed_boundary_conditions.hpp"
#include "interface_specializations/ihdg_swe_interface_specializations.hpp"

#include "data_structure/ihdg_swe_data.hpp"
#include "data_structure/ihdg_swe_edge_data.hpp"
#include "data_structure/ihdg_swe_global_data.hpp"

#include "problem/SWE/problem_input/swe_inputs.hpp"
#include "problem/SWE/problem_parser/swe_parser.hpp"
#include "problem/SWE/problem_postprocessor/swe_postprocessor.hpp"

#include "geometry/mesh_definitions.hpp"
#include "geometry/mesh_skeleton_definitions.hpp"
#include "preprocessor/mesh_metadata.hpp"

namespace SWE {
namespace IHDG {
struct Problem {
    using ProblemInputType   = SWE::Inputs;
    using ProblemStepperType = RKStepper;
    using ProblemWriterType  = Writer<Problem>;
    using ProblemParserType  = SWE::Parser;

    using ProblemDataType       = Data;
    using ProblemEdgeDataType   = EdgeData;
    using ProblemGlobalDataType = GlobalData;

    using ProblemInterfaceTypes = Geometry::InterfaceTypeTuple<Data, ISP::Internal, ISP::Levee>;
    using ProblemBoundaryTypes  = Geometry::BoundaryTypeTuple<Data, BC::Land, BC::Tide, BC::Flow>;
    using ProblemDistributedBoundaryTypes =
        Geometry::DistributedBoundaryTypeTuple<Data, DBC::Distributed, DBC::DistributedLevee>;

    using ProblemEdgeInterfaceTypes = Geometry::EdgeInterfaceTypeTuple<EdgeData, ProblemInterfaceTypes>::Type;
    using ProblemEdgeBoundaryTypes  = Geometry::EdgeBoundaryTypeTuple<EdgeData, ProblemBoundaryTypes>::Type;
    using ProblemEdgeDistributedTypes =
        Geometry::EdgeDistributedTypeTuple<EdgeData, ProblemDistributedBoundaryTypes>::Type;

    using ProblemMeshType = Geometry::MeshType<Data,
                                               std::tuple<ISP::Internal, ISP::Levee>,
                                               std::tuple<BC::Land, BC::Tide, BC::Flow>,
                                               std::tuple<DBC::Distributed, DBC::DistributedLevee>>::Type;

    using ProblemMeshSkeletonType = Geometry::MeshSkeletonType<
        EdgeData,
        Geometry::InterfaceTypeTuple<Data, ISP::Internal, ISP::Levee>,
        Geometry::BoundaryTypeTuple<Data, BC::Land, BC::Tide, BC::Flow>,
        Geometry::DistributedBoundaryTypeTuple<Data, DBC::Distributed, DBC::DistributedLevee>>::Type;

    using ProblemDiscretizationType = HDGDiscretization<Problem>;

    // preprocessor kernels
    static void initialize_problem_parameters(const ProblemInputType& problem_specific_input);

    static void preprocess_mesh_data(InputParameters<ProblemInputType>& input);

    template <typename RawBoundaryType>
    static void create_interfaces(std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
                                  ProblemMeshType& mesh,
                                  ProblemInputType& problem_input,
                                  ProblemWriterType& writer);

    template <typename RawBoundaryType>
    static void create_boundaries(std::map<uchar, std::map<std::pair<uint, uint>, RawBoundaryType>>& raw_boundaries,
                                  ProblemMeshType& mesh,
                                  ProblemInputType& problem_input,
                                  ProblemWriterType& writer);

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
        ProblemWriterType& writer);

    // helpers to create communication
    static constexpr uint n_communications = SWE::IHDG::n_communications;
    static std::vector<uint> comm_buffer_offsets(std::vector<uint>& begin_index, uint ngp);

    static void create_edge_interfaces(ProblemMeshType& mesh,
                                       ProblemMeshSkeletonType& mesh_skeleton,
                                       ProblemWriterType& writer);

    static void create_edge_boundaries(ProblemMeshType& mesh,
                                       ProblemMeshSkeletonType& mesh_skeleton,
                                       ProblemWriterType& writer);

    static void create_edge_distributeds(ProblemMeshType& mesh,
                                         ProblemMeshSkeletonType& mesh_skeleton,
                                         ProblemWriterType& writer);

    static void preprocessor_serial(ProblemDiscretizationType& discretization,
                                    const ProblemInputType& problem_specific_input);

    template <typename OMPISimUnitType>
    static void preprocessor_ompi(std::vector<std::unique_ptr<OMPISimUnitType>>& sim_units,
                                  uint begin_sim_id,
                                  uint end_sim_id);

    static void initialize_data_serial(ProblemMeshType& mesh, const ProblemInputType& problem_specific_input);

    static void initialize_data_parallel(ProblemMeshType& mesh, const ProblemInputType& problem_specific_input);

    static void initialize_global_problem_serial(ProblemDiscretizationType& discretization, uint& global_dof_offset);

    template <typename Communicator>
    static void initialize_global_problem_parallel_pre_send(ProblemDiscretizationType& discretization,
                                                            Communicator& communicator,
                                                            uint& global_dof_offset);

    static void initialize_global_problem_parallel_finalize_pre_send(ProblemDiscretizationType& discretization,
                                                                     uint global_dof_offset);

    template <typename Communicator>
    static void initialize_global_problem_parallel_post_receive(ProblemDiscretizationType& discretization,
                                                                Communicator& communicator,
                                                                std::vector<uint>& global_dof_indx);

    // processor kernels
    template <typename SerialSimType>
    static void step_serial(SerialSimType* sim);

    static void stage_serial(const RKStepper& stepper, ProblemDiscretizationType& discretization);

    template <typename OMPISimType>
    static void step_ompi(OMPISimType* sim, uint begin_sim_id, uint end_sim_id);

    template <typename OMPISimUnitType>
    static void stage_ompi(std::vector<std::unique_ptr<OMPISimUnitType>>& sim_units,
                           uint begin_sim_id,
                           uint end_sim_id);

    /* local step begin */

    template <typename ElementType>
    static void local_volume_kernel(const RKStepper& stepper, ElementType& elt);

    template <typename ElementType>
    static void local_source_kernel(const RKStepper& stepper, ElementType& elt);

    template <typename InterfaceType>
    static void local_interface_kernel(const RKStepper& stepper, InterfaceType& intface);

    template <typename BoundaryType>
    static void local_boundary_kernel(const RKStepper& stepper, BoundaryType& bound);

    template <typename DistributedBoundaryType>
    static void local_distributed_boundary_kernel(const RKStepper& stepper, DistributedBoundaryType& dbound);

    template <typename EdgeInterfaceType>
    static void local_edge_interface_kernel(const RKStepper& stepper, EdgeInterfaceType& edge_int);

    template <typename EdgeBoundaryType>
    static void local_edge_boundary_kernel(const RKStepper& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeDistributedType>
    static void local_edge_distributed_kernel(const RKStepper& stepper, EdgeDistributedType& edge_dbound);

    /* local step end */

    /* global step begin */

    template <typename EdgeInterfaceType>
    static void global_edge_interface_kernel(const RKStepper& stepper, EdgeInterfaceType& edge_int);

    template <typename EdgeBoundaryType>
    static void global_edge_boundary_kernel(const RKStepper& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeDistributedType>
    static void global_edge_distributed_kernel(const RKStepper& stepper, EdgeDistributedType& edge_bound);

    static bool serial_solve_global_problem(const RKStepper& stepper, ProblemDiscretizationType& discretization);

    template <typename OMPISimUnitType>
    static bool ompi_solve_global_problem(std::vector<std::unique_ptr<OMPISimUnitType>>& sim_units,
                                          uint begin_sim_id,
                                          uint end_sim_id);

    /* global step end */

    template <typename ElementType>
    static void update_kernel(const RKStepper& stepper, ElementType& elt);

    template <typename ElementType>
    static bool scrutinize_solution_kernel(const RKStepper& stepper, ElementType& elt);

    template <typename ElementType>
    static void swap_states_kernel(const RKStepper& stepper, ElementType& elt);

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
    static void write_modal_data(const RKStepper& stepper, MeshType& mesh, const std::string& output_path) {
        SWE::write_modal_data(stepper, mesh, output_path);
    }

    template <typename ElementType>
    static double compute_residual_L2(const RKStepper& stepper, ElementType& elt) {
        return SWE::compute_residual_L2(stepper, elt);
    }
};
}
}

#endif
