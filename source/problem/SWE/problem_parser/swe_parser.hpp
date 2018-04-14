#ifndef SWE_PARSER_HPP
#define SWE_PARSER_HPP

#include "../../../preprocessor/input_parameters.hpp"
#include "../../../simulation/stepper.hpp"

#include "../swe_definitions.hpp"

namespace SWE {
class Parser {
  private:
    bool parsing_input;

    SWE::MeteoForcingType                               meteo_forcing_type;
    uint                                                meteo_parse_frequency;
    std::string                                         meteo_data_file;
    std::map<uint, std::map<uint, std::vector<double>>> node_meteo_data_step;
    std::map<uint, std::vector<double>>                 node_meteo_data_interp;

  public:
    Parser() = default;
    Parser(const InputParameters<SWE::Inputs>& input) {
        this->parsing_input = input.problem_input.parse_input;

        this->meteo_forcing_type = input.problem_input.meteo_forcing.type;
        this->meteo_parse_frequency =
            (uint)std::ceil(input.problem_input.meteo_forcing.frequency / input.stepper_input.dt);
        this->meteo_data_file = input.problem_input.meteo_forcing.meteo_data_file;
    }
    Parser(const InputParameters<SWE::Inputs>& input, const uint locality_id, const uint submesh_id)
        : Parser(input) {}  // this is for partitioned input files

    bool ParsingInput() { return parsing_input; }

    template <typename MeshType>
    void ParseInput(const Stepper& stepper, MeshType& mesh) {
        if (SWE::SourceTerms::meteo_forcing) {
            if (stepper.GetStep() % this->meteo_parse_frequency == 0 && stepper.GetStage() == 0) {
                this->ParseMeteoInput(stepper);
            }

            // Initialize container to store parsed data and store pointers for fast access
            if (stepper.GetStep() == 0 && stepper.GetStage() == 0) {
                mesh.CallForEachElement([this](auto& elt) {
                    std::vector<uint>& node_ID = elt.GetNodeID();

                    for (uint node = 0; node < elt.data.get_nnode(); node++) {
                        if (this->node_meteo_data_interp.find(node_ID[node]) == this->node_meteo_data_interp.end()) {
                            this->node_meteo_data_interp[node_ID[node]] = std::vector<double>(3);
                        }

                        elt.data.source.parsed_meteo_data[node] = &this->node_meteo_data_interp[node_ID[node]];
                    }
                });
            }

            this->InterpolateMeteoData(stepper);

            mesh.CallForEachElement([this](auto& elt) {
                std::vector<uint>& node_ID = elt.GetNodeID();

                for (uint node = 0; node < elt.data.get_nnode(); node++) {
                    elt.data.source.tau_s[GlobalCoord::x][node] = elt.data.source.parsed_meteo_data[node]->at(0);
                    elt.data.source.tau_s[GlobalCoord::y][node] = elt.data.source.parsed_meteo_data[node]->at(1);
                    elt.data.source.p_atm[node]                 = elt.data.source.parsed_meteo_data[node]->at(2);
                }
            });
        }
    }

  private:
    void ParseMeteoInput(const Stepper& stepper);
    void InterpolateMeteoData(const Stepper& stepper);
};
}

#endif