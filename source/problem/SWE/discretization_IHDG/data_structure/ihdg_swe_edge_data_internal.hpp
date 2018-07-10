#ifndef IHDG_SWE_EDGE_DATA_INTERNAL_HPP
#define IHDG_SWE_EDGE_DATA_INTERNAL_HPP

#include "general_definitions.hpp"

namespace SWE {
namespace IHDG {
struct EdgeInternal {
    EdgeInternal() = default;
    EdgeInternal(const uint ngp) : q_hat_at_gp(ngp), aux_hat_at_gp(ngp), q_init_at_gp(ngp) {}

    std::vector<Vector<double, SWE::n_variables>> q_hat_at_gp;
    std::vector<Vector<double, SWE::n_auxiliaries>> aux_hat_at_gp;

    std::vector<Vector<double, SWE::n_variables>> q_init_at_gp;
};
}
}

#endif
