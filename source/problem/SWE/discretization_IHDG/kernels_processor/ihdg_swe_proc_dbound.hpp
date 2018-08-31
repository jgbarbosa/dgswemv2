#ifndef IHDG_SWE_PROC_DBOUND_HPP
#define IHDG_SWE_PROC_DBOUND_HPP

namespace SWE {
namespace IHDG {
template <typename DistributedBoundaryType>
void Problem::local_distributed_boundary_kernel(const RKStepper& stepper, DistributedBoundaryType& dbound) {
    const uint stage = stepper.GetStage();

    auto& state    = dbound.data.state[stage + 1];
    auto& boundary = dbound.data.boundary[dbound.bound_id];

    boundary.q_at_gp = dbound.ComputeUgp(state.q);

    row(boundary.aux_at_gp, SWE::Auxiliaries::h) =
        row(boundary.q_at_gp, SWE::Variables::ze) + row(boundary.aux_at_gp, SWE::Auxiliaries::bath);

    /* Compute fluxes at boundary state */
    auto nx = row(dbound.surface_normal, GlobalCoord::x);
    auto ny = row(dbound.surface_normal, GlobalCoord::y);

    auto u = cwise_division(row(boundary.q_at_gp, SWE::Variables::qx), row(boundary.aux_at_gp, SWE::Auxiliaries::h));
    auto v = cwise_division(row(boundary.q_at_gp, SWE::Variables::qy), row(boundary.aux_at_gp, SWE::Auxiliaries::h));

    auto uuh = cwise_multiplication(u, row(boundary.q_at_gp, SWE::Variables::qx));
    auto vvh = cwise_multiplication(v, row(boundary.q_at_gp, SWE::Variables::qy));
    auto uvh = cwise_multiplication(u, row(boundary.q_at_gp, SWE::Variables::qy));
    auto pe  = Global::g * (0.5 * cwise_multiplication(row(boundary.q_at_gp, SWE::Variables::ze),
                                                      row(boundary.q_at_gp, SWE::Variables::ze)) +
                           cwise_multiplication(row(boundary.q_at_gp, SWE::Variables::ze),
                                                row(boundary.aux_at_gp, SWE::Auxiliaries::bath)));

    // Fn terms
    row(boundary.F_hat_at_gp, SWE::Variables::ze) =
        cwise_multiplication(row(boundary.q_at_gp, SWE::Variables::qx), nx) +
        cwise_multiplication(row(boundary.q_at_gp, SWE::Variables::qy), ny);
    row(boundary.F_hat_at_gp, SWE::Variables::qx) = cwise_multiplication(uuh + pe, nx) + cwise_multiplication(uvh, ny);
    row(boundary.F_hat_at_gp, SWE::Variables::qy) = cwise_multiplication(uvh, nx) + cwise_multiplication(vvh + pe, ny);

    // dFn/dq terms
    set_constant(row(boundary.dF_hat_dq_at_gp, JacobianVariables::ze_ze), 0.0);
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::ze_qx) = nx;
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::ze_qy) = ny;

    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qx_ze) =
        cwise_multiplication(-cwise_multiplication(u, u) + Global::g * row(boundary.aux_at_gp, SWE::Auxiliaries::h),
                             nx) -
        cwise_multiplication(cwise_multiplication(u, v), ny);
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qx_qx) =
        2.0 * cwise_multiplication(u, nx) + cwise_multiplication(v, ny);
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qx_qy) = cwise_multiplication(u, ny);

    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qy_ze) =
        -cwise_multiplication(cwise_multiplication(u, v), nx) +
        cwise_multiplication(-cwise_multiplication(v, v) + Global::g * row(boundary.aux_at_gp, SWE::Auxiliaries::h),
                             ny);
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qy_qx) = cwise_multiplication(v, nx);
    row(boundary.dF_hat_dq_at_gp, JacobianVariables::qy_qy) =
        cwise_multiplication(u, nx) + 2.0 * cwise_multiplication(v, ny);
}
}
}

#endif
