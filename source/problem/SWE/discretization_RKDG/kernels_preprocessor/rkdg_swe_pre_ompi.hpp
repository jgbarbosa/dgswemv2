#ifndef RKDG_SWE_PRE_OMPI_HPP
#define RKDG_SWE_PRE_OMPI_HPP

#include "problem/SWE/problem_preprocessor/swe_pre_init_data.hpp"

namespace SWE {
namespace RKDG {
template <typename OMPISimType>
void Problem::preprocessor_ompi(OMPISimType* sim, uint begin_sim_id, uint end_sim_id) {
    auto& sim_units = sim->sim_units;

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        initialize_data_parallel_pre_send(
            sim_units[su_id]->discretization.mesh, sim_units[su_id]->problem_input, CommTypes::baryctr_coord);
    }

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->communicator.ReceiveAll(CommTypes::baryctr_coord, sim->stepper.GetTimestamp());
    }

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->communicator.SendAll(CommTypes::baryctr_coord, sim->stepper.GetTimestamp());
    }

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->communicator.WaitAllReceives(CommTypes::baryctr_coord, sim->stepper.GetTimestamp());

        initialize_data_parallel_post_receive(sim_units[su_id]->discretization.mesh, CommTypes::baryctr_coord);
    }

    for (uint su_id = begin_sim_id; su_id < end_sim_id; ++su_id) {
        sim_units[su_id]->communicator.WaitAllSends(CommTypes::baryctr_coord, sim->stepper.GetTimestamp());
    }
}
}
}

#endif