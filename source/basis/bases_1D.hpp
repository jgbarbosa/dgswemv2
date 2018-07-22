#ifndef BASES_1D_HPP
#define BASES_1D_HPP

#include "general_definitions.hpp"
#include "basis_polynomials.hpp"

namespace Basis {
class Legendre_1D : Basis<1> {
  public:
    DynMatrix<double> GetPhi(const uint p, const DynVector<Point<1>>& points);
    StatVector<DynMatrix<double>, 1> GetDPhi(const uint p, const DynVector<Point<1>>& points);

    DynMatrix<double> GetMinv(const uint p);

    template <typename InputArrayType>
    decltype(auto) ProjectBasisToLinear(const InputArrayType& u);
    template <typename InputArrayType>
    decltype(auto) ProjectLinearToBasis(const uint ndof, const InputArrayType& u_lin);
};
}

#endif