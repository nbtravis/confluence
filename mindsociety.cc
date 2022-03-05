#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

const int kDim = 2;
const int kTrueModelMax = 5;
const int kTrueModelMin = -5;
const int kTrueModelSamplingRadius = 1;

string vec2str(const vector<float>& v) {
  string s;

  for (int i = 0; i < v.size(); ++i) {
    stringstream stream;
    stream << fixed << setprecision(2) << v[i];
    s += stream.str();
    if (i < v.size() - 1) {
      s += ", ";
    }
  }
  
  return s;
}

// Organisms that will be evolving.
struct Organism {
  vector<float> model1;
  vector<float> model2;
  float fitness;
};

// True model that organisms will be evolving to match.
struct TrueModel {
  vector<float> true_model1;
  vector<float> true_model2;
  vector<float> sampled_model1;
  vector<float> sampled_model2;

  string toString() {
    string s;
    s += "TModel1:\n  " + vec2str(this->true_model1) + "\n";
    s += "TModel2:\n  " + vec2str(this->true_model2) + "\n";
    s += "SModel1:\n  " + vec2str(this->sampled_model1) + "\n";
    s += "SModel2:\n  " + vec2str(this->sampled_model2) + "\n";        
    return s;
  }  
};


// Organism createP() {
//   P p = { vector<float>(), vector<float>(), 0.0 };
//   for (int i = 0; i < kDim; ++i) {
//     p.model1.push_back(0.0);
//   }
// }


// Samples the sampled models (for each generation) from the overall true
// models.
void sampleModels(TrueModel& m) {
  std::default_random_engine gen;

  for (int i = 0; i < kDim; ++i) {
    const float v1 = m.true_model1[i];
    const float v2 = m.true_model2[i];
    std::uniform_real_distribution<float> dist1(
      v1 - kTrueModelSamplingRadius, v1 + kTrueModelSamplingRadius);
    std::uniform_real_distribution<float> dist2(
      v2 - kTrueModelSamplingRadius, v1 + kTrueModelSamplingRadius);
    m.sampled_model1[i] = dist1(gen);
    m.sampled_model2[i] = dist2(gen);
  }
}


// Initializes the true models that the organisms will be trying to evolve to
// match.
TrueModel initializeTrueModel() {
  TrueModel m = {
    vector<float>(kDim), vector<float>(kDim),
    vector<float>(kDim), vector<float>(kDim)
  };
  
  std::default_random_engine gen;
  std::uniform_real_distribution<float> dist(kTrueModelMin, kTrueModelMax);

  for (int i = 0; i < kDim; ++i) {
    m.true_model1[i] = dist(gen);
    m.true_model2[i] = dist(gen);
  }

  sampleModels(m);

  return m;
}


int main() {
  TrueModel m = initializeTrueModel();
  cout << m.toString() << endl; 
}
