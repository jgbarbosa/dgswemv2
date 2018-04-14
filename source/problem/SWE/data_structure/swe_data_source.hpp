#ifndef SWE_DATA_SOURCE_HPP
#define SWE_DATA_SOURCE_HPP

#include "../../../general_definitions.hpp"

namespace SWE {
struct Source {
    Source() = default;
    Source(const uint nnode)
        : parsed_meteo_data(nnode),
          tau_s({std::vector<double>(nnode), std::vector<double>(nnode)}),
          p_atm(nnode),
          tidal_pot(nnode),
          manning_n(nnode) {}

    double coriolis_f = 0.0;

    bool   manning        = false;
    double g_manning_n_sq = 0.0;

    std::vector<std::vector<double>*>  parsed_meteo_data;
    std::array<std::vector<double>, 2> tau_s;
    std::vector<double>                p_atm;

    std::vector<double> tidal_pot;
    std::vector<double> manning_n;
};
}

#endif