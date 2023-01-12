#pragma once

#include <algorithm>

using namespace std;

// TODO: compress all enum mode_t in a single entry (give it a meaningful name)

namespace Constraints
{

enum home_mode_t { HOME, AWAY, ANY };

inline string PrintMode(Constraints::home_mode_t mode)
{
    switch (mode)
    {
        case HOME: return "HOME";
        case AWAY: return "AWAY";
        case ANY: return "ANY";
    }
    return "UNKNOWN";
}


enum ConstraintType { CA1, CA2, CA3, CA4, GA1, BR1, BR2, FA2, SE1 };

// Constraints CA1
struct CA1Spec
{
    // Each team in team group $T$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} in slot group $S$
    int k_min, k_max;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned int team_group_index;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints CA2
struct CA2Spec
{
    // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against {teams, each team} in team group $T_2$ in slot group $S$
    enum mode1_t { HOME, AWAY, ANY };
    int k_min, k_max;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned int team_group_1_index, team_group_2_index;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints CA3
struct CA3Spec
{
    // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against teams in team group $T_2$ in each sequence of $k$ {time slots, games}
    int k_min, k_max, k;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned  int team_group_1_index, team_group_2_index;
    bool both_phases;
};

// Constraints CA4
struct CA4Spec
{
    // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against teams in team group $T_2$ in {time group, each time slot of time group}
    enum mode2_t { GLOBAL, EVERY };
    int k_min, k_max;
    home_mode_t mode1;
    mode2_t mode2;
    bool hard;
    int penalty;
    unsigned int team_group_1_index, team_group_2_index;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints GA1
struct GA1Spec
{
    // At least $k_{min}$ and at most $k_{max}$ games from $G = \{ (i_1, j_1), (i_2, j_2), \ldots \}$ take place in time group $S$.
    int k_min, k_max;
    bool hard;
    int penalty;
    vector<pair<unsigned int, unsigned int>> meeting_group;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints BR1
struct BR1Spec
{
    // Each team in team group $T$ has {exactly, no more than} $k$ {home breaks, away break, breaks} in time group $S$.
    int k;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned int team_group_index;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints BR2
struct BR2Spec
{
    // The sum over all {home, away, home or away} breaks of teams in team group $T$ is {exactly, no more than} $k$ in time group $S$
    int k_min, k_max, k;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned int team_group_index;
    unsigned int slot_group_index;
    bool both_phases;
};

// Constraints FA2
struct FA2Spec
{
    // Each pair of teams in team group $T$ has a difference in played {home games, away games, games} that is no longer than $k$ after each time slot in S
    int k;
    home_mode_t mode;
    bool hard;
    int penalty;
    unsigned int team_group_index;
    unsigned int slot_group_index;
    bool both_phases;
};


// Constraints SE1
struct SE1Spec
{
    // Each pair of teams in team group $T$ has at least $m_{min}$ and at most $m_{max}$ {time slots, games} between two consecutive mutual games
    int m_min, m_max;
    bool hard;
    int penalty;
    unsigned int team_group_index;
    bool both_phases;
};

}
