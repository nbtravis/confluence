#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <chrono>
#include <algorithm>

using namespace std;

const int kDim = 100;
const float kMutateProbability = 0.005;
const int kPopulation = 200;
const int kGenerations = 5000;
const int kRounds = 10;
const int kMaxReward = 7;
const int kMinReward = 3;
const int kModel2RewardDiff = 2;

const bool USE_COOPERATION = true;
const bool USE_MIXED_COOPERATION = false;
const bool USE_MATING = true;

// Organisms that will be evolving.
struct Organism {
  vector<bool> model1;
  vector<bool> model2;
  int min_reward_diff;
  float fitness;
  float avg_perc_correct1;
  float avg_perc_correct2;  
};

// True model that organisms will be evolving to match.
struct TrueModel {
  vector<bool> model1;
  vector<bool> model2;
};

struct Challenge {
  float reward1;
  float reward2;
};

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
  m.model1 = initializeModel();
  m.model2 = initializeModel();
  return m;
}

Organism initializeOrganism() {
  Organism o;
  o.model1 = initializeModel();
  o.model2 = initializeModel();  
  o.fitness = 0.0;
  o.avg_perc_correct1 = 0.0;
  o.avg_perc_correct2 = 0.0;

  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  uniform_int_distribution<> dist(-kModel2RewardDiff - 1, kModel2RewardDiff + 1);
  o.min_reward_diff = dist(gen);  
  
  return o;
}

void doCrossover(const Organism& parent1, const Organism& parent2, Organism& child) {
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  bernoulli_distribution dist(0.5);  

  // Take model1 from one parent and model2 from the other parent.
  if (dist(gen)) {
    child.model1 = parent1.model1;
    child.model2 = parent2.model2;    
  } else {
    child.model1 = parent2.model1;
    child.model2 = parent1.model2;    
  }

  // Take min_reward_diff from one parent.
  child.min_reward_diff = dist(gen)
    ? parent1.min_reward_diff
    : parent2.min_reward_diff;
}

void mutate(Organism& o) {
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  bernoulli_distribution dist(kMutateProbability);

  // Possibly mutate models.
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

  // Possibly mutate task-selection strategy. 
  uniform_int_distribution<> dist1(-kModel2RewardDiff - 1, kModel2RewardDiff + 1);
  if (dist(gen)) {
    o.min_reward_diff = dist1(gen);
  }
}

// Pick one of the multiple tasks from a challenge.
int pickTask(const Organism& o, const Challenge& c) {
  float reward_diff = c.reward2 - c.reward1;
  return reward_diff >= o.min_reward_diff ? 1 : 0;
}

// Performs task then returns the (partial) reward based on performance.
//
// The task of the organism is to match its model (i.e. bit string) to the
// "ideal" model (i.e. another bit string). The reward is the percentage of bits
// that match times the max reward.
float doTask(const vector<bool>& model, const vector<bool>& true_model, float reward) {
  int num_correct = 0;
  for (int i = 0; i < model.size(); ++i) {
    num_correct += model[i] == true_model[i];
  }

  float perc_correct = static_cast<float>(num_correct) / kDim;
  return reward * perc_correct;
}

float doChallenge(const Organism& o, const Challenge& c, const TrueModel& m) {
  int task = pickTask(o, c);

  return task == 0
    ? doTask(o.model1, m.model1, c.reward1)
    : doTask(o.model2, m.model2, c.reward2);
}

float doChallengeCooperatively(const Organism& o1, const Organism& o2, const Challenge& c, const TrueModel& m) {
  int o1_task = pickTask(o1, c);
  int o2_task = pickTask(o2, c);
  float o1_reward = o1_task == 0
    ? doTask(o1.model1, m.model1, c.reward1)
    : doTask(o1.model2, m.model2, c.reward2);
  float o2_reward = o2_task == 0
    ? doTask(o2.model1, m.model1, c.reward1)
    : doTask(o2.model2, m.model2, c.reward2);
  
  return o1_task == o2_task ? max(o1_reward, o2_reward) : o1_reward + o2_reward;
}


// Samples challenge, which is just rewards for task1 and task2. The rewards are
// such that task1 on average corresponds to higher reward, but sometimes, task2
// corresponds to a higher reward.
Challenge sampleChallenge() {
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());  
  uniform_real_distribution<float> dist(kMinReward, kMaxReward);
  float reward1 = dist(gen);
  float reward2 = dist(gen) - kModel2RewardDiff;
  return (struct Challenge){ .reward1 = reward1, .reward2 = reward2 };
}

