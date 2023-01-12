#pragma once

#include "stt_basics.hh"
#include "stt_helpers.hh"


class STT_SolutionManager
: public SolutionManager<STT_Input, STT_Solution>
{
public:
  STT_SolutionManager(const STT_Input & in, string name, bool mix_initial_phase = true, bool display_OF = false, bool vizing_greedy = false)
     : SolutionManager<STT_Input, STT_Solution>::SolutionManager(in, name), vizing_greedy(vizing_greedy), mix_initial_phase(mix_initial_phase), display_OF(display_OF) {}
  void RandomState(STT_Solution& st);
  void GreedyState(STT_Solution& st);
  bool CheckConsistency(const STT_Solution& st) const;
  void PrettyPrintOutput(const STT_Solution& st, std::string filename) const;
  bool OptimalStateReached(const STT_Solution& st) const override;

private:
  bool vizing_greedy;
  bool mix_initial_phase;
  bool display_OF;
};

class CA1CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class CA2CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class CA3CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class CA4CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class GA1CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class BR1CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class BR2CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class FA2CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class SE1CostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
};

class PhasedCostComponent : public CostComponent<STT_Input, STT_Solution>
{
public:
    using CostComponent<STT_Input, STT_Solution>::CostComponent;
    int ComputeCost(const STT_Solution& st) const;
    void PrintViolations(const STT_Solution& st, std::ostream& os) const;
    //void SetWeight(int new_weight) {weight = new_weight;}
};

/***************************************************************************
 * 1 STT_SwapHomes Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapHomesNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapHomes> 
{
public:
  STT_SwapHomesNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void AnyRandomMove(const STT_Solution&, STT_SwapHomes&) const;
  void RandomMove(const STT_Solution&, STT_SwapHomes&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapHomes&) const; 
  void ExecuteMove(STT_Solution&,const STT_SwapHomes&) const; 
  void MakeMove(STT_Solution&,const STT_SwapHomes&) const; 
  void FirstMove(const STT_Solution&,STT_SwapHomes&) const;
  bool NextMove(const STT_Solution&,STT_SwapHomes&) const;   
};

/***************************************************************************
 * 2 STT_SwapTeams Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapTeamsNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapTeams> 
{
public:
  STT_SwapTeamsNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void AnyRandomMove(const STT_Solution&, STT_SwapTeams&) const;
  void RandomMove(const STT_Solution&, STT_SwapTeams&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapTeams&) const; 
  void ExecuteMove(STT_Solution&,const STT_SwapTeams&) const; 
  void MakeMove(STT_Solution&,const STT_SwapTeams&) const; 
  void FirstMove(const STT_Solution&,STT_SwapTeams&) const;
  bool NextMove(const STT_Solution&,STT_SwapTeams&) const;   
};


/***************************************************************************
 * 3 STT_SwapRounds Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapRoundsNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapRounds> 
{
public:
  STT_SwapRoundsNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void AnyRandomMove(const STT_Solution&, STT_SwapRounds&) const;
  void RandomMove(const STT_Solution&, STT_SwapRounds&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapRounds&) const; 
  void ExecuteMove(STT_Solution&,const STT_SwapRounds&) const; 
  void MakeMove(STT_Solution&,const STT_SwapRounds&) const; 
  void FirstMove(const STT_Solution&,STT_SwapRounds&) const;
  bool NextMove(const STT_Solution&,STT_SwapRounds&) const;   
};

/***************************************************************************
 * 4 STT_SwapMatchesNotPhased Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapMatchesNotPhasedNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchesNotPhased> 
{
public:
  STT_SwapMatchesNotPhasedNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void RandomMove(const STT_Solution&, STT_SwapMatchesNotPhased&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapMatchesNotPhased&) const; 
  void FastExecuteMove(STT_Solution&,const STT_SwapMatchesNotPhased&) const;  //used only in FeasibleMove
  void MakeMove(STT_Solution&,const STT_SwapMatchesNotPhased&) const; 
  void FirstMove(const STT_Solution&,STT_SwapMatchesNotPhased&) const;
  bool NextMove(const STT_Solution&,STT_SwapMatchesNotPhased&) const;   
  bool ComputeAndCheckInvolvedRounds(const STT_Solution& st,
				      STT_SwapMatchesNotPhased& m) const;
  void SetMaxChainLength(unsigned mml) { max_move_length = mml; }
  unsigned GetMaxMoveLength() const { return max_move_length; }
private:
  bool AnyNextMove(const STT_Solution&,STT_SwapMatchesNotPhased&) const;  
  void AnyRandomMove(const STT_Solution&,STT_SwapMatchesNotPhased&) const;  
  unsigned max_move_length;
  unsigned max_move_lenght_partial_cost_components;
};

/***************************************************************************
 * 5 STT_SwapMatchesPhased Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapMatchesPhasedNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchesPhased> 
{
public:
  STT_SwapMatchesPhasedNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void RandomMove(const STT_Solution&, STT_SwapMatchesPhased&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapMatchesPhased&) const; 
  void FastExecuteMove(STT_Solution&,const STT_SwapMatchesPhased&) const;  //used only in FeasibleMove
  void MakeMove(STT_Solution&,const STT_SwapMatchesPhased&) const; 
  void FirstMove(const STT_Solution&,STT_SwapMatchesPhased&) const;
  bool NextMove(const STT_Solution&,STT_SwapMatchesPhased&) const;  
  bool SamePhase(unsigned int r1, unsigned int r2) const; //returns if two rounds belong to the same phase or not
  bool ComputeAndCheckInvolvedRounds(const STT_Solution& st, STT_SwapMatchesPhased& m) const;
  void SetMaxChainLength(unsigned mml) { max_move_length = mml; }
  unsigned GetMaxMoveLength() const { return max_move_length; }
private:
  bool AnyNextMove(const STT_Solution&,STT_SwapMatchesPhased&) const;  
  void AnyRandomMove(const STT_Solution&,STT_SwapMatchesPhased&) const;  
  unsigned max_move_length;
  unsigned max_move_lenght_partial_cost_components;
};

/***************************************************************************
 * 6 STT_SwapMatchRound Neighborhood Explorer:
 ***************************************************************************/

class STT_SwapMatchRoundNeighborhoodExplorer
  : public NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchRound> 
{
public:
  STT_SwapMatchRoundNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm);
  void RandomMove(const STT_Solution&, STT_SwapMatchRound&) const;
  bool FeasibleMove(const STT_Solution&, const STT_SwapMatchRound&) const; 
  void ExecuteMove(STT_Solution&,const STT_SwapMatchRound&) const; 
  void MakeMove(STT_Solution&,const STT_SwapMatchRound&) const; 
  void FirstMove(const STT_Solution&,STT_SwapMatchRound&) const;
  bool NextMove(const STT_Solution&,STT_SwapMatchRound&) const;   
  bool ComputeAndCheckInvolvedTeams(const STT_Solution& st,	const STT_SwapMatchRound& m, unsigned max) const;
  void SetMaxChainLength(unsigned mml) { max_move_length = mml; }
  unsigned GetMaxMoveLength() const { return max_move_length; }
private:
  bool AnyNextMove(const STT_Solution&,STT_SwapMatchRound&) const;  
  void AnyRandomMove(const STT_Solution&,STT_SwapMatchRound&) const;  
  unsigned max_move_length;
};



