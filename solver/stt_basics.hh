#pragma once

#include "stt_data.hh"
#include <iostream>
#include <easylocal.hh>
using namespace EasyLocal::Core;

class STT_Solution
{
    friend ostream& operator<<(ostream& os, const STT_Solution& st);
    friend istream& operator>>(istream& is, STT_Solution& st);
    friend bool operator==(const STT_Solution& sol1, const STT_Solution& sol2);
    // Assumptions:
    // 1. being a compact double round-robin each team plays at each time slot
public:
    STT_Solution(const STT_Input& in,  bool display_OF = false) 
        : in(in), opponent(in.teams.size(), vector<unsigned int>(in.slots.size())), home(in.teams.size(), 
        vector<bool>(in.slots.size())), match(in.teams.size(), vector<unsigned int>(in.teams.size())), is_return_match(in.teams.size(), 
        vector<bool>(in.slots.size())), cost_components(N_CONSTRAINTS,0), cost_components_hard(N_CONSTRAINTS,0), 
        cost_single_constraints(N_CONSTRAINTS, vector<int>(0)), total_cost_components(0), 
        total_cost_components_hard(0), cost_phased(0), stt_hard_weight(in.initial_stt_hard_weight), 
        stt_phased_weight(in.initial_stt_phased_weight),
        display_OF_isset(display_OF), move_counter(1), last_best_solution(0), 
        last_best_counter(0)
    {
        cost_single_constraints[CA1].resize(in.constraints_CA1.size(),0);
        cost_single_constraints[CA2].resize(in.constraints_CA2.size(),0);
        cost_single_constraints[CA3].resize(in.constraints_CA3.size(),0);
        cost_single_constraints[CA4].resize(in.constraints_CA4.size(),0);
        cost_single_constraints[GA1].resize(in.constraints_GA1.size(),0);
        cost_single_constraints[BR1].resize(in.constraints_BR1.size(),0);
        cost_single_constraints[BR2].resize(in.constraints_BR2.size(),0);
        cost_single_constraints[FA2].resize(in.constraints_FA2.size(),0);
        cost_single_constraints[SE1].resize(in.constraints_SE1.size(),0);
    }
    STT_Solution(const STT_Solution& st) : in(st.in)
    {
        opponent = st.opponent;
        home = st.home;
        match = st.match;
        is_return_match = st.is_return_match;
        cost_components = st.cost_components;
        cost_components_hard = st.cost_components_hard;
        cost_single_constraints = st.cost_single_constraints;
        total_cost_components = st.total_cost_components;
        total_cost_components_hard = st.total_cost_components_hard;
        cost_phased = st.cost_phased;
        stt_hard_weight = st.stt_hard_weight;
        stt_phased_weight = st.stt_phased_weight;
        move_counter = st.move_counter;
        display_OF_isset = st.display_OF_isset;
        last_best_solution = st.last_best_solution;
        last_best_counter = st.last_best_counter;
        print_solution_on_one_line = st.print_solution_on_one_line;
    }
    STT_Solution& operator=(const STT_Solution& st)
    {
        opponent = st.opponent;
        home = st.home;
        match = st.match;
        is_return_match = st.is_return_match;
        cost_components = st.cost_components;
        cost_components_hard = st.cost_components_hard;
        cost_single_constraints = st.cost_single_constraints;   
        total_cost_components = st.total_cost_components;
        total_cost_components_hard = st.total_cost_components_hard;
        cost_phased = st.cost_phased;
        stt_hard_weight = st.stt_hard_weight;
        stt_phased_weight = st.stt_phased_weight;
        move_counter = st.move_counter;
        display_OF_isset = st.display_OF_isset;
        last_best_solution = st.last_best_solution;
        last_best_counter = st.last_best_counter;
        print_solution_on_one_line = st.print_solution_on_one_line;
        return *this;
    }
    void CanonicalPattern(bool permute, bool mix_initial_phase = true);
    //needed by makemove functions
    void UpdateStateCell(unsigned t1, unsigned r, unsigned t2, bool home_game);
    void UpdateMatches(unsigned t1, unsigned t2, unsigned r, bool rev1 = false, bool rev2 = false);
    void InternalMakeSwapHomes(unsigned t1, unsigned t2);
    void Shuffle(vector<size_t>::iterator start, vector<size_t>::iterator stop);

