Lightweight Trained Neural Network
==================================

[![Build Status][build-img]][build-link] [![Scan Status][scan-img]][scan-link]
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.597221.svg)](https://doi.org/10.5281/zenodo.597221)

[build-img]: https://travis-ci.org/lwtnn/lwtnn.svg?branch=master
[build-link]: https://travis-ci.org/lwtnn/lwtnn
[scan-img]: https://scan.coverity.com/projects/9285/badge.svg
[scan-link]: https://scan.coverity.com/projects/lwtnn-lwtnn

What is this?
-------------

The code comes in two parts:

 1. A set of scripts to convert saved neural networks to a JSON format
 2. A set of classes can be used for inference in a C++ framework

The main design principles are:

 - **Minimal dependencies:** The core class should only depend on
   C++11 and [Eigen][eg]. The JSON parser to read in NNs also requires
   boost [PropertyTree][pt].
 - **Minimal API:** The C++ classes have no non-const methods. Once
   they have been constructed they can be used for inference and
   nothing else.
 - **Easy to extend:** Should cover 95% of deep network architectures we
   would realistically consider.
 - **Hard to break:** The NN constructor checks the serialized NN for
   consistency and fails loudly if anything goes wrong.

We also include converters from several popular formats to the `lwtnn`
JSON format. Currently the following formats are supported:
 - [AGILEPack][ap]
 - [Keras][kr] (most popular, see below)

[eg]: http://eigen.tuxfamily.org
[pt]: http://www.boost.org/doc/libs/1_59_0/doc/html/property_tree.html
[ap]: https://github.com/lukedeo/AGILEPack
[kr]: http://keras.io/

How do I use it?
----------------

Applying a saved neural network within C++ code is easy:

```C++
// Include several headers. See the files for more documentation.
// First include the class that does the computation
#include "lwtnn/LightweightGraph.hh"
// Then include the json parsing functions
#include "lwtnn/parse_json.hh"

...

// get your saved JSON file as an std::istream object
std::ifstream input("path-to-file.json");
// build the graph
LightweightGraph graph(parse_json_graph(input));

...

// fill a map of input nodes
std::map<std::string, std::map<std::string, double> > inputs;
inputs["input_node"] = {{"value", value}, {"value_2", value_2}};
inputs["another_input_node"] = {{"another_value", another_value}};
// compute the output values
std::map<std::string, double> outputs = graph.compute(inputs);
```

All inputs and outputs are stored in `std::map`s to prevent bugs with
incorrectly ordered inputs and outputs. The strings used as keys in
the map are specified by the network configuration.

If you find this interface cumbersome or slow, you're free to work
with the underlying `Graph` class's `Eigen::VectorXd` interface.

### Supported Layers ###

In particular, the following layers are supported as implemented in the
Keras sequential and functional models:

|               | K sequential | K functional  |
|---------------|--------------|---------------|
| Dense         |  yes         |  yes          |
| Normalization | See Note 1   | See Note 1    |
| Maxout        |  yes         |  yes          |
| Highway       |  yes         |  yes          |
| LSTM          |  yes         |  yes          |
| GRU           |  yes         |  yes          |
| Embedding     | sorta        | [issue][ghie] |
| Concatenate   |  no          |  yes          |

**Note 1:** Normalization layers (i.e. Batch Normalization) are only
supported for Keras 1.0.8 and higher.

[ghie]: https://github.com/lwtnn/lwtnn/issues/39
[ghkeras2]: https://github.com/lwtnn/lwtnn/issues/40

#### Supported Activation Functions ####

| Function      | Implemented? |
|---------------|--------------|
| ReLU          | Yes          |
| Sigmoid       | Yes          |
| Hard Sigmoid  | Yes          |
| Tanh          | Yes          |
| Softmax       | Yes          |
| ELU           | No           |

The converter scripts can be found in `converters/`. Run them with
`-h` for more information.


Quick Start
-----------

After running `make`, just run `./tests/test-ipmp.sh`. If nothing
goes wrong you should see something like:

```
all outputs within thresholds!
 *** Success! ***
cleaning up
```

There may be some problems if you don't have python 3 or
[`h5py`][h5py]. At the very least you should be able to run
`./bin/lwtnn-test-graph`, which will spit out a few numbers (and
nothing else).

[h5py]: http://docs.h5py.org/en/latest/build.html#source-installation-on-linux-and-os-x

#### Cool, what the hell did that do? ####

Take a look inside the test script, it does a few things:

 - Runs `./converters/keras2json.py`. This takes a [Keras][kr]
   output and write a JSON file to standard out.
 - Sends the output to `./bin/lwtag-test-rnn`. This will
   construct a NN from the resulting JSON and run a single test
   pattern.

Of course this isn't very useful, to do more you have to understand...

Code Organization
-----------------

Code is organized into a low and high level interface. The main files are:

 - `Stack` and `Graph` files: contain the low level NN classes, and
   any code that relies on Eigen.
 - `LightweightNeuralNetwork` and `LightweightGraph` files: contain
   the high-level wrappers, which implement STL (rather than Eigen)
   interfaces. To speed up compilation the header file can be included
   without including Eigen.
 - `NNLayerConfig` and `lightweight_network_config` headers: define
   the structures to initialize the (low and high level) networks.
 - `parse_json` files: contain functions to build the config
   structures from JSON.

There are a few other less important files that contain debugging code
and utilities.

#### The High Level Interface ####

Open `include/LightweightNeuralNetwork.hh` and find the class
declaration for `LightweightNeuralNetwork`. The constructor takes
three arguments:

 - A vector of `Input`s: these structures give the variable `name`,
   `offset`, and `scale`. Note that these are applied as `v = (input +
   offset) * scale`, so if you're normalizing inputs with some `mean`
   and `standard_deviation`, these are given by `offset = - mean` and
   `scale = 1 / standard_deviation`.
 - A vector of `LayerConfig` structures. See the below section for an
   explanation of this class.
 - A vector of output names.

The constructor should check to make sure everything makes sense
internally. If anything goes wrong it will throw a
`NNConfigurationException`.

After the class is constructed, it has one method, `compute`, which
takes a `map<string, double>` as an input and returns a `map` of named
outputs (of the same type). It's fine to give `compute` a map with
more arguments than the NN requires, but if some argument is _missing_
it will throw an `NNEvaluationException`. All the exceptions inherit
from `LightweightNNException`.

#### The Low Level Interface ####

The `Stack` class is initialized with two parameters: the number of
input parameters, and a `std::vector<LayerConfig>` to specify the
layers. Each `LayerConfig` structure contains:

 - A vector of `weights`. This can be zero-length, in which case no
   matrix is inserted (but the bias and activation layers are).
 - A `bias` vector. Again, it can be zero length for no bias in this
   layer.
 - An `activation` function. Defaults to `LINEAR` (i.e. no activation
   function).

Note that the dimensions of the matrices aren't specified after the
`n_inputs` in the `Stack` constructor, because this should be
constrained by the dimensions of the `weight` vectors. If something
doesn't make sense the constructor should throw an
`NNConfigurationException`.

The `Stack::compute(VectorXd)` method will return a `VectorXd` of
outputs.

Testing an Arbitrary NN
-----------------------

The `lwtnn-test-arbitrary-net` executable takes in a JSON file along
with two text files, one to specify the variable names and another to
give the input values. Run with no arguments to get help.

You can also use `lwtnn-test-keras-arbitrary-net.py` to test the
corresponding model saved in the Keras format.

Recurrent Networks
------------------

Currently we support LSTMs and GRUs in sequential models. The low
level interface is implemented as `RecurrentStack`. See
`lwtnn-test-rnn` for a working example.

Again, the corresponding model in Keras can be tested with
`lwtnn-test-keras-rnn.py`.

Graphs
------

Like the feed-forward models, the graphs (functional models in Keras)
are broken into low level and high level interfaces. See
`lwtnn-test-graph.cxx` and `lwtnn-test-lightweight-graph.cxx` for a
working example.

Have problems?
--------------

If you find a bug in this code, or have any ideas, criticisms,
etc, please email me at `dguest@cern.ch`.
