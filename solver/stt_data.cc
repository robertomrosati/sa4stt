#include "stt_data.hh"

#define ADD_CONSTRAINT(constraint_set, constraint) \
  constraint_set.push_back(constraint)

std::vector<std::string> split(const string &input, const string &regex)
{
  // passing -1 as the submatch index parameter performs splitting
  std::regex re(regex);
  std::sregex_token_iterator
      first{input.begin(), input.end(), re, -1},
      last;
  return {first, last};
}

ostream &operator<<(ostream &os, const Team &t)
{
  os << t.name << " (" << t.id << ", " << t.league << ")";
  return os;
}

ostream &operator<<(ostream &os, const Slot &s)
{
  os << s.name << " (" << s.id << ")";
  return os;
}

STT_Input::STT_Input(string filename, int global_hard_weight, int phased_weight, bool only_hard, bool mix_phase_during_search, bool forbid_hard_worsening_moves, bool stop_at_zero_hard, array<int, N_CONSTRAINTS> detailed_hard_weights)
    : initial_stt_hard_weight(global_hard_weight), initial_stt_phased_weight(phased_weight),
      only_hard(only_hard), mix_phase_during_search(mix_phase_during_search),
      forbid_hard_worsening_moves(forbid_hard_worsening_moves),
      stop_at_zero_hard(stop_at_zero_hard), hard_weights(detailed_hard_weights)
{
  constraints_hard_indexes.resize(N_CONSTRAINTS, vector<unsigned>(0));

  pugi::xml_document doc;

  pugi::xml_parse_result result = doc.load_file(filename.c_str());

  if (!result)
  {
    ostringstream msg;
    msg << "XML [" << filename << "] parsed with errors" << endl;
    msg << "Error description: " << result.description() << endl;
    msg << "Error offset: " << result.offset;
    throw logic_error(msg.str());
  }

  name = doc.select_node("/Instance/MetaData/InstanceName").node().text().get();
  // assume a single league
  if (doc.select_nodes("/Instance/Structure/Format").size() != 1)
    throw logic_error("It is assumed to have a single league format");
  auto format = doc.select_node("/Instance/Structure/Format[1]").node();
  phased = (format.child("gameMode").text().get() == string("P"));
  // perform a few semantic checks about the compliance to the ITC-2021 rules
  unsigned int k = format.child("numberRoundRobin").text().as_uint();
  if (k != 2)
    throw logic_error("The tournament should be a 2-RoundRobin, found instead " + to_string(k) + "-RR specification");
  string compactness = format.child("compactness").text().get();
  if (compactness != string("C"))
    throw logic_error("The tournament should require a time-constrained (compact) timetable, found " + compactness);
  if (!doc.select_nodes("/Instance/Structure/AdditionalGames/*").empty())
    throw logic_error("It is not expected to have additional games");
  string objective = doc.select_node("/Instance/ObjectiveFunction/Objective").node().text().get();
  if (objective != string("SC"))
    throw logic_error("The tournament requires a timetable which minimizes the total soft constraints penalty, found " + objective);
  auto resources = doc.select_node("/Instance/Resources").node();
  if (!resources.select_nodes("LeagueGroups/*").empty())
    throw logic_error("It is expected not to have any league group");
  if (resources.select_nodes("Leagues/league").size() != 1)
    throw logic_error("It is expected to have a single league");
  league_name = resources.select_node("Leagues/league").node().attribute("name").as_string();

  // teams dispatching
  for (auto t : resources.select_nodes("Teams/team"))
  {
    string id = t.node().attribute("id").as_string();
    team_index[id] = teams.size();
    teams.push_back(Team{id, t.node().attribute("league").as_string(), t.node().attribute("name").as_string()});
  }
  // TODO: dispatch TeamGroups
  // Explicit team groups are supposed not to be present, but just implicit ones as mentioned in
  // constraints, check against it
  if (!resources.select_nodes("TeamGroups/*").empty())
    throw logic_error("We were assuming there's no explicit team group");

  // slot dispatching
  for (auto s : resources.select_nodes("Slots/slot"))
  {
    string id = s.node().attribute("id").as_string();
    slot_index[id] = slots.size();
    slots.push_back(Slot{id, s.node().attribute("name").as_string()});
  }
  // TODO: dispatch SlotGroups
  // Explicit slot groups are supposed not to be present, but just implicit ones as mentioned in
  // constraints, check against it
  if (!resources.select_nodes("SlotGroups/*").empty())
    throw logic_error("We were assuming there's no explicit slot group");

  // *******************************************
  // Constraints dispatching
  // *******************************************

  // Dispatch constraints
  auto constraints = doc.select_node("/Instance/Constraints").node();

  // Basic constraints should be empty
  if (!constraints.select_nodes("BasicConstraints/*").empty())
    throw logic_error("It is expected not to have BasicConstraints");

  // Capacity constraints
  for (auto c : constraints.child("CapacityConstraints").children())
  {
    // Capacity constraints should be only of type CA1–CA4
    if (c.name() == string("CA1"))
    {
      // Each team in team group $T$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} in slot group $S$
      CA1Spec constraint;
      if (c.attribute("min"))
        constraint.k_min = c.attribute("min").as_int();
      else
        constraint.k_min = 0;
      if (c.attribute("max"))
        constraint.k_max = c.attribute("max").as_int();
      else
        constraint.k_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode").as_string() == string("H"))
        constraint.mode = HOME;
      else if (c.attribute("mode").as_string() == string("A"))
        constraint.mode = AWAY;
      // it is supposed that CA1 are just with HOME and AWAY modes
      //            else if (c.attribute("mode").as_string() == string("HA"))
      //                constraint.mode = ANY;
      else
        throw logic_error("Unknown mode " + string(c.attribute("mode").as_string()) + " for CA1 constraint");
      constraint.team_group_index = DispatchTeamIds(c.attribute("teams").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());

      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_CA1, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_CA1, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[CA1].push_back(constraints_CA1.size() - 1);
    }
    else if (c.name() == string("CA2"))
    {
      // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against {teams, each team} in team group $T_2$ in slot group $S$
      CA2Spec constraint;
      if (c.attribute("min"))
        constraint.k_min = c.attribute("min").as_int();
      else
        constraint.k_min = 0;
      if (c.attribute("max"))
        constraint.k_max = c.attribute("max").as_int();
      else
        constraint.k_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode1").as_string() == string("H"))
        constraint.mode = HOME;
      else if (c.attribute("mode1").as_string() == string("A"))
        constraint.mode = AWAY;
      else if (c.attribute("mode1").as_string() == string("HA"))
        constraint.mode = ANY;
      else
        throw logic_error("Unknown mode1 " + string(c.attribute("mode1").as_string()) + " for CA2 constraint");
      if (c.attribute("mode2").as_string() != string("GLOBAL"))
        throw logic_error("Unknown mode2 " + string(c.attribute("mode2").as_string()) + " for CA2 constraint");

      constraint.team_group_1_index = DispatchTeamIds(c.attribute("teams1").as_string());
      constraint.team_group_2_index = DispatchTeamIds(c.attribute("teams2").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      // constraints_CA2.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_CA2, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_CA2, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[CA2].push_back(constraints_CA2.size() - 1);
    }
    else if (c.name() == string("CA3"))
    {
      // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against teams in team group $T_2$ in each sequence of $k$ {time slots, games}
      CA3Spec constraint;
      if (c.attribute("min"))
        constraint.k_min = c.attribute("min").as_int();
      else
        constraint.k_min = 0;
      if (c.attribute("max"))
        constraint.k_max = c.attribute("max").as_int();
      else
        constraint.k_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode1").as_string() == string("H"))
        constraint.mode = HOME;
      else if (c.attribute("mode1").as_string() == string("A"))
        constraint.mode = AWAY;
      else if (c.attribute("mode1").as_string() == string("HA"))
        constraint.mode = ANY;
      else
        throw logic_error("Unknown mode1 " + string(c.attribute("mode1").as_string()) + " for CA2 constraint");
      if (c.attribute("mode2").as_string() != string("SLOTS"))
        throw logic_error("Unknown mode2 " + string(c.attribute("mode2").as_string()) + " for CA3 constraint");
      constraint.k = c.attribute("intp").as_int();

      constraint.team_group_1_index = DispatchTeamIds(c.attribute("teams1").as_string());
      constraint.team_group_2_index = DispatchTeamIds(c.attribute("teams2").as_string());
      // constraints_CA3.push_back(constraint);
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_CA3, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_CA3, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[CA3].push_back(constraints_CA3.size() - 1);
    }
    else if (c.name() == string("CA4"))
    {
      // Each team in team group $T_1$ plays at least $k_{min}$ and at most $k_{max}$ {home games, away games, games} against teams in team group $T_2$ in {time group, each time slot of time group}
      CA4Spec constraint;
      if (c.attribute("min"))
        constraint.k_min = c.attribute("min").as_int();
      else
        constraint.k_min = 0;
      if (c.attribute("max"))
        constraint.k_max = c.attribute("max").as_int();
      else
        constraint.k_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode1").as_string() == string("H"))
        constraint.mode1 = HOME;
      else if (c.attribute("mode1").as_string() == string("A"))
        constraint.mode1 = AWAY;
      else if (c.attribute("mode1").as_string() == string("HA"))
        constraint.mode1 = ANY;
      else
        throw logic_error("Unknown mode1 " + string(c.attribute("mode1").as_string()) + " for CA4 constraint");
      if (c.attribute("mode2").as_string() == string("GLOBAL"))
        constraint.mode2 = CA4Spec::GLOBAL;
      else if (c.attribute("mode2").as_string() == string("EVERY"))
        constraint.mode2 = CA4Spec::EVERY;

      constraint.team_group_1_index = DispatchTeamIds(c.attribute("teams1").as_string());
      constraint.team_group_2_index = DispatchTeamIds(c.attribute("teams2").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      // constraints_CA4.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_CA4, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_CA4, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[CA4].push_back(constraints_CA4.size() - 1);
    }
    else
      throw logic_error("Capacity constraints of type " + string(c.name()) + " are not allowed");
  }
  // Game constraints
  for (auto c : constraints.child("GameConstraints").children())
  {
    // TODO: maybe it is worth defining and storing also meeting_groups as team and slot groups
    if (c.name() == string("GA1"))
    {
      // At least $k_{min}$ and at most $k_{max}$ games from $G = \{ (i_1, j_1), (i_2, j_2), \ldots \}$ take place in time group $S$.
      GA1Spec constraint;
      if (c.attribute("min"))
        constraint.k_min = c.attribute("min").as_int();
      else
        constraint.k_min = 0;
      if (c.attribute("max"))
        constraint.k_max = c.attribute("max").as_int();
      else
        constraint.k_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      constraint.meeting_group = DispatchMeetings(c.attribute("meetings").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      // constraints_GA1.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_GA1, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_GA1, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[GA1].push_back(constraints_GA1.size() - 1);
    }
    else
      throw logic_error("Game constraints of type " + string(c.name()) + " are not allowed");
  }
  // Break constraints
  for (auto c : constraints.child("BreakConstraints").children())
  {
    if (c.name() == string("BR1"))
    {
      // Each team in team group $T$ has {exactly, no more than} $k$ {home breaks, away break, breaks} in time group $S$.
      BR1Spec constraint;
      if (c.attribute("intp"))
        constraint.k = c.attribute("intp").as_int();
      else
        throw logic_error("Game constraints BR1 should have a max specification");
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (!c.attribute("homeMode")) // FIXME: this is an assumption to be checked with organizers
        constraint.mode = ANY;
      else if (c.attribute("homeMode").as_string() == string("H"))
        constraint.mode = HOME;
      else if (c.attribute("homeMode").as_string() == string("A"))
        constraint.mode = AWAY;
      else if (c.attribute("homeMode").as_string() == string("HA"))
        constraint.mode = ANY;
      else
        throw logic_error("Unknown homeMode " + string(c.attribute("homeMode").as_string()) + " for BR1 constraint");
      constraint.team_group_index = DispatchTeamIds(c.attribute("teams").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      // constraints_BR1.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_BR1, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_BR1, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[BR1].push_back(constraints_BR1.size() - 1);
    }
    else if (c.name() == string("BR2"))
    {
      // The sum over all {home, away, home or away} breaks of teams in team group $T$ is {exactly, no more than} $k$ in time group $S$
      BR2Spec constraint;
      if (c.attribute("intp"))
        constraint.k = c.attribute("intp").as_int();
      else
        throw logic_error("Game constraints BR2 should have a intp attribute specification");
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (!c.attribute("homeMode") || c.attribute("homeMode").as_string() == string("HA"))
        constraint.mode = ANY;
      else
        throw logic_error("Unknown homeMode " + string(c.attribute("homeMode").as_string()) + " for BR2 constraint");
      if (c.attribute("mode2").as_string() != string("LEQ"))
        throw logic_error("Assuming mode2 is LEQ for BR2 constraints, found " + string(c.attribute("mode2").as_string()));

      constraint.team_group_index = DispatchTeamIds(c.attribute("teams").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());

      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_BR2, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_BR2, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[BR2].push_back(constraints_BR2.size() - 1);
    }
    else
      throw logic_error("Game constraints of type " + string(c.name()) + " are not allowed");
  }
  // Fairness constraints
  for (auto c : constraints.child("FairnessConstraints").children())
  {
    if (c.name() == string("FA2"))
    {
      // Each pair of teams in team group $T$ has a difference in played {home games, away games, games} that is no longer than $k$ after each time slot in S
      FA2Spec constraint;
      if (c.attribute("intp"))
        constraint.k = c.attribute("intp").as_int();
      else
        throw logic_error("Fairness constraints FA2 should have a intp attribute specification");
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode").as_string() == string("H"))
        constraint.mode = HOME;
      else if (c.attribute("mode").as_string() == string("A"))
        constraint.mode = AWAY;
      else if (c.attribute("mode").as_string() == string("HA"))
        constraint.mode = ANY;
      else
        throw logic_error("Unknown mode " + string(c.attribute("mode").as_string()) + " for FA2 constraint");

      constraint.team_group_index = DispatchTeamIds(c.attribute("teams").as_string());
      constraint.slot_group_index = DispatchSlotIds(c.attribute("slots").as_string());
      //popolo il valore di constraint.both_phases
      constraint.both_phases = false;
      for (unsigned i = 1; i < slot_group[constraint.slot_group_index].size(); i++)
      {
        if (!SamePhase(slot_group[constraint.slot_group_index][i - 1], slot_group[constraint.slot_group_index][i]))
        {
          constraint.both_phases = true;
          // constraint.penalty = constraint.penalty*2;
          break;
        }
      }
      // constraints_FA2.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_FA2, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_FA2, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[FA2].push_back(constraints_FA2.size() - 1);
    }
    else
      throw logic_error("Game constraints of type " + string(c.name()) + " are not allowed");
  }
  // Separation constraints
  for (auto c : constraints.child("SeparationConstraints").children())
  {
    if (c.name() == string("SE1"))
    {
      // Each pair of teams in team group $T$ has at least $m_{min}$ and at most $m_{max}$ {time slots, games} between two consecutive mutual games
      SE1Spec constraint;
      if (c.attribute("min"))
        constraint.m_min = c.attribute("min").as_int();
      else
        constraint.m_min = 0;
      if (c.attribute("max"))
        constraint.m_max = c.attribute("max").as_int();
      else
        constraint.m_max = static_cast<int>(slots.size());
      constraint.hard = c.attribute("type").as_string() == string("HARD");
      constraint.penalty = c.attribute("penalty").as_int();
      if (c.attribute("mode1") && c.attribute("mode1").as_string() != string("SLOTS"))
        throw logic_error("Not expected mode1 " + string(c.attribute("mode1").as_string()) + " for SE1 constraint");

      constraint.team_group_index = DispatchTeamIds(c.attribute("teams").as_string());
      constraint.both_phases = false;
      // constraints_SE1.push_back(constraint);
      if (only_hard)
      {
        if (constraint.hard)
          ADD_CONSTRAINT(constraints_SE1, constraint);
      }
      else
        ADD_CONSTRAINT(constraints_SE1, constraint);

      //in ogni caso
      if (constraint.hard)
        constraints_hard_indexes[SE1].push_back(constraints_SE1.size() - 1);
    }
    else
      throw logic_error("Game constraints of type " + string(c.name()) + " are not allowed");
  }

  // *******************************************
  // Auxiliary data for efficient access
  // *******************************************

  // these are the reverse index of groups to teams / slots

  groups_of_team.resize(teams.size());
  for (size_t i = 0; i < team_group.size(); i++)
  {
    for (auto t : team_group[i])
      groups_of_team[t].push_back(i);
  }

  groups_of_slot.resize(slots.size());
  for (size_t i = 0; i < slot_group.size(); i++)
  {
    for (auto s : slot_group[i])
      groups_of_slot[s].push_back(i);
  }

  // Auxiliary Data: Now I populate the inverse matrices from team/slots to constraints
  // I resize the matrices
  team_slot_constraints.resize(N_CONSTRAINTS, vector<vector<vector<unsigned>>>(teams.size(), vector<vector<unsigned>>(slots.size(), vector<unsigned>(0, 0))));
  team_constraints.resize(N_CONSTRAINTS, vector<vector<unsigned>>(teams.size(), vector<unsigned>(0, 0)));
  slot_constraints.resize(N_CONSTRAINTS, vector<vector<unsigned>>(slots.size(), vector<unsigned>(0, 0)));
  // I execute, for each constraint, a cycle to fill in the matrix

  unsigned t1, t2, s1; //dichiaro quattro variabili di supporto per rendere più leggibile il codice seguente
  bool already_in_vector;

  //CA1
  //per ogni constraint
  //per ogni team presente nel team_group_index
  //per ogni slot presente nel vincolo
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle CA1 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_CA1.size(); i++)
    for (unsigned int t = 0; t < team_group[constraints_CA1[i].team_group_index].size(); t++)
      for (unsigned int s = 0; s < slot_group[constraints_CA1[i].slot_group_index].size(); s++)
      {
        t1 = team_group[constraints_CA1[i].team_group_index][t];
        s1 = slot_group[constraints_CA1[i].slot_group_index][s];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA1][t1][s1].size(); j++)
          if (i == team_slot_constraints[CA1][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA1][t1][s1].push_back(i);
      }

  //CA2
  //per ogni constraint
  //per ogni slot presente nel vincolo
  //per ogni team presente nel team_group_1_index
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle CA2 agli indici [t,s]
  //ripeto per team_group_2_index
  for (unsigned int i = 0; i < constraints_CA2.size(); i++)
  {
    for (unsigned int s = 0; s < slot_group[constraints_CA2[i].slot_group_index].size(); s++)
    {
      s1 = slot_group[constraints_CA2[i].slot_group_index][s];
      for (unsigned int t = 0; t < team_group[constraints_CA2[i].team_group_1_index].size(); t++)
      {
        t1 = team_group[constraints_CA2[i].team_group_1_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA2][t1][s1].size(); j++)
          if (i == team_slot_constraints[CA2][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA2][t1][s1].push_back(i);
      }
      for (unsigned int t = 0; t < team_group[constraints_CA2[i].team_group_2_index].size(); t++)
      {
        t2 = team_group[constraints_CA2[i].team_group_2_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA2][t2][s1].size(); j++)
          if (i == team_slot_constraints[CA2][t2][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA2][t2][s1].push_back(i);
      }
    }
  }
  //CA3
  //per ogni constraint
  //per tutti gli slot (questo vincolo non è riferito a uno slot specifico)
  //per tutti i team in team_group_1_index
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle CA3 agli indici [t,s]
  //ripeto per team_group_2_index
  for (unsigned int i = 0; i < constraints_CA3.size(); i++)
  {
    for (unsigned int s = 0; s < slots.size(); s++)
    {
      for (unsigned int t = 0; t < team_group[constraints_CA3[i].team_group_1_index].size(); t++)
      {
        t1 = team_group[constraints_CA3[i].team_group_1_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA3][t1][s].size(); j++)
          if (i == team_slot_constraints[CA3][t1][s][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA3][t1][s].push_back(i);
      }

      //ripeto per team_group_2_index
      for (unsigned int t = 0; t < team_group[constraints_CA3[i].team_group_2_index].size(); t++)
      {
        t2 = team_group[constraints_CA3[i].team_group_2_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA3][t2][s].size(); j++)
          if (i == team_slot_constraints[CA3][t2][s][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA3][t2][s].push_back(i);
      }
    }
  }
  //CA4
  //per ogni constraint
  //per ogni slot presente nel vincolo
  //per ogni team presente nel team_group_1_index
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle CA4 agli indici [t,s]
  //ripeto per team_group_2_index
  for (unsigned int i = 0; i < constraints_CA4.size(); i++)
  {
    for (unsigned int s = 0; s < slot_group[constraints_CA4[i].slot_group_index].size(); s++)
    {
      s1 = slot_group[constraints_CA4[i].slot_group_index][s];
      for (unsigned int t = 0; t < team_group[constraints_CA4[i].team_group_1_index].size(); t++)
      {
        t1 = team_group[constraints_CA4[i].team_group_1_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA4][t1][s1].size(); j++)
          if (i == team_slot_constraints[CA4][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA4][t1][s1].push_back(i);
      }
      for (unsigned int t = 0; t < team_group[constraints_CA4[i].team_group_2_index].size(); t++)
      {
        t2 = team_group[constraints_CA4[i].team_group_2_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[CA4][t2][s1].size(); j++)
          if (i == team_slot_constraints[CA4][t2][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[CA4][t2][s1].push_back(i);
      }
    }
  }

  //GA1
  //per ogni slot presente nel vincolo
  //per ogni meeting
  //per ogni team del meeting
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle GA1 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_GA1.size(); i++)
    for (unsigned int s = 0; s < slot_group[constraints_GA1[i].slot_group_index].size(); s++)
    {
      s1 = slot_group[constraints_GA1[i].slot_group_index][s];
      for (unsigned int m = 0; m < constraints_GA1[i].meeting_group.size(); m++)
      {
        //per t1
        t1 = constraints_GA1[i].meeting_group[m].first;

        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[GA1][t1][s1].size(); j++)
          if (i == team_slot_constraints[GA1][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[GA1][t1][s1].push_back(i);

        //per t2
        t2 = constraints_GA1[i].meeting_group[m].second;
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[GA1][t2][s1].size(); j++)
          if (i == team_slot_constraints[GA1][t2][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[GA1][t2][s1].push_back(i);
      }
    }

  //BR1
  //per ogni constraint
  //per ogni team presente nel team_group_index
  //per ogni slot presente nel vincolo
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle BR1 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_BR1.size(); i++)
    for (unsigned int t = 0; t < team_group[constraints_BR1[i].team_group_index].size(); t++)
      for (unsigned int s = 0; s < slot_group[constraints_BR1[i].slot_group_index].size(); s++)
      {
        t1 = team_group[constraints_BR1[i].team_group_index][t];
        s1 = slot_group[constraints_BR1[i].slot_group_index][s];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[BR1][t1][s1].size(); j++)
          if (i == team_slot_constraints[BR1][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[BR1][t1][s1].push_back(i);

        // poiché questa constraint riguarda i break,
        // le costanti che valgono per la coppia [t,s1] possono venire
        // influenzate anche da cambi fatti al turno s1-1
        if (s1 != 0)
        {
          already_in_vector = false;
          for (unsigned int j = 0; j < team_slot_constraints[BR1][t1][s1 - 1].size(); j++)
            if (i == team_slot_constraints[BR1][t1][s1 - 1][j])
            {
              already_in_vector = true;
              break;
            }
          if (!already_in_vector)
            team_slot_constraints[BR1][t1][s1 - 1].push_back(i);
        }
      }

  //BR2
  //per ogni constraint
  //per ogni team presente nel team_group_index
  //per ogni slot presente nel vincolo
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle BR2 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_BR2.size(); i++)
    for (unsigned int t = 0; t < team_group[constraints_BR2[i].team_group_index].size(); t++)
      for (unsigned int s = 0; s < slot_group[constraints_BR2[i].slot_group_index].size(); s++)
      {
        t1 = team_group[constraints_BR2[i].team_group_index][t];
        s1 = slot_group[constraints_BR2[i].slot_group_index][s];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[BR2][t1][s1].size(); j++)
          if (i == team_slot_constraints[BR2][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[BR2][t1][s1].push_back(i);

        // poiché questa constraint riguarda i break,
        // le costanti che valgono per la coppia [t,s1] possono venire
        // influenzate anche da cambi fatti al turno s1-1
        if (s1 != 0)
        {
          already_in_vector = false;
          for (unsigned int j = 0; j < team_slot_constraints[BR2][t1][s1 - 1].size(); j++)
            if (i == team_slot_constraints[BR2][t1][s1 - 1][j])
            {
              already_in_vector = true;
              break;
            }
          if (!already_in_vector)
            team_slot_constraints[BR2][t1][s1 - 1].push_back(i);
        }
      }

  //FA2
  //per ogni constraint
  //per ogni team presente nel team_group_index
  //per ogni slot presente nel vincolo
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle FA2 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_FA2.size(); i++)
    for (unsigned int t = 0; t < team_group[constraints_FA2[i].team_group_index].size(); t++)
      for (unsigned int s = 0; s < slot_group[constraints_FA2[i].slot_group_index].size(); s++)
      {
        t1 = team_group[constraints_FA2[i].team_group_index][t];
        s1 = slot_group[constraints_FA2[i].slot_group_index][s];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[FA2][t1][s1].size(); j++)
          if (i == team_slot_constraints[FA2][t1][s1][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[FA2][t1][s1].push_back(i);
      }

  //SE1
  //per ogni constraint
  //per tutti gli slot (questo vincolo non è riferito a uno slot specifico)
  //per tutti i team in team_group_1_index
  //se non l'ho già aggiunta per quella coppia [t,s]
  //aggiungo un elemento alla matrice delle SE1 agli indici [t,s]
  for (unsigned int i = 0; i < constraints_SE1.size(); i++)
    for (unsigned int s = 0; s < slots.size(); s++)
    {
      for (unsigned int t = 0; t < team_group[constraints_SE1[i].team_group_index].size(); t++)
      {
        t1 = team_group[constraints_SE1[i].team_group_index][t];
        already_in_vector = false;
        for (unsigned int j = 0; j < team_slot_constraints[SE1][t1][s].size(); j++)
          if (i == team_slot_constraints[SE1][t1][s][j])
          {
            already_in_vector = true;
            break;
          }
        if (!already_in_vector)
          team_slot_constraints[SE1][t1][s].push_back(i);
      }
    }

  //Now that team_slot_constraints matrix has been filled in, I fill in the data team_constraints and slot_constraints
  //ADD to team_constraints
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
    for (unsigned int t = 0; t < teams.size(); t++)
      for (unsigned int s = 0; s < slots.size(); s++)
        for (unsigned int i = 0; i < team_slot_constraints[c_type][t][s].size(); i++)
        {
          already_in_vector = false;
          for (unsigned int j = 0; j < team_constraints[c_type][t].size(); j++)
            if (team_slot_constraints[c_type][t][s][i] == team_constraints[c_type][t][j])
            {
              already_in_vector = true;
              break;
            }
          if (!already_in_vector)
            team_constraints[c_type][t].push_back(team_slot_constraints[c_type][t][s][i]);
        }
  //ADD to team_constraints
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
    for (unsigned int s = 0; s < slots.size(); s++)
      for (unsigned int t = 0; t < teams.size(); t++)
        for (unsigned int i = 0; i < team_slot_constraints[c_type][t][s].size(); i++)
        {
          already_in_vector = false;
          for (unsigned int j = 0; j < slot_constraints[c_type][s].size(); j++)
            if (team_slot_constraints[c_type][t][s][i] == slot_constraints[c_type][s][j])
            {
              already_in_vector = true;
              break;
            }
          if (!already_in_vector)
            slot_constraints[c_type][s].push_back(team_slot_constraints[c_type][t][s][i]);
        }
}

unsigned int STT_Input::DispatchTeamIds(const string &team_ids)
{
  // transform the groups id into their indexes
  auto _team_group = split(team_ids, ";");
  std::vector<unsigned int> cur_team_group, cur_slot_group;
  transform(begin(_team_group), end(_team_group), back_inserter(cur_team_group),
            [this](const string &t_id)
            { return this->team_index[t_id]; });
  sort(begin(cur_team_group), end(cur_team_group));
  // search whether the team is already present or create it if not
  int team_group_index = SearchForGroup(team_group, cur_team_group);
  if (team_group_index == -1)
  {
    team_group_index = team_group.size();
    team_group.push_back(cur_team_group);
  }

  return team_group_index;
}

unsigned int STT_Input::DispatchSlotIds(const string &slot_ids)
{
  auto _slot_group = split(slot_ids, ";");
  std::vector<unsigned int> cur_slot_group;

  transform(begin(_slot_group), end(_slot_group), back_inserter(cur_slot_group),
            [this](const string &s_id)
            { return this->slot_index[s_id]; });
  sort(begin(cur_slot_group), end(cur_slot_group));
  // search whether the team is already present or create it if not
  int slot_group_index = SearchForGroup(slot_group, cur_slot_group);
  if (slot_group_index == -1)
  {
    slot_group_index = slot_group.size();
    slot_group.push_back(cur_slot_group);
  }

  return slot_group_index;
}

vector<pair<unsigned int, unsigned int>> STT_Input::DispatchMeetings(const string &meetings)
{
  auto _meetings_group = split(meetings, ";");
  std::vector<pair<unsigned int, unsigned int>> meeting_indexes;

  transform(begin(_meetings_group), end(_meetings_group), back_inserter(meeting_indexes),
            [this](const string &meeting)
            {
              auto indexes = split(meeting, ",");
              if (indexes.size() != 2)
                throw logic_error("Expecting meeting format is with comma separator, found " + meeting);
              return make_pair(this->team_index[indexes[0]], this->team_index[indexes[1]]);
            });
  sort(begin(meeting_indexes), end(meeting_indexes));

  return meeting_indexes;
}

bool STT_Input::IsHard(unsigned int c_type, unsigned int c) const
{
  switch (c_type)
  {
  case CA1:
    return constraints_CA1[c].hard;
  case CA2:
    return constraints_CA2[c].hard;
  case CA3:
    return constraints_CA3[c].hard;
  case CA4:
    return constraints_CA4[c].hard;
  case GA1:
    return constraints_GA1[c].hard;
  case BR1:
    return constraints_BR1[c].hard;
  case BR2:
    return constraints_BR2[c].hard;
  case FA2:
    return constraints_FA2[c].hard;
  case SE1:
    return constraints_SE1[c].hard;
  }
  return false;
}

unsigned int STT_Input::ConstraintsVectorSize(unsigned int c_type) const
{
  switch (c_type)
  {
  case CA1:
    return constraints_CA1.size();
  case CA2:
    return constraints_CA2.size();
  case CA3:
    return constraints_CA3.size();
  case CA4:
    return constraints_CA4.size();
  case GA1:
    return constraints_GA1.size();
  case BR1:
    return constraints_BR1.size();
  case BR2:
    return constraints_BR2.size();
  case FA2:
    return constraints_FA2.size();
  case SE1:
    return constraints_SE1.size();
  }
  return 0;
}

ostream &operator<<(ostream &os, const STT_Input &in)
{
  os << "Name: " << in.name << " (" << in.league_name << ")" << endl;
  os << "Game Mode: " << (in.phased ? "phased" : "non-phased") << endl;
  os << "Teams: ";
  copy(begin(in.teams), end(in.teams), ostream_iterator<Team>(os, ", "));
  os << std::endl;
  os << "Slots: ";
  copy(begin(in.slots), end(in.slots), ostream_iterator<Slot>(os, ", "));
  os << std::endl;
  return os;
}