    //other methods
    int ReturnTotalCost();
    int CalculateFullCost();
    int CalculateCostComponent(unsigned int c_type);
    int CalculateCostComponentHard(unsigned int c_type);
    int CalculateCostSingleConstraint(unsigned int c_type, unsigned int c) const; //calculate the value but doesn't modify the data
  // calculate the single cost of a given constraint up to round r
    float GreedyCalculateCostSingleConstraint(Constraints::ConstraintType c_type, unsigned int c, unsigned int r) const;
    float GreedyCalculateCost(unsigned int r) const;
    int CalculateCostPhased();
    void UpdateSelectionedCostsConstraints(vector<vector<unsigned>> involved_constraints); //function that, given a matrix of constraints indexes (of size N_CONSTRAINTS) as input, recalucalte the cost taking into account only those specific constraints. 
    void RescaleWeightConstraintsBothPhases(unsigned weight);


    vector<vector<vector<int>>> GetCplexWarmSolution();

    void UpdateLocalCostsDeleteMatch(unsigned t1, unsigned t2, unsigned old_r);
    void UpdateLocalCostsInsertMatch(unsigned t1, unsigned t2, unsigned r);
    vector<unsigned> getInvolvedConstraints(Constraints::ConstraintType c_type, unsigned t1, unsigned t2, unsigned s) const;
    vector<unsigned> getInvolvedConstraints(Constraints::ConstraintType c_type, vector<unsigned> involved_teams, vector<unsigned> involved_slots) const;
    void RecalculateConstraintsForTeamsMatches(vector<unsigned> involved_teams, vector<unsigned> involved_matches);
    bool SamePhase(unsigned int r1, unsigned int r2) const;
    void PopulateIsReturnMatrix(); //given a consistent state of opponent, home and match, populates the "is_return_match" matrix
    bool IsReturnMatch(unsigned t1, unsigned t2) const {return is_return_match[t1][match[t1][t2]];} //finds the match t1-t2 and states wheter it is a "go" or a "return"
    void PopulateCellInsideIsReturnMatrix(unsigned t, unsigned s);
    void PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(unsigned t, unsigned s);
    unsigned SpecularMatch(unsigned t1, unsigned t2) const; //given a match t1-t2, returns the index of the match t2-tq
    int PrintIsReturnMatrix(bool suppress_print=false) const;
    bool InternalCheckConsistency() const;
    string PrintCostComponentsCompacted() const;
    

    void UpdateMoveCounterAndBestSolution(int new_cost);
    void DisplayOFIfNeeded();
    void SetPrintSolutionOneLine(bool v) {print_solution_on_one_line = v;}
    const STT_Input& in;
    
    // these were the same state structures as for the TTP
    vector<vector<unsigned int>> opponent; // matrix (i, s) stating which is the opponent of team i at slot s
    vector<vector<bool>> home; // matrix (i, s) stating whether the team i plays at home at slot s
    //new redundant data added on 2020-11-24 to ease the execution of SwapHomes move
    vector<vector<unsigned int>> match; //matrix (i,j) stating which is the slot in which teams at row plays at home against teams at column
    vector<vector<bool>> is_return_match; // matrix (i,s) stating whether the team i is playing a "go" (value false) game or a "return" (value true)

    //materialized costs
    vector<int> cost_components; //usage: cost_components[Constraints::Constraint_Type c_type], example cost_components[CA1]
    vector<int> cost_components_hard; // like cost_components, but tracks only hard cost
    vector<vector<int>> cost_single_constraints; //usage: cost_single_constraints[Constraints::Constraint_Type c_type][unsigned i], example cost_single_constraints[CA1][4]
    int total_cost_components;
    int total_cost_components_hard;
    int cost_phased;
    int stt_hard_weight;
    int stt_phased_weight;
    bool display_OF_isset;
    long long unsigned int move_counter;
    int last_best_solution;
    long long unsigned int last_best_counter;
  private:
    //parameters that guide the way of displaying the solution
    bool print_solution_on_one_line;
};



// ***************************************************************************
// ************** NEIGHBORHOOD 1: swap homes              ********************
// ***************************************************************************

class STT_SwapHomes
{
    friend bool operator==(const STT_SwapHomes &m1, const STT_SwapHomes &m2);
    friend bool operator!=(const STT_SwapHomes &m1, const STT_SwapHomes &m2);
    friend bool operator<(const STT_SwapHomes &m1, const STT_SwapHomes &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapHomes& m);
    friend istream& operator>>(istream& is, STT_SwapHomes& m);
public:
    STT_SwapHomes(unsigned s1 = 0, unsigned s2 = 0) : t1(s1), t2(s2) {}
    unsigned t1, t2;
};

