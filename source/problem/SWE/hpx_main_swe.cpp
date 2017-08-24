#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>

#include "general_definitions.hpp"

#include "simulation/stepper.hpp"
#include "swe_problem.hpp"
#include "swe_kernels.hpp"
#include "simulation/hpx_simulation.hpp"

#include "preprocessor/input_parameters.hpp"
#include "utilities/file_exists.hpp"

void local_main(std::string, std::string);
HPX_PLAIN_ACTION(local_main, local_main_action);

hpx::future<void> solve_mesh(std::string, uint);
HPX_PLAIN_ACTION(solve_mesh, solve_mesh_action);

using hpx_simulation_swe = HPXSimulation<SWE::Problem>;
using hpx_simulation_swe_component = hpx::components::simple_component<HPXSimulation<SWE::Problem>>;
HPX_REGISTER_COMPONENT(hpx_simulation_swe_component, hpx_simulation_swe);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage\n"
                  << "    /path/to/DG_HYPER_SWE input_file\n";
        return 1;
    } else {
        return hpx::init(argc, argv);
    }
}

int hpx_main(int argc, char* argv[]) {
    std::string input_file = std::string(argv[1]);
    InputParameters inputs( input_file );

    const std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    std::vector<hpx::future<void>> futures;
    futures.reserve(localities.size());

    auto t1 = std::chrono::high_resolution_clock::now();
    for (hpx::naming::id_type const& node : localities) {
        futures.push_back(hpx::async<local_main_action>(node, input_file, inputs.mesh_file_name));
    }

    hpx::wait_all(futures);
    auto t2 = std::chrono::high_resolution_clock::now();

    std::cout << "Time Elapsed (in us): " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
              << "\n";

    return hpx::finalize();  // Handles HPX shutdown
}

void local_main(std::string input_string, std::string mesh_file_name) {
    const hpx::naming::id_type here = hpx::find_here();

    uint locality_id = hpx::get_locality_id();
    std::vector<hpx::future<void>> futures;

    std::string mesh_file_prefix = mesh_file_name;
    mesh_file_prefix.erase(mesh_file_prefix.size() - 3);
    mesh_file_prefix += '_' + std::to_string(locality_id) + '_';
    hpx::cout << mesh_file_prefix << '\n';

    uint sbmsh_id = 0;
    while ( Utilities::file_exists( mesh_file_prefix + std::to_string(sbmsh_id) + ".14") ) {
        futures.push_back(hpx::async<solve_mesh_action>(here, input_string, sbmsh_id));
        ++sbmsh_id;
    }

    return hpx::when_all(futures);
}

hpx::future<void> solve_mesh(std::string input_string, uint sbmsh_id) {
    try {


/*
        hpx::id_type here = hpx::find_here();

        hpx::future<hpx::id_type> simulation_id =
            hpx::new_<hpx_simulation_swe_component>(here, input_string, hpx::get_locality_id(), thread);

        HPXSimulationClient<SWE::Problem> simulation_client(std::move(simulation_id));

        //     HPXSimulation<SWE::Problem> simulation_client(input_string, hpx::get_locality_id(), thread);

        return simulation_client.Run(1800.);*/
        return hpx::make_ready_future();
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught\n";
        std::cerr << "  " << e.what() << std::endl;

        return hpx::make_ready_future();
    }
}
