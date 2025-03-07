#ifndef EHDG_GN_DBC_DISTRIBUTED_HPP
#define EHDG_GN_DBC_DISTRIBUTED_HPP

#include "communication/db_data_exchanger.hpp"

namespace GN {
namespace EHDG {
namespace DBC {
class Distributed : public SWE_SIM::DBC::Distributed {
  public:
    Distributed(const DBDataExchanger& exchanger) : SWE_SIM::DBC::Distributed(exchanger) {}

    template <typename EdgeDistributedType>
    void ComputeGlobalKernelsDC(EdgeDistributedType& edge_dbound);
};

template <typename EdgeDistributedType>
void Distributed::ComputeGlobalKernelsDC(EdgeDistributedType& edge_dbound) {
    auto& edge_internal = edge_dbound.edge_data.edge_internal;
    auto& boundary      = edge_dbound.boundary.data.boundary[edge_dbound.boundary.bound_id];

    double tau = -20;

    set_constant(edge_internal.w1_hat_w1_hat_kernel_at_gp, 0.0);
    set_constant(row(edge_internal.w1_hat_w1_hat_kernel_at_gp, RowMajTrans2D::xx), -tau);
    set_constant(row(edge_internal.w1_hat_w1_hat_kernel_at_gp, RowMajTrans2D::yy), -tau);

    set_constant(boundary.w1_hat_w1_kernel_at_gp, 0.0);
    set_constant(row(boundary.w1_hat_w1_kernel_at_gp, RowMajTrans2D::xx), tau);
    set_constant(row(boundary.w1_hat_w1_kernel_at_gp, RowMajTrans2D::yy), tau);

    boundary.w1_hat_w2_kernel_at_gp = edge_dbound.boundary.surface_normal;
}
}
}
}

#endif