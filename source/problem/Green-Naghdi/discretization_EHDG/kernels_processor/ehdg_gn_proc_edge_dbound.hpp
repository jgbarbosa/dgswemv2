#ifndef EHDG_GN_PROC_EDGE_DBOUND_HPP
#define EHDG_GN_PROC_EDGE_DBOUND_HPP

namespace GN {
namespace EHDG {
template <typename EdgeDistributedType>
void Problem::local_dc_edge_distributed_kernel(const ESSPRKStepper& stepper, EdgeDistributedType& edge_dbound) {
    auto& edge_internal = edge_dbound.edge_data.edge_internal;
    auto& internal      = edge_dbound.boundary.data.internal;
    auto& boundary      = edge_dbound.boundary.data.boundary[edge_dbound.boundary.bound_id];

    double tau = -20;  // hardcode the tau value here

    // at this point h_hat_at_gp
    // has been calculated in derivatives kernel and stored to row(boundary.aux_at_gp, SWE::Auxiliaries::h)
    row(edge_internal.aux_hat_at_gp, SWE::Auxiliaries::h) = row(boundary.aux_at_gp, SWE::Auxiliaries::h);

    const auto h_hat = row(edge_internal.aux_hat_at_gp, SWE::Auxiliaries::h);
    const auto bx    = row(boundary.dbath_hat_at_gp, GlobalCoord::x);
    const auto by    = row(boundary.dbath_hat_at_gp, GlobalCoord::y);
    const auto nx    = row(edge_dbound.boundary.surface_normal, GlobalCoord::x);
    const auto ny    = row(edge_dbound.boundary.surface_normal, GlobalCoord::y);

    set_constant(boundary.w1_w1_kernel_at_gp, 0.0);
    set_constant(row(boundary.w1_w1_kernel_at_gp, RowMajTrans2D::xx), -NDParameters::alpha / 3.0 * tau);
    set_constant(row(boundary.w1_w1_kernel_at_gp, RowMajTrans2D::yy), -NDParameters::alpha / 3.0 * tau);

    set_constant(boundary.w1_w1_hat_kernel_at_gp, 0.0);
    set_constant(row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::xx), NDParameters::alpha / 3.0 * tau);
    set_constant(row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::yy), NDParameters::alpha / 3.0 * tau);
    row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::xx) +=
        NDParameters::alpha / 2.0 * vec_cw_mult(vec_cw_mult(bx, nx), h_hat);
    row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::xy) +=
        NDParameters::alpha / 2.0 * vec_cw_mult(vec_cw_mult(by, nx), h_hat);
    row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::yx) +=
        NDParameters::alpha / 2.0 * vec_cw_mult(vec_cw_mult(bx, ny), h_hat);
    row(boundary.w1_w1_hat_kernel_at_gp, RowMajTrans2D::yy) +=
        NDParameters::alpha / 2.0 * vec_cw_mult(vec_cw_mult(by, ny), h_hat);

    row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::x) = -vec_cw_div(nx, h_hat);
    row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::y) = -vec_cw_div(ny, h_hat);

    for (uint dof_i = 0; dof_i < edge_dbound.boundary.data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_dbound.boundary.data.get_ndof(); ++dof_j) {
            submatrix(internal.w1_w1,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) +=
                reshape<double, GN::n_dimensions>(
                    edge_dbound.boundary.IntegrationPhiPhi(dof_i, dof_j, boundary.w1_w1_kernel_at_gp));
        }
    }

    for (uint dof_i = 0; dof_i < edge_dbound.boundary.data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_dbound.edge_data.get_ndof(); ++dof_j) {
            submatrix(boundary.w1_w1_hat,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_dbound.IntegrationPhiLambda(dof_i, dof_j, boundary.w1_w1_hat_kernel_at_gp));
            boundary.w2_w1_hat(dof_i, GN::n_dimensions * dof_j + GlobalCoord::x) =
                edge_dbound.IntegrationPhiLambda(dof_i, dof_j, row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::x));
            boundary.w2_w1_hat(dof_i, GN::n_dimensions * dof_j + GlobalCoord::y) =
                edge_dbound.IntegrationPhiLambda(dof_i, dof_j, row(boundary.w2_w1_hat_kernel_at_gp, GlobalCoord::y));
        }
    }
}

template <typename EdgeDistributedType>
void Problem::global_dc_edge_distributed_kernel(const ESSPRKStepper& stepper, EdgeDistributedType& edge_dbound) {
    auto& edge_internal = edge_dbound.edge_data.edge_internal;
    auto& boundary      = edge_dbound.boundary.data.boundary[edge_dbound.boundary.bound_id];

    edge_dbound.boundary.boundary_condition.ComputeGlobalKernelsDC(edge_dbound);

    for (uint dof_i = 0; dof_i < edge_dbound.edge_data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_dbound.edge_data.get_ndof(); ++dof_j) {
            submatrix(edge_internal.w1_hat_w1_hat,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_dbound.IntegrationLambdaLambda(dof_i, dof_j, edge_internal.w1_hat_w1_hat_kernel_at_gp));
        }
    }

    for (uint dof_i = 0; dof_i < edge_dbound.edge_data.get_ndof(); ++dof_i) {
        for (uint dof_j = 0; dof_j < edge_dbound.boundary.data.get_ndof(); ++dof_j) {
            submatrix(boundary.w1_hat_w1,
                      GN::n_dimensions * dof_i,
                      GN::n_dimensions * dof_j,
                      GN::n_dimensions,
                      GN::n_dimensions) =
                reshape<double, GN::n_dimensions>(
                    edge_dbound.IntegrationPhiLambda(dof_j, dof_i, boundary.w1_hat_w1_kernel_at_gp));
            boundary.w1_hat_w2(GN::n_dimensions * dof_i + GlobalCoord::x, dof_j) =
                edge_dbound.IntegrationPhiLambda(dof_j, dof_i, row(boundary.w1_hat_w2_kernel_at_gp, GlobalCoord::x));
            boundary.w1_hat_w2(GN::n_dimensions * dof_i + GlobalCoord::y, dof_j) =
                edge_dbound.IntegrationPhiLambda(dof_j, dof_i, row(boundary.w1_hat_w2_kernel_at_gp, GlobalCoord::y));
        }
    }
}
}
}

#endif