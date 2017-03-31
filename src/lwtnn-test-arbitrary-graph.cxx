#include "lwtnn/LightweightGraph.hh"
#include "lwtnn/parse_json.hh"

#include "test_utilities.hh"

#include <iostream>
#include <fstream>
#include <cassert>

namespace {
  lwt::LightweightGraph::SeqNodeMap get_sequences(
    const std::vector<lwt::InputNodeConfig>& config);

  int run_on_generated(const lwt::GraphConfig& config);
}


void usage(const std::string& name) {
  std::cout << "usage: " << name << " <nn config>\n"
    "\n"
    "The <nn config> file should be generated by one of the scripts in\n"
    "`converters/`.\n"
    "\n";
}

int main(int argc, char* argv[]) {
  if (argc > 2 || argc < 2) {
    usage(argv[0]);
    exit(1);
  }
  // Read in the configuration.
  std::string in_file_name(argv[1]);
  std::ifstream in_file(in_file_name);
  auto config = lwt::parse_json_graph(in_file);

  run_on_generated(config);
  return 0;
}
namespace {
  int run_on_generated(const lwt::GraphConfig& config) {
    using namespace lwt;
    assert(config.outputs.size() > 0);
    lwt::LightweightGraph tagger(config, config.outputs.begin()->first);
    std::map<std::string, std::map<std::string, double> > in_nodes;

    for (const auto& input: config.inputs) {
      const size_t total_inputs = input.variables.size();
      std::map<std::string, double> in_vals;
      for (size_t nnn = 0; nnn < total_inputs; nnn++) {
        const auto& var = input.variables.at(nnn);
        double ramp_val = ramp(var, nnn, total_inputs);
        in_vals[var.name] = ramp_val;
      }
      in_nodes[input.name] = in_vals;
    }
    LightweightGraph::SeqNodeMap seq = get_sequences(config.input_sequences);
    for (const auto& output: config.outputs) {
      auto out_vals = tagger.compute(in_nodes, seq, output.first);
      std::cout << output.first << ":" << std::endl;
      for (const auto& out: out_vals) {
        std::cout << out.first << " " << out.second << std::endl;
      }
    }
    return 0;
  }

  lwt::LightweightGraph::SeqNodeMap get_sequences(
    const std::vector<lwt::InputNodeConfig>& config) {
    lwt::LightweightGraph::SeqNodeMap nodes;
    for (const auto& input: config) {
      nodes[input.name] = get_values_vec(input.variables, 20);
    }
    return nodes;
  }
}