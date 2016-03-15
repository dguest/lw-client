#ifndef NN_LAYER_CONFIG_HH
#define NN_LAYER_CONFIG_HH

// Layer Configiruation for Lightweight Tagger
//
// The structures below are used to initalize
// `LightweightNeuralNetwork` and the simpler `Stack`.
//
// Author: Dan Guest <dguest@cern.ch>

#include <vector>
#include <string>
#include <ostream>


namespace lwt {
  enum class Activation {LINEAR, SIGMOID, RECTIFIED, SOFTMAX, TANH};
  struct LayerConfig
  {
    std::vector<double> weights;
    std::vector<double> bias;
    Activation activation;
  };

  struct Input
  {
    std::string name;
    double offset;
    double scale;
  };
}

#endif
