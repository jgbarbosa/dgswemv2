#ifndef EHDG_SWE_BC_FUNCTION_HPP
#define EHDG_SWE_BC_FUNCTION_HPP

#include "problem/SWE/problem_jacobian/swe_jacobian.hpp"

namespace SWE {
namespace EHDG {
namespace BC {
class Function {
  private:
    HybMatrix<double, SWE::n_variables * SWE::n_variables> Aplus;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAplus_dze;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAplus_dqx;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAplus_dqy;

    HybMatrix<double, SWE::n_variables * SWE::n_variables> Aminus;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAminus_dze;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAminus_dqx;
    HybMatrix<double, SWE::n_variables * SWE::n_variables> dAminus_dqy;

  public:
    template <typename BoundaryType>
    void Initialize(BoundaryType& bound);

    template <typename StepperType, typename EdgeBoundaryType>
    void ComputeGlobalKernels(const StepperType& stepper, EdgeBoundaryType& edge_bound);

    template <typename EdgeBoundaryType>
    void ComputeNumericalFlux(EdgeBoundaryType& edge_bound);
};

template <typename BoundaryType>
void Function::Initialize(BoundaryType& bound) {
    uint ngp = bound.data.get_ngp_boundary(bound.bound_id);

    this->Aplus.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAplus_dze.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAplus_dqx.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAplus_dqy.resize(SWE::n_variables * SWE::n_variables, ngp);

    this->Aminus.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAminus_dze.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAminus_dqx.resize(SWE::n_variables * SWE::n_variables, ngp);
    this->dAminus_dqy.resize(SWE::n_variables * SWE::n_variables, ngp);
}

template <typename StepperType, typename EdgeBoundaryType>
void Function::ComputeGlobalKernels(const StepperType& stepper, EdgeBoundaryType& edge_bound) {
    auto& edge_internal = edge_bound.edge_data.edge_internal;

    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    auto& q_hat_at_gp    = edge_internal.q_hat_at_gp;
    auto& aux_hat_at_gp  = edge_internal.aux_hat_at_gp;
    auto& surface_normal = edge_bound.boundary.surface_normal;

    get_Aplus(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->Aplus);
    get_dAplus_dze(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAplus_dze);
    get_dAplus_dqx(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAplus_dqx);
    get_dAplus_dqy(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAplus_dqy);

    get_Aminus(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->Aminus);
    get_dAminus_dze(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAminus_dze);
    get_dAminus_dqx(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAminus_dqx);
    get_dAminus_dqy(q_hat_at_gp, aux_hat_at_gp, surface_normal, this->dAminus_dqy);

    double t = stepper.GetTimeAtCurrentStage();

    HybMatrix<double, SWE::n_variables> q_ex = edge_bound.boundary.ComputeFgp([t](Point<2>& pt) {
        double ze = 0.0;
        double qx = 0.0;
        double qy = 0.0;

        if (t <= 3.0) {
            ze = cos(PI * t) - 1.0;
        } else {
            ze = -2.0;
        }

        // StatVector<double, SWE::n_variables> q{ze, qx, qy};
        StatVector<double, SWE::n_variables> q(SWE::ic_q(t, pt));

        return q;
    });

    using AMatrix = StatMatrix<double, SWE::n_variables, SWE::n_variables>;

    for (uint gp = 0; gp < edge_bound.edge_data.get_ngp(); ++gp) {
        AMatrix A_plus      = reshape<double, SWE::n_variables>(column(Aplus, gp));
        AMatrix dA_plus_dze = reshape<double, SWE::n_variables>(column(dAplus_dze, gp));
        AMatrix dA_plus_dqx = reshape<double, SWE::n_variables>(column(dAplus_dqx, gp));
        AMatrix dA_plus_dqy = reshape<double, SWE::n_variables>(column(dAplus_dqy, gp));

        AMatrix A_minus      = reshape<double, SWE::n_variables>(column(Aminus, gp));
        AMatrix dA_minus_dze = reshape<double, SWE::n_variables>(column(dAminus_dze, gp));
        AMatrix dA_minus_dqx = reshape<double, SWE::n_variables>(column(dAminus_dqx, gp));
        AMatrix dA_minus_dqy = reshape<double, SWE::n_variables>(column(dAminus_dqy, gp));

        auto q     = column(boundary.q_at_gp, gp);
        auto q_hat = column(edge_internal.q_hat_at_gp, gp);
        auto q_inf = column(q_ex, gp);

        AMatrix dB_dq_hat = A_minus - A_plus;

        column(dB_dq_hat, SWE::Variables::ze) += dA_plus_dze * (q - q_hat) - dA_minus_dze * (q_inf - q_hat);
        column(dB_dq_hat, SWE::Variables::qx) += dA_plus_dqx * (q - q_hat) - dA_minus_dqx * (q_inf - q_hat);
        column(dB_dq_hat, SWE::Variables::qy) += dA_plus_dqy * (q - q_hat) - dA_minus_dqy * (q_inf - q_hat);

        column(edge_internal.delta_hat_global_kernel_at_gp, gp) = flatten<double>(dB_dq_hat);
        column(edge_internal.rhs_global_kernel_at_gp, gp)       = A_plus * (q - q_hat) - A_minus * (q_inf - q_hat);
    }
}

template <typename EdgeBoundaryType>
void Function::ComputeNumericalFlux(EdgeBoundaryType& edge_bound) {
    auto& boundary = edge_bound.boundary.data.boundary[edge_bound.boundary.bound_id];

    boundary.F_hat_at_gp = boundary.Fn_at_gp;

    add_F_hat_tau_terms_bound_LF(edge_bound);
}
}
}
}

#endif
