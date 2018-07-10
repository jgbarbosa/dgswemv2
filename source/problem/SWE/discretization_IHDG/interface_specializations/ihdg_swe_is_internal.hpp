#ifndef IHDG_SWE_IS_INTERNAL_HPP
#define IHDG_SWE_IS_INTERNAL_HPP

#include "general_definitions.hpp"
#include "simulation/stepper/rk_stepper.hpp"
#include "problem/SWE/discretization_IHDG/stabilization_parameters/ihdg_swe_stabilization_params.hpp"

namespace SWE {
namespace IHDG {
namespace IS {
class Internal {
  public:
    template <typename InterfaceType>
    void Initialize(InterfaceType& intface) {} /*nothing to initialize*/

    template <typename EdgeInterfaceType>
    void ComputeGlobalKernels(EdgeInterfaceType& edge_int);

    template <typename EdgeInterfaceType>
    void ComputeNumericalFlux(EdgeInterfaceType& edge_int);
};

template <typename EdgeInterfaceType>
void Internal::ComputeGlobalKernels(EdgeInterfaceType& edge_int) {
    auto& edge_global = edge_int.edge_data.edge_global;

    auto& boundary_in = edge_int.interface.data_in.boundary[edge_int.interface.bound_id_in];
    auto& boundary_ex = edge_int.interface.data_ex.boundary[edge_int.interface.bound_id_ex];

    uint gp_ex;
    for (uint gp = 0; gp < edge_int.edge_data.get_ngp(); ++gp) {
        gp_ex = edge_int.edge_data.get_ngp() - gp - 1;

        edge_global.rhs_kernel_at_gp[gp] = -boundary_in.F_hat_at_gp[gp];
        edge_global.rhs_kernel_at_gp[gp] += -boundary_ex.F_hat_at_gp[gp_ex];

        edge_global.delta_hat_kernel_at_gp[gp] = boundary_in.dF_hat_dq_hat_at_gp[gp];
        edge_global.delta_hat_kernel_at_gp[gp] += boundary_ex.dF_hat_dq_hat_at_gp[gp_ex];
    }
}

template <typename EdgeInterfaceType>
void Internal::ComputeNumericalFlux(EdgeInterfaceType& edge_int) {
    auto& boundary_in = edge_int.interface.data_in.boundary[edge_int.interface.bound_id_in];
    auto& boundary_ex = edge_int.interface.data_ex.boundary[edge_int.interface.bound_id_ex];

    boundary_in.F_hat_at_gp = boundary_in.Fn_at_gp;
    boundary_ex.F_hat_at_gp = boundary_ex.Fn_at_gp;

    add_F_hat_tau_terms_intface_LF(edge_int);
}
}
}
}

#endif