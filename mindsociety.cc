#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <chrono>

using namespace std;

const int kDim = 100;
const float kFlipProbability = 0.3;
const float kMutateProbability = 0.05;
const int kPopulation = 100;
const int kGenerations = 50;
const int kRounds = 10;

string vec2str(const vector<bool>& v) {
  string s;

  for (int i = 0; i < v.size(); ++i) {
    s += v[i] ? "1" : "0";
    s += i < v.size() - 1 ? ", " : "";
  }
  
  return s;
}

// Organisms that will be evolving.
struct Organism {
  vector<bool> model1;
  vector<bool> model2;
  float fitness;
};

// True model that organisms will be evolving to match.
struct TrueModel {
  vector<bool> true_model1;
  vector<bool> true_model2;
  vector<bool> noisy_model1;
  vector<bool> noisy_model2;

  string toString() {
    string s;
    s += "TModel1:\n  " + vec2str(this->true_model1) + "\n";
    s += "TModel2:\n  " + vec2str(this->true_model2) + "\n";
    s += "NModel1:\n  " + vec2str(this->noisy_model1) + "\n";
    s += "NModel2:\n  " + vec2str(this->noisy_model2) + "\n";        
    return s;
  }  
};

struct Challenge {
  float reward1;
  float reward2;
};

// Samples the sampled models (for each generation) from the overall true
// models.
void sampleNoisyModels(TrueModel& m) {
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  bernoulli_distribution dist(kFlipProbability);  

  for (int i = 0; i < kDim; ++i) {
    const bool v1 = m.true_model1[i];
    const bool v2 = m.true_model2[i];
    if (dist(gen)) {
      m.noisy_model1[i] = !v1;
    }
    if (dist(gen)) {
      m.noisy_model2[i] = !v2;
    }
  }
}

vector<bool> initializeModel() {
  vector<bool> m(kDim);
  
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
  bernoulli_distribution dist(0.5);

  for (int i = 0; i < kDim; ++i) {
    m[i] = dist(gen);
  }

  return m;
}

TrueModel initializeTrueModel() {
  TrueModel m;
  m.true_model1 = initializeModel();
  m.true_model2 = initializeModel();
  m.noisy_model1 = initializeModel();
  m.noisy_model2 = initializeModel();  

  sampleNoisyModels(m);

  return m;
}

Organism initializeOrganism() {
  Organism o;
  o.model1 = initializeModel();
  o.model2 = initializeModel();
  o.fitness = 0.0;
  return o;
}

void mutate(Organism& o) {
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  bernoulli_distribution dist(kMutateProbability);  

  for (int i = 0; i < kDim; ++i) {
    const bool v1 = o.model1[i];
    const bool v2 = o.model2[i];
    if (dist(gen)) {
      o.model1[i] = !v1;
    }
    if (dist(gen)) {
      o.model2[i] = !v2;      
    }
  }
}

// Pick one of the multiple tasks from a challenge.
int pickTask(const Organism& o, const Challenge& c) {
  // TODO
  return 0;
}

// Performs task then returns the (partial) reward based on performance.
//
// The task of the organism is to match its model (i.e. bit string) to the
// "ideal" model (i.e. another bit string). The reward is the percentage of bits
// that match times the max reward.
pair<float, float> doTask(const vector<bool>& model, const vector<bool>& noisy_model, float reward) {
  int num_correct = 0;
  for (int i = 0; i < model.size(); ++i) {
    num_correct += model[i] == noisy_model[i];
  }
  cout << num_correct << endl;

  float perc_correct = static_cast<float>(num_correct) / kDim;
  return pair<float, float>(reward * perc_correct, perc_correct);
}

pair<float, float> doChallenge(const Organism& o, const Challenge& c, const TrueModel& m) {
  int task = pickTask(o, c);

  return task == 0
    ? doTask(o.model1, m.noisy_model1, c.reward1)
    : doTask(o.model2, m.noisy_model2, c.reward2);
}

Challenge sampleChallenge() {
  // TODO
  return (struct Challenge){ .reward1 = 2, .reward2 = 1 };
}

// Get new population according to tournament selection.
vector<Organism> getNewPopulation(const vector<Organism>& population, const TrueModel& m) {
  vector<Organism> new_population;

  // Shuffle a list of indices to get pairs to compare in tournament.
  vector<int> indices(kPopulation);
  iota(indices.begin(), indices.end(), 0);
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());    
  shuffle(indices.begin(), indices.end(), gen);

  // Compare each adjacent pair. The winner of each pair (i.e. one with higher
  // fitness will be able to create two mutated offspring for the next
  // generation).
  float max_fitness = 0;
  const Organism* max_o = nullptr;
  for (int i = 0; i < indices.size() - 1; ++i) {
    const Organism& o1 = population[indices[i]];
    const Organism& o2 = population[indices[i + 1]];

    if (o1.fitness > max_fitness) {
      max_fitness = o1.fitness;
      max_o = &o1;
    } else if (o2.fitness > max_fitness) {
      max_fitness = o2.fitness;
      max_o = &o2;
    }

    Organism new_o1, new_o2;    
    new_o1 = o1.fitness > o2.fitness ? o1 : o2;
    new_o2 = o1.fitness > o2.fitness ? o1 : o2;    
    mutate(new_o1); mutate(new_o2);
    new_o1.fitness = 0; new_o2.fitness = 0;
    new_population.push_back(new_o1); new_population.push_back(new_o2);
  }

  // Print best organism so far (for debug reasons).
  cout << "avg perc correct: " << max_o->avg_perc_correct << endl;  
  cout << "true model1:" << endl;
  cout << "  " << vec2str(m.true_model1) << endl;
  cout << "best organism model1:" << endl;
  cout << "  " << vec2str(max_o->model1) << endl;
  cout << "true model2:" << endl;
  cout << "  " << vec2str(m.true_model2) << endl;
  cout << "best organism model2:" << endl;
  cout << "  " << vec2str(max_o->model2) << endl;
  cout << endl << endl;

  return new_population;
}

// Run a generation of the evolutionary algorithm. This consists of:
//   (1) Adding noise to the true models.
//   (2) Each organism in the population doing kRounds of challenges to get
//       their total fitness.
//   (3) Picking the best performing organisms to be able to reproduce
//       (asexually, i.e. just with mutation, no crossover) for the next
//       generation.
vector<Organism> runGeneration(vector<Organism>& population, TrueModel& m) {
  sampleNoisyModels(m);

  for (int i = 0; i < kRounds; ++i) {
    Challenge c = sampleChallenge();
    for (int j = 0; j < population.size(); ++j) {
      Organism& o = population[j];
      const pair<float, float>& reward = doChallenge(o, c, m);
      o.fitness += reward.first;
      o.avg_perc_correct += reward.second / kRounds;
    }
  }

  return getNewPopulation(population, m);
}


int main() {
  TrueModel m = initializeTrueModel();
  cout << m.toString() << endl;

  vector<Organism> population(kPopulation);
  for (int i = 0; i < kPopulation; ++i) {
    population[i] = initializeOrganism();
  }

  for (int i = 0; i < kGenerations; ++i) {
    population = runGeneration(population, m);
  }
}
