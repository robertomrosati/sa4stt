#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <list>
#include <map>
#include <stdexcept>
#include <regex>
#include <map>
#include <pugixml.hpp>
#include <sstream>
#include "stt_constraints.hh"

using namespace std;
using namespace Constraints;

const size_t N_CONSTRAINTS = 9;

struct Team 
{
    friend ostream& operator<<(ostream& os, const Team& t);
    string id;
    string league;
    string name;
};

struct Slot
{
    friend ostream& operator<<(ostream& os, const Slot& s);
    string id;
    string name;
};

class STT_Input
{
    friend ostream& operator<<(ostream& os, const STT_Input& in);
public:
    STT_Input(string filename, int global_hard_weight, 
    int phased_weight, bool only_hard = false, bool mix_phase_during_search = true, 
    bool forbid_hard_worsening_moves = false, bool stop_at_zero_hard = false,
    array<int, N_CONSTRAINTS> detailed_hard_weights = {1, 1, 1, 1, 1, 1, 1, 1, 1});
    unsigned Teams() const {return teams.size();}
    string PrintSlots(const std::vector<unsigned int>& list) const
    {
        string result = "{";
        bool first = true;
        for (auto s : list)
            if (first)
            {
                result += slots[s].id;
                first = false;
            }
            else
                result += string(", ") + slots[s].id;
        result += "}";
        return result;
    }

    string PrintTeams(const std::vector<unsigned int>& list) const
    {
        string result = "{";
        bool first = true;
        for (auto t : list)
            if (first)
            {
                result += teams[t].id;
                first = false;
            }
            else
                result += string(", ") + teams[t].id;
        result += "}";
        return result;
    }
    
    string PrintMeetings(const std::vector<pair<unsigned int, unsigned int>>& list) const
    {
        string result = "{";
        bool first = true;
        for (auto m : list)
            if (first)
            {
                result += "(" + teams[m.first].id + ", " + teams[m.second].id + ")";
                first = false;
            }
            else
                result += string(", ") + "(" + teams[m.first].id + ", " + teams[m.second].id + ")";
        result += "}";
        return result;
    }
    
protected:
    int SearchForGroup(const std::vector<std::vector<unsigned int>>& these_groups, const std::vector<unsigned int>& other_group)
    {
        auto it = find_if(begin(these_groups), end(these_groups),
                          [other_group](const std::vector<unsigned int>& this_group) {
            return equal(begin(this_group), end(this_group), begin(other_group), end(other_group));
        });
        if (it != end(these_groups))
            return it - begin(these_groups);
        else
            return -1;
    }
    
    unsigned int DispatchTeamIds(const string& team_ids);
    unsigned int DispatchSlotIds(const string& slot_ids);
    vector<pair<unsigned int, unsigned int>> DispatchMeetings(const string& meetings);
    bool SamePhase(unsigned int s1, unsigned int s2) const {return (s1 < slots.size()/2 && s2 < slots.size()/2) || (s1 >= slots.size()/2 && s2 >= slots.size()/2);}
public:

    string name;
    string league_name; 
    // the tournament will be 2RR, time-constrained
    bool phased; // the tournament might be phased or not
    vector<Team> teams;
    vector<Slot> slots;
protected:
    // map between id and index
    map<string, unsigned int> team_index;
    // map between id and index
    map<string, unsigned int> slot_index;
    
public:
  int initial_stt_hard_weight;
  int initial_stt_phased_weight;
  bool only_hard;
  bool mix_phase_during_search;
  bool forbid_hard_worsening_moves;
  bool stop_at_zero_hard;
  array<int, N_CONSTRAINTS> hard_weights;
    // a team group is either an explicit group, if mentioned in the instance
    // or an implicit one if mentioned in the constraints
    // it is assumed that the indexes of the team members of each group are sorted in increasing order
    vector<vector<unsigned int>> team_group;
    
    // also a slot group can be explicitly mentioned or implicitly in the constraints
    // it is assumed that the indexes of the slot members of each group are sorted in increasing order
    vector<vector<unsigned int>> slot_group;
    
    // Redundant
    
    // reverse index, indicating for each team to which team groups it belongs to
    vector<list<unsigned int>> groups_of_team;
    
    // reverse index, indicating for each slot to which slot groups it belongs to
    vector<list<unsigned int>> groups_of_slot;
        
    vector<CA1Spec> constraints_CA1;
    vector<CA2Spec> constraints_CA2;
    vector<CA3Spec> constraints_CA3;
    vector<CA4Spec> constraints_CA4;
    vector<GA1Spec> constraints_GA1;
    vector<BR1Spec> constraints_BR1;
    vector<BR2Spec> constraints_BR2;
    vector<FA2Spec> constraints_FA2;
    vector<SE1Spec> constraints_SE1;
  

    //indici delle constraints hard, usate per rendere più veloce la ricerca dell violazioni nel 2-stages
    vector<vector<unsigned>> constraints_hard_indexes;
    
    // Redundant
    // data needed for material cost, inverse matrix[t,s] that contains 
    // for each team (t) and slot (s) the vector of constraints in which it appears
    // please note that there is a different matrix for each constraint,
    // each one is indexed by by the Constraints::ConstraintType because
    // each matrix is in a pair with its Constraints::ConstraintType
    // Usage: team_slot_constraints[GA1][t][s][i] to access the i-th constraint
    // that involves team s and slot s for constraint type GA1
    vector<vector<vector<vector<unsigned int>>>> team_slot_constraints;
    vector<vector<vector<unsigned int>>> team_constraints;
    vector<vector<vector<unsigned int>>> slot_constraints;
    bool IsHard(unsigned int c_type, unsigned int c) const;
    unsigned int ConstraintsVectorSize(unsigned int c_type) const;
};