// ***************************************************************************
// *****   NEIGHBORHOOD 1+: improved swap homes (with explicit rounds)   *****
// ***************************************************************************

class STT_ImprovedSwapHomes
{
    friend bool operator==(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2);
    friend bool operator!=(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2);
    friend bool operator<(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2);
    friend ostream& operator<<(ostream& os, const STT_ImprovedSwapHomes& m);
    friend istream& operator>>(istream& is, STT_ImprovedSwapHomes& m);
public:
    STT_ImprovedSwapHomes(unsigned s1 = 0, unsigned s2 = 0, unsigned r1 = 0, unsigned r2 = 0) : t1(s1), t2(s2), r1(r1), r2(r2) {}
    unsigned t1, t2, r1, r2;
};


// ***************************************************************************
// ************** NEIGHBORHOOD 2: swap teams              ********************
// ***************************************************************************

class STT_SwapTeams
{
    friend bool operator==(const STT_SwapTeams &m1, const STT_SwapTeams &m2);
    friend bool operator!=(const STT_SwapTeams &m1, const STT_SwapTeams &m2);
    friend bool operator<(const STT_SwapTeams &m1, const STT_SwapTeams &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapTeams& m);
    friend istream& operator>>(istream& is, STT_SwapTeams& m);
public:
  STT_SwapTeams(unsigned s1 = 0, unsigned s2 = 0) : t1(s1), t2(s2) {}
  unsigned t1, t2;
};

// ***************************************************************************
// ************** NEIGHBORHOOD 3: swap rounds             ********************
// ***************************************************************************

class STT_SwapRounds
{
    friend bool operator==(const STT_SwapRounds &m1, const STT_SwapRounds &m2);
    friend bool operator!=(const STT_SwapRounds &m1, const STT_SwapRounds &m2);
    friend bool operator<(const STT_SwapRounds &m1, const STT_SwapRounds &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapRounds& m);
    friend istream& operator>>(istream& is, STT_SwapRounds& m);
public:
    STT_SwapRounds(unsigned s1 = 0, unsigned s2 = 0) : r1(s1), r2(s2) {}
    unsigned r1, r2;
};

// ***************************************************************************
// ****      NEIGHBORHOOD 4: swap matches     (NOT PHASED case)           ****
// ***************************************************************************

class STT_SwapMatchesNotPhased
{
    friend bool operator==(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2);
    friend bool operator!=(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2);
    friend bool operator<(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapMatchesNotPhased& m);
    friend istream& operator>>(istream& is, STT_SwapMatchesNotPhased& m);
public:
    STT_SwapMatchesNotPhased(unsigned s1 = 0, unsigned s2 = 0) : t1(s1), t2(s2), rs(1,0) {}
    unsigned t1, t2;
    mutable vector<unsigned> rs; // this part is computed while checking the move 
};

// ***************************************************************************
// ****      NEIGHBORHOOD 5: swap matches     (PHASED case)               ****
// ***************************************************************************

class STT_SwapMatchesPhased
{
    friend bool operator==(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2);
    friend bool operator!=(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2);
    friend bool operator<(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapMatchesPhased& m);
    friend istream& operator>>(istream& is, STT_SwapMatchesPhased& m);
public:
    STT_SwapMatchesPhased(unsigned s1 = 0, unsigned s2 = 0) : t1(s1), t2(s2), rs(1,0), swap_homes_t1(1,false), swap_homes_t2(1,false) {}
    unsigned t1, t2;
    mutable vector<unsigned> rs; // this part is computed while checking the move 
    mutable vector<bool> swap_homes_t1; // this part is computed while checking the move (we have to calculate it in advance)
    mutable vector<bool> swap_homes_t2; // this part is computed while checking the move (we have to calculate it in advance)
};


// ***************************************************************************
// ****   NEIGHBORHOOD 6: swap matchround (called PartialSwapRound)       ****
// ***************************************************************************

class STT_SwapMatchRound
{
    friend bool operator==(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2);
    friend bool operator!=(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2);
    friend bool operator<(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2);
    friend ostream& operator<<(ostream& os, const STT_SwapMatchRound& m);
    friend istream& operator>>(istream& is, STT_SwapMatchRound& m);
public:
    STT_SwapMatchRound(unsigned s1 = 0, unsigned s2 = 0) : r1(s1), r2(s2), ts(1,0) {}
    unsigned r1, r2;
    mutable vector<unsigned> ts; // this part is computed while checking the move

};