void printDebugInfo(const vector<Organism>& population, const TrueModel& m) {
  // Compare population to the true models.
  vector<float> model1_perc_correct, model2_perc_correct;
  vector<int> min_reward_diffs;
  for (int i = 0; i < population.size(); ++i) {
    int num_correct1 = 0, num_correct2 = 0;
    for (int j = 0; j < kDim; ++j) {
      num_correct1 += population[i].model1[j] == m.model1[j];
      num_correct2 += population[i].model2[j] == m.model2[j];      
    }

    model1_perc_correct.push_back(static_cast<float>(num_correct1) / kDim);
    model2_perc_correct.push_back(static_cast<float>(num_correct2) / kDim);
    min_reward_diffs.push_back(population[i].min_reward_diff);
  }
  sort(model1_perc_correct.begin(), model1_perc_correct.end());
  sort(model2_perc_correct.begin(), model2_perc_correct.end());
  sort(min_reward_diffs.begin(), min_reward_diffs.end());


  cout << "Model1 stats: [" << 
    model1_perc_correct[0] << ", " <<   
    model1_perc_correct[kPopulation / 4] << ", " <<
    model1_perc_correct[kPopulation / 2] << ", " <<
    model1_perc_correct[kPopulation * 3 / 4] << ", " <<
    model1_perc_correct[kPopulation - 1] << "]" << endl;
  cout << "Model2 stats: [" << 
    model2_perc_correct[0] << ", " <<   
    model2_perc_correct[kPopulation / 4] << ", " <<
    model2_perc_correct[kPopulation / 2] << ", " <<
    model2_perc_correct[kPopulation * 3 / 4] << ", " <<
    model2_perc_correct[kPopulation - 1] << "]" << endl;
  cout << "Min reward diffs: [" << 
    min_reward_diffs[0] << ", " <<   
    min_reward_diffs[kPopulation / 4] << ", " <<
    min_reward_diffs[kPopulation / 2] << ", " <<
    min_reward_diffs[kPopulation * 3 / 4] << ", " <<
    min_reward_diffs[kPopulation - 1] << "]" << endl;
}

// Returns shuffled list of indices corresponding to population members.
vector<int> shufflePopulation() {
  vector<int> indices(kPopulation);
  iota(indices.begin(), indices.end(), 0);
  default_random_engine gen;
  gen.seed(std::chrono::system_clock::now().time_since_epoch().count());    
  shuffle(indices.begin(), indices.end(), gen);
  return indices;
}

// Returns list of indices such that matched pairs are adjacent. The idea is
// that "complementary" pairs should be matched, so individuals with the most
// different min_reward_diff values are matched.
vector<int> sortPopulationForMatching(const vector<Organism>& population) {
  const vector<int> shuffled_indices = shufflePopulation();
  
  vector<pair<int, int> > min_reward_diffs_and_indices;
  for (int i = 0; i < kPopulation; ++i) {
    min_reward_diffs_and_indices.push_back(
      make_pair<int, int>(
        population[shuffled_indices[i]].min_reward_diff,
        shuffled_indices[i]));
  }

  sort(min_reward_diffs_and_indices.begin(),
       min_reward_diffs_and_indices.end(),
       greater<pair<int, int> >());

  vector<int> indices;
  for (int i = 0; i < kPopulation / 2; ++i) {
    int index1 = min_reward_diffs_and_indices[i].second;
    int index2 = min_reward_diffs_and_indices[kPopulation - 1 - i].second;
    indices.push_back(index1);
    indices.push_back(index2);    
  }
  return indices;
}

// Get new population according to tournament selection and with or without
// mating (sexual or asexual).
vector<Organism> getNewPopulation(
  const vector<Organism>& population, const TrueModel& m) {
  vector<Organism> new_population;

  const vector<int> indices = shufflePopulation();

  // Compare each adjacent pair. The winner of each pair (i.e. one with higher
  // fitness will be able to create two mutated offspring for the next
  // generation).
  vector<Organism> winners;
  for (int i = 0; i < indices.size() - 1; i+=2) {
    const Organism& o1 = population[indices[i]];
    const Organism& o2 = population[indices[i + 1]];
    winners.push_back(o1.fitness > o2.fitness ? o1 : o2);
  }

  if (USE_MATING) {
    for (int i = 0; i < winners.size(); i+=2) {
      const Organism& parent1 = winners[i];
      const Organism& parent2 = winners[i + 1];
      Organism child1, child2, child3, child4;
      doCrossover(parent1, parent2, child1);
      doCrossover(parent1, parent2, child2);
      doCrossover(parent1, parent2, child3);
      doCrossover(parent1, parent2, child4);
      mutate(child1); mutate(child2);
      mutate(child3); mutate(child4);
      child1.fitness = 0; child2.fitness = 0;
      child3.fitness = 0; child4.fitness = 0;      
      new_population.push_back(child1); new_population.push_back(child2);
      new_population.push_back(child3); new_population.push_back(child4);            
    }
  } else {
    for (int i = 0; i < winners.size(); ++i) {
      Organism child1 = winners[i], child2 = winners[i];    
      mutate(child1);
      mutate(child2);
      child1.fitness = 0;
      child2.fitness = 0;
      new_population.push_back(child1);
      new_population.push_back(child2);      
    }
  }

  return new_population;
}

// Run a generation of the evolutionary algorithm.
vector<Organism> runGeneration(vector<Organism>& population, TrueModel& m) {
  const vector<int> indices = sortPopulationForMatching(population);
  for (int i = 0; i < kRounds; ++i) {
    Challenge c = sampleChallenge();

    if (USE_COOPERATION && (!USE_MIXED_COOPERATION || i % 2 == 0)) {
      for (int j = 0; j < kPopulation - 1; j+=2) {
        Organism& o1 = population[indices[j]];
        Organism& o2 = population[indices[j + 1]];
        float reward = doChallengeCooperatively(o1, o2, c, m);
        o1.fitness += reward;
        o2.fitness += reward;        
      }
    } else {
      for (int j = 0; j < kPopulation; ++j) {
        Organism& o = population[j];
        o.fitness += doChallenge(o, c, m);
      }      
    }
  }

  return getNewPopulation(population, m);
}


int main() {
  TrueModel m = initializeTrueModel();

  vector<Organism> population(kPopulation);
  for (int i = 0; i < kPopulation; ++i) {
    population[i] = initializeOrganism();
  }

  for (int i = 0; i < kGenerations; ++i) {
    population = runGeneration(population, m);
    printDebugInfo(population, m);
  }
}
