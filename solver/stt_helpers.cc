#include "stt_helpers.hh"
#include <pugixml.hpp>
#include <algorithm>
#include <numeric>
#include <functional>
#include <boost/coroutine2/coroutine.hpp>

#undef PARALLEL_GREEDY


#if defined(PARALLEL_GREEDY) && defined(TBB_AVAILABLE)
#include "tbb/tbb.h"
#endif

using namespace boost::coroutines2;

void STT_SolutionManager::RandomState(STT_Solution& st)
{
    st.CanonicalPattern(true, mix_initial_phase);
    st.display_OF_isset = display_OF;
    st.last_best_solution = st.CalculateFullCost();
    st.last_best_counter = st.move_counter;
}

std::vector<std::pair<unsigned int, unsigned int>> canonical_order(unsigned int teams)
{
  static unsigned int s_teams = 0;
  static std::vector<std::pair<unsigned int, unsigned int>> order;
  if (s_teams != teams)
  {
    order.resize(0);
    order.push_back(make_pair(0, 1));
    for (size_t i = 2; i <= teams / 2; i++)
      order.push_back(std::make_pair(i, teams - i + 1));
    s_teams = teams;
  }
  return order;
}

std::vector<std::pair<unsigned int, unsigned int>> random_order(unsigned int teams)
{
  static unsigned int s_teams = 0;
  static std::vector<std::pair<unsigned int, unsigned int>> order;
  if (s_teams != teams)
  {
    order.resize(0);
    order.push_back(make_pair(0, 1));
    std::vector<unsigned int> still_available(teams - 2);
    std::iota(begin(still_available), end(still_available), 2);
    std::shuffle(begin(still_available), end(still_available), Random::GetGenerator());
    while (!still_available.empty())
    {
      size_t i1 = Random::Uniform(static_cast<size_t>(0), still_available.size() - 1);
      unsigned int t1 = still_available[i1];
      still_available.erase(begin(still_available) + i1);
      size_t i2 = Random::Uniform(static_cast<size_t>(0), still_available.size() - 1);
      unsigned int t2 = still_available[i2];
      still_available.erase(begin(still_available) + i2);
      if (t1 < t2)
        order.push_back(std::make_pair(t1, t2));
      else
        order.push_back(std::make_pair(t2, t1));
      s_teams = teams;
    }
  }
  return order;
}

template <typename T>
T random_pick_from_set(std::vector<T> s)
{
  return *(s.begin() + Random::Uniform(static_cast<size_t>(0), s.size() - 1));
}

template <typename T>
void remove_from_set(std::vector<T>& s, T value)
{
  s.erase(std::remove(begin(s), end(s), value));
}

template <typename T>
void add_to_set(std::vector<T>& s, T value)
{
  for (auto it = s.begin(); it != s.end(); ++it)
  {
    if (*it < value)
      continue;
    if (*it == value)
      break;
    s.insert(it, value);
    return;
  }
  s.push_back(value);
}

std::list<std::pair<size_t, size_t>> chain(std::vector<std::pair<size_t, size_t>> edges, size_t n, const std::vector<int>& edge_color, std::pair<size_t, size_t> vertices, std::pair<int, int> colors)
{
  size_t current_v = vertices.first, next_v;
  int current_color = colors.second;
  std::list<std::pair<size_t, size_t>> path;
  bool chain_finished = false;
  while (!chain_finished)
  {
    chain_finished = true;
    for (size_t e = 0; e < edges.size(); e++)
    {
      if (edges[e].first >= n || edges[e].second >= n)
        continue;
      if (edges[e].first != current_v && edges[e].second != current_v) // this is an edge not incident to current_v
        continue;
      // here e is an edge incident to current_v
      if (edge_color[e] == current_color)
      {
        chain_finished = false;
        next_v = edges[e].first == current_v ? edges[e].second : edges[e].first;
        path.push_back(std::make_pair(e, next_v));
        current_v = next_v;
        current_color = current_color == colors.first ? colors.second : colors.first;
        break;
      }
    }
  }
  return path;
}

void color_edges(std::vector<std::pair<size_t, size_t>> edges, size_t n, std::vector<int>& edge_color)
{
  std::vector<size_t> unassigned_edges, subgraph_edges;
  std::vector<int> colors;
  std::generate_n(std::inserter(colors, end(colors)), (n % 2 == 0 ? n - 1 : n), [m = 0] () mutable { return m++; });
  std::vector<std::vector<int>> available_colors(n, colors);
  
  
  for (size_t e = 0; e < edges.size(); e++)
  {
    if (edges[e].first < n && edges[e].second < n)
    {
      subgraph_edges.push_back(e);
      if (edge_color[e] < 0)
        unassigned_edges.push_back(e);
      else
      {
        // remove used colors
        remove_from_set(available_colors[edges[e].first], edge_color[e]);
        remove_from_set(available_colors[edges[e].second], edge_color[e]);
      }
    }
  }
  int tabu = -1;
  size_t e0, v0, w, e1, v1;
  int phi, alpha0, beta;
  while (!unassigned_edges.empty())
  {
    if (tabu == -1)
    {
      e0 = random_pick_from_set(unassigned_edges);
      std::tie(v0, w) = edges[e0];
    }
    std::vector<int> available_common_colors;
    std::set_intersection(begin(available_colors[v0]), end(available_colors[v0]), begin(available_colors[w]), end(available_colors[w]), std::back_inserter(available_common_colors));
    if (!available_common_colors.empty())
    {
      phi = random_pick_from_set(available_common_colors);
      edge_color[e0] = phi;
      
      remove_from_set(available_colors[v0], phi);
      remove_from_set(available_colors[w], phi);
      tabu = -1;
    }
    else
    {
      do
      {
        alpha0 = random_pick_from_set(available_colors[v0]);
      }
      while (alpha0 == tabu);
      beta = random_pick_from_set(available_colors[w]);
      auto p = chain(edges, n, edge_color, std::make_pair(v0, w), std::make_pair(alpha0, beta));
      if (p.back().second != w) // the path does not end at w
      {
        // swap colors
        for (std::pair<size_t, size_t> e_s : p)
        {
          size_t e = e_s.first;
          edge_color[e] = edge_color[e] == alpha0 ? beta : alpha0;
        }
        edge_color[e0] = beta;
        remove_from_set(available_colors[v0], alpha0);
        remove_from_set(available_colors[w], beta);
        if (p.size() % 2 == 1)
        {
          add_to_set(available_colors[p.back().second], beta);
          remove_from_set(available_colors[p.back().second], alpha0);
        }
        else
        {
          add_to_set(available_colors[p.back().second], alpha0);
          remove_from_set(available_colors[p.back().second], beta);
        }
        tabu = -1;
      }
      else // the last is w
      {
        e1 = p.back().first;
        v1 = edges[e1].first == w ? edges[e1].second : edges[e1].first;
        edge_color[e1] = -1;
        edge_color[e0] = alpha0;
        remove_from_set(available_colors[v0], alpha0);
        add_to_set(available_colors[v1], alpha0);
        e0 = e1;
        v0 = v1;
        tabu = alpha0;
      }
    }
    unassigned_edges.clear();
    for (size_t e = 0; e < edges.size(); e++)
    {
      if (edges[e].first < n && edges[e].second < n)
      {
        if (edge_color[e] < 0)
          add_to_set(unassigned_edges, e);
      }
    }
  }
}

std::vector<std::vector<unsigned int>> compute_vizing_matches(std::vector<unsigned int> permutation)
{
  size_t teams = permutation.size();
  size_t rounds = teams - 1;
  std::vector<std::pair<size_t, size_t>> edges;
  for (size_t i = 0; i < teams - 1; i++)
  {
    for (size_t j = i + 1; j < teams; j++)
    {
      edges.emplace_back(i, j);
    }
  }
  std::vector<int> edge_color(edges.size(), -1);
  
  // seed color
  int c = 0;
  for (size_t e = 0; e < edges.size(); e++)
  {
    if (edges[e].first < 3 && edges[e].second < 3)
      edge_color[e] = c++;
  }
  for (size_t t = 4; t <= teams; t++)
  {
    color_edges(edges, t, edge_color);
    size_t colored = 0;
    for (size_t e1 = 0; e1 < edges.size() - 1; e1++)
    {
      if (edge_color[e1] >= 0)
        colored++;
      for (size_t e2 = e1 + 1; e2 < edges.size();  e2++)
      {
        if (edges[e1].first == edges[e2].first || edges[e1].first == edges[e2].second || edges[e1].second == edges[e2].first || edges[e1].second == edges[e2].second)
          assert((edge_color[e1] == -1 && edge_color[e2] == -1) || (edge_color[e1] != edge_color[e2]));
      }
    }
    if (edge_color[edges.size() - 1] >= 0)
      colored++;
    assert(colored == (t * (t - 1) / 2));
  }
  
  std::vector<vector<unsigned int>> candidate_tournament;
  std::vector<std::pair<unsigned int, unsigned int>> matches_order = canonical_order(teams);
  
  for (size_t r = 0; r < rounds; r++)
  {
    std::vector<size_t> current_round_matches;
    std::vector<unsigned int> current_round(teams, 0);
    for (size_t e = 0; e < edges.size(); e++)
    {
      if (edge_color[e] == static_cast<int>(r))
        current_round_matches.push_back(e);
    }
    for (size_t i = 0; i < matches_order.size(); i++)
    {
      unsigned int p1, p2;
      std::tie(p1, p2) = matches_order[i];
      current_round[p1] = permutation[edges[current_round_matches[i]].first];
      current_round[p2] = permutation[edges[current_round_matches[i]].second];
    }
    
    candidate_tournament.push_back(current_round);
  }
  
  return candidate_tournament;
}

std::vector<std::vector<unsigned int>> compute_polygon_matches(std::vector<unsigned int> permutation)
{
  size_t rounds;
  //teams = permutation.size();
  rounds = permutation.size() - 1;
  std::vector<vector<unsigned int>> candidate_tournament;
  for (unsigned int r = 0; r < rounds; ++r)
  {
    candidate_tournament.push_back(permutation);
    std::rotate(begin(permutation) + 1, begin(permutation) + 2, end(permutation));
  }
  return candidate_tournament;
}

void generator(coroutine<std::vector<bool>>::push_type& sink, std::vector<bool> flippable, size_t i)
{
  if (i > flippable.size())
    return;
  if (i == flippable.size())
    sink(flippable);
  else
  {
    generator(sink, flippable, i + 1);
    flippable[i] = !flippable[i];
    generator(sink, flippable, i + 1);
  }
}

void assign_candidate_at_round(coroutine<std::pair<STT_Solution, size_t>>::push_type& sink, unsigned int r, STT_Solution st, const std::vector<std::vector<unsigned int>>& candidate_tournament)
{
  std::vector<std::pair<unsigned int, unsigned int>> matches_order = canonical_order(st.in.teams.size());

  for (size_t t = 0; t < candidate_tournament.size(); t++)
  {
    const std::vector<unsigned int>& candidate = candidate_tournament[t];
    std::vector<unsigned int> free_ha_matches;
    std::vector<bool> home_away;

    for (size_t i = 0; i < matches_order.size(); i++)
    {
      unsigned int p1, p2;
      std::tie(p1, p2) = matches_order[i];
      unsigned int t1 = candidate[p1], t2 = candidate[p2];
      
      st.opponent[t1][r] = t2;
      st.opponent[t2][r] = t1;
      if (st.match[t1][t2] < r)
      {
        // the match t1->t2 was already held, therefore the only possibility is t2->t1
        st.home[t2][r] = true;
        st.home[t1][r] = false;
        st.match[t2][t1] = r;
      }
      else if (st.match[t2][t1] < r)
      {
        // the match t2->t1 was already held, therefore the only possibility is t1->t2
        st.home[t1][r] = true;
        st.home[t2][r] = false;
        st.match[t1][t2] = r;
      }
      else
      {
        free_ha_matches.push_back(i);
        home_away.push_back(false);
      }
    }
    if (free_ha_matches.size() == 0)
      // there is no possibility to change home/away patterns because of previous assignments
      sink(make_pair(st, t)); // yield the state
    else
    {
      coroutine<std::vector<bool>>::pull_type source(std::bind(generator, std::placeholders::_1, home_away, 0));
      for (auto f : source)
      {
        for (size_t j = 0; j < f.size(); j++)
        {
          unsigned int p1, p2;
          std::tie(p1, p2) = matches_order[free_ha_matches[j]];
          unsigned int t1 = candidate[p1], t2 = candidate[p2];
          if (f[j])
          {
            st.home[t1][r] = true;
            st.home[t2][r] = false;
            st.match[t1][t2] = r;
          }
          else
          {
            st.home[t2][r] = true;
            st.home[t1][r] = false;
            st.match[t2][t1] = r;
          }
        }
        sink(make_pair(st, t)); // yield the current state
        // go back to the previous situation (to avoid problems)
        for (size_t j = 0; j < f.size(); j++)
        {
          unsigned int p1, p2;
          std::tie(p1, p2) = matches_order[free_ha_matches[j]];
          unsigned int t1 = candidate[p1], t2 = candidate[p2];
          if (f[j])
            st.match[t1][t2] = st.in.slots.size();
          else
            st.match[t2][t1] = st.in.slots.size();
        }
      }
    }
    // undo assignment of fixed slots
    for (size_t i = 0; i < matches_order.size(); i++)
    {
      unsigned int p1, p2;
      std::tie(p1, p2) = matches_order[i];
      unsigned int t1 = candidate[p1], t2 = candidate[p2];
      
      if (st.match[t1][t2] < r)
      {
        st.match[t2][t1] = st.in.slots.size();
      }
      else if (st.match[t2][t1] < r)
      {
        st.match[t1][t2] = st.in.slots.size();
      }
    }
  }
}
  
void STT_SolutionManager::GreedyState(STT_Solution& st)
{
  std::vector<unsigned int> permutation(st.in.teams.size());
  std::iota(begin(permutation), end(permutation), 0);
  std::shuffle(begin(permutation), end(permutation), Random::GetGenerator());
  std::vector<vector<unsigned int>> candidate_tournament_first_leg, candidate_tournament_second_leg;
  // get two candidate tournaments, one for each leg and just a single matches ordering (it is irrelevant which one to use)
  if (vizing_greedy)
    candidate_tournament_first_leg = compute_vizing_matches(permutation);
  else
    candidate_tournament_first_leg = compute_polygon_matches(permutation);
  std::shuffle(begin(permutation), end(permutation), Random::GetGenerator());
  if (vizing_greedy)
    candidate_tournament_second_leg = compute_vizing_matches(permutation);
  else
    candidate_tournament_second_leg = compute_polygon_matches(permutation);
  st.match = std::vector<std::vector<unsigned int>>(in.teams.size(), std::vector<unsigned int>(in.teams.size(), in.slots.size()));
    
  if (st.in.phased)
  {
    for (unsigned int r = 0; r < st.in.teams.size() - 1; ++r)
    {
      float best_cost = std::numeric_limits<float>::infinity();
      STT_Solution best_solution(st.in);
      size_t best_index = st.in.teams.size() - 1;
      coroutine<std::pair<STT_Solution, size_t>>::pull_type source(std::bind(assign_candidate_at_round, std::placeholders::_1, r, st, candidate_tournament_first_leg));
#if defined(TBB_AVAILABLE) && defined(PARALLEL_GREEDY)
)
      tbb::spin_mutex mx_best_state;
      tbb::parallel_for_each(begin(source), end(source), [r, &best_cost, &best_solution, &best_index, &mx_best_state](const auto& state) {
        float current_cost = state.first.GreedyCalculateCost(r);
        tbb::spin_mutex::scoped_lock  lock(mx_best_state);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      });
#else
      for (auto state : source)
      {
        float current_cost = state.first.GreedyCalculateCost(r);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      }
#endif
      st = best_solution;
      //      std::cout << st << static_cast<int>(best_cost) << std::endl << best_index << std::endl;
      candidate_tournament_first_leg.erase(begin(candidate_tournament_first_leg) + best_index);
    }
    // second leg
    for (unsigned int r = st.in.teams.size() - 1; r < st.in.slots.size(); ++r)
    {
      float best_cost = std::numeric_limits<float>::infinity();
      STT_Solution best_solution(st.in);
      size_t best_index = st.in.slots.size();
      coroutine<std::pair<STT_Solution, size_t>>::pull_type source(std::bind(assign_candidate_at_round, std::placeholders::_1, r, st, candidate_tournament_second_leg));
#if defined(TBB_AVAILABLE) && defined(PARALLEL_GREEDY)
      tbb::spin_mutex mx_best_state;
      tbb::parallel_for_each(begin(source), end(source), [r, &best_cost, &best_solution, &best_index, &mx_best_state](const auto& state) {
        float current_cost = state.first.GreedyCalculateCost(r);
        tbb::spin_mutex::scoped_lock  lock(mx_best_state);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      });
#else
      for (auto state : source)
      {
        float current_cost = state.first.GreedyCalculateCost(r);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      }
#endif
      st = best_solution;
//      std::cout << st << static_cast<int>(best_cost) << std::endl  << best_index << std::endl;
      candidate_tournament_second_leg.erase(begin(candidate_tournament_second_leg) + best_index);
    }
  }
  else
  {
    std::vector<std::vector<unsigned int>> candidate_tournament;
    candidate_tournament.insert(end(candidate_tournament), begin(candidate_tournament_first_leg), end(candidate_tournament_first_leg));
    candidate_tournament.insert(end(candidate_tournament), begin(candidate_tournament_second_leg), end(candidate_tournament_second_leg));
    for (unsigned int r = 0; r < st.in.slots.size(); ++r)
    {
      float best_cost = std::numeric_limits<float>::infinity();
      STT_Solution best_solution(st.in);
      size_t best_index = candidate_tournament.size();
      coroutine<std::pair<STT_Solution, size_t>>::pull_type source(std::bind(assign_candidate_at_round, std::placeholders::_1, r, st, candidate_tournament));
#if defined(TBB_AVAILABLE) && defined(PARALLEL_GREEDY)
      tbb::spin_mutex mx_best_state;
      tbb::parallel_for_each(begin(source), end(source), [r, &best_cost, &best_solution, &best_index, &mx_best_state](const auto& state) {
        float current_cost = state.first.GreedyCalculateCost(r);
        tbb::spin_mutex::scoped_lock  lock(mx_best_state);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      });
#else
      for (auto state : source)
      {
        float current_cost = state.first.GreedyCalculateCost(r);
        if (current_cost < best_cost || (current_cost == best_cost && Random::Uniform(0, 1)))
        {
          best_solution = state.first;
          best_cost = current_cost;
          best_index = state.second;
        }
      }
#endif
      st = best_solution;
//      std::cout << st << static_cast<int>(best_cost) << std::endl;
      candidate_tournament.erase(begin(candidate_tournament) + best_index);
    }
  }
  st.PopulateIsReturnMatrix();
  st.CalculateFullCost();
  st.display_OF_isset = display_OF;
  st.last_best_solution = st.CalculateFullCost();
  st.last_best_counter = st.move_counter;
}

bool STT_SolutionManager::CheckConsistency(const STT_Solution& st) const
{
    return st.InternalCheckConsistency();
}

void STT_SolutionManager::PrettyPrintOutput(const STT_Solution& st, std::string filename) const
{
    ofstream os(filename);

    vector<vector<pair<unsigned,unsigned>>> matches(in.slots.size(), vector<pair<unsigned,unsigned>>(0));

    //se voglio salvare un file XML
    if(filename.size()>4 && filename[filename.size()-1] == 'l' &&  filename[filename.size()-2] == 'm'  && filename[filename.size()-3] == 'x')
    {
      pugi::xml_document doc;
    
      pugi::xml_node root = doc.append_child("Solution");
      pugi::xml_node metadata = root.append_child("MetaData");
      metadata.append_child("InstanceName").append_child(pugi::node_pcdata).set_value(st.in.name.c_str());
      metadata.append_child("SolutionName").append_child(pugi::node_pcdata).set_value(("EasyLocal" + st.in.name).c_str());
      // TODO: set the cost values
      // however, since the validator considers this section, it would be meaningful only for double checking at some later point
  #ifdef OBJECTIVE_VALIDATION
      pugi::xml_node objective_value = metadata.append_child("ObjectiveValue");
      objective_value.append_attribute("infeasibility").set_value(0);
      objective_value.append_attribute("objective").set_value(0);
  #endif
      pugi::xml_node games = root.append_child("Games");
      for (size_t s = 0; s < st.in.slots.size(); s++)
      {
          for (size_t t = 0; t < st.in.teams.size(); t++)
          {
              if (st.home[t][s])
              {
                  pugi::xml_node game = games.append_child("ScheduledMatch");
                  game.append_attribute("home").set_value(st.in.teams[t].id.c_str());
                  game.append_attribute("away").set_value(st.in.teams[st.opponent[t][s]].id.c_str());
                  game.append_attribute("slot").set_value(st.in.slots[s].id.c_str());
              }
          }
      }
      doc.print(os);
    }
    else //se è un normale file di testo
    {
      //riempio la matrice matches
      for(unsigned s=0; s<in.slots.size(); s++)
        for(unsigned t=0; t<in.teams.size(); t++)
        {
          if(t < st.opponent[t][s]) //per non stampare due volte la stessa partita in un turno
          {
            if(st.home[t][s])
              matches[s].push_back(pair<unsigned,unsigned>(t,st.opponent[t][s]));
            else
              matches[s].push_back(pair<unsigned,unsigned>(st.opponent[t][s],t));
          }
        }

      //adesso stampo su file
      for(unsigned i=0; i<in.teams.size()/2;i++)
      {
        for(unsigned s=0; s<in.slots.size(); s++)
          {
            os << matches[s][i].first << "-" << matches[s][i].second;
            //metto adesso il numero di spazi appropriato
            if(matches[s][i].first<10 && matches[s][i].second<10)
              os << "   ";
            else if((matches[s][i].first<10 && matches[s][i].second>=10) || (matches[s][i].first>=10 && matches[s][i].second<10))
              os << "  ";
            else if(matches[s][i].first>=10 && matches[s][i].second>=10)
              os << " ";
          }
        os << endl;
      }
    } 
}

bool STT_SolutionManager::OptimalStateReached(const STT_Solution& st) const
{
  if (st.in.stop_at_zero_hard)
    return st.total_cost_components_hard == 0 && st.cost_phased == 0;
  
  return false;
}

int CA1CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[CA1];
}

void CA1CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_CA1)
    {
        for (auto t : in.team_group[c.team_group_index])
        {
            int games = 0;
            for (auto s : in.slot_group[c.slot_group_index])
            {
                if (c.mode == HOME && st.home[t][s])
                    games++;
                else if (c.mode == AWAY && !st.home[t][s])
                    games++;
            }
            if (games < c.k_min)
            {
                os << "CA1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at least " << c.k_min << " " << PrintMode(c.mode) <<
                " games of team " << in.teams[t].id << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
            else if (games > c.k_max)
            {
                os << "CA1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at most " << c.k_max << " " << PrintMode(c.mode) <<
                " games of team " << in.teams[t].id << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
            
        }
    }
}

int CA2CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[CA2];
}

void CA2CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_CA2)
    {
        for (auto t1 : in.team_group[c.team_group_1_index])
        {
            int games = 0;
            for (auto t2 : in.team_group[c.team_group_2_index])
                for (auto s : in.slot_group[c.slot_group_index])
                {
                    if (st.opponent[t1][s] != t2)
                        continue;
                    if (c.mode == HOME && st.home[t1][s])
                        games++;
                    else if (c.mode == AWAY && !st.home[t1][s])
                        games++;
                    else if (c.mode == ANY)
                        games++;
                }
            if (games < c.k_min)
            {
                os << "CA2" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at least " << c.k_min << " " << PrintMode(c.mode) <<
                " games of team " << in.teams[t1].id << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
            else if (games > c.k_max)
            {
                os << "CA2" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at most " << c.k_max << " " << PrintMode(c.mode) <<
                " games of team " << in.teams[t1].id << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
            
        }
    }
}

int CA3CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[CA3];
}

void CA3CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_CA3)
    {
        for (auto t1 : in.team_group[c.team_group_1_index])
        {
            vector<int> games(in.slots.size(), 0);
            for (auto t2 : in.team_group[c.team_group_2_index])
            {
                for (size_t s = 0; s < in.slots.size(); s++)
                {
                    if (st.opponent[t1][s] != t2)
                        continue;
                    if (c.mode == HOME && st.home[t1][s])
                        games[s]++;
                    else if (c.mode == AWAY && !st.home[t1][s])
                        games[s]++;
                    else if (c.mode == ANY)
                        games[s]++;
                }
            }
            for (size_t s = 0; s + c.k - 1 < in.slots.size(); s++)
            {
                int total = 0;
                for (int i = 0; i < c.k; i++)
                    total += games[s + i];
                if (total < c.k_min)
                {
                    os << "CA3" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - total, total - c.k_max)) << "]: expected at least " << c.k_min << " " << PrintMode(c.mode) <<
                    " games of team " << in.teams[t1].id << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                    << " in " << c.k << " consecutive time slots "
                    << "found " << total << " from " << s << " to " << s + c.k - 1;
                    if(in.phased) //se è un'istanza phased
                    {
                      os << " ## Constraint among both phases: ";
                      if(c.both_phases)
                        os << "YES";
                      else
                        os << "NO";
                    }
                    os << endl;
                }
                else if (total > c.k_max)
                {
                    os << "CA3" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - total, total - c.k_max)) << "]: expected at most " << c.k_max << " " << PrintMode(c.mode) <<
                    " games of team " << in.teams[t1].id << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                    << " in " << c.k << " consecutive time slots "
                    << "found " << total << " from " << s << " to " << s + c.k - 1;
                    if(in.phased) //se è un'istanza phased
                    {
                      os << " ## Constraint among both phases: ";
                      if(c.both_phases)
                        os << "YES";
                      else
                        os << "NO";
                    }
                    os << endl;
                }
            }
        }
    }
}

int CA4CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[CA4];
}

void CA4CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    
    for (const auto& c : in.constraints_CA4)
    {
        if (c.mode2 == CA4Spec::GLOBAL)
        {
            int games = 0;
            for (auto t1 : in.team_group[c.team_group_1_index])
            {
                for (auto t2 : in.team_group[c.team_group_2_index])
                    for (auto s : in.slot_group[c.slot_group_index])
                    {
                        if (st.opponent[t1][s] != t2)
                            continue;
                        if (c.mode1 == HOME && st.home[t1][s])
                            games++;
                        else if (c.mode1 == AWAY && !st.home[t1][s])
                            games++;
                        else if (c.mode1 == ANY)
                            games++;
                    }
            }
            if (games < c.k_min)
            {
                os << "CA4" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: " << " (GLOBAL mode) " << "  expected at least " << c.k_min << " " << PrintMode(c.mode1) <<
                " games of teams " << in.PrintTeams(in.team_group[c.team_group_1_index]) << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
            else if (games > c.k_max)
            {
                os << "CA4" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: " << " (GLOBAL mode) " << " expected at most " << c.k_max << " " << PrintMode(c.mode1) <<
                " games of teams " << in.PrintTeams(in.team_group[c.team_group_1_index]) << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << games;
                if(in.phased) //se è un'istanza phased
                {
                  os << " ## Constraint among both phases: ";
                  if(c.both_phases)
                    os << "YES";
                  else
                    os << "NO";
                }
                os << endl;
            }
        }
        else // (c.mode2 == CA4Spec::EVERY)
        {
            for (auto s : in.slot_group[c.slot_group_index])
            {
                int games = 0;
                for (auto t1 : in.team_group[c.team_group_1_index])
                {
                    for (auto t2 : in.team_group[c.team_group_2_index])
                    {
                        if (st.opponent[t1][s] != t2)
                            continue;
                        if (c.mode1 == HOME && st.home[t1][s])
                            games++;
                        else if (c.mode1 == AWAY && !st.home[t1][s])
                            games++;
                        else if (c.mode1 == ANY)
                            games++;
                    }
                }
                if (games < c.k_min)
                {
                    os << "CA4" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: " << " (EVERY mode) " << " expected at least " << c.k_min << " " << PrintMode(c.mode1) <<
                    " games of teams " << in.PrintTeams(in.team_group[c.team_group_1_index]) << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                    << " at time slot " << s  
                    << " found " << games;
                    if(in.phased) //se è un'istanza phased
                    {
                      os << " ## Constraint among both phases: ";
                      if(c.both_phases)
                        os << "YES";
                      else
                        os << "NO";
                    }
                    os << endl;
                }
                else if (games > c.k_max)
                {
                    os << "CA4" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: " << " (EVERY mode) " << " expected at most " << c.k_max << " " << PrintMode(c.mode1) <<
                    " games of teams " << in.PrintTeams(in.team_group[c.team_group_1_index]) << " against teams " << in.PrintTeams(in.team_group[c.team_group_2_index])
                    << " at time slot " << s 
                    << " found " << games;
                    if(in.phased) //se è un'istanza phased
                    {
                      os << " ## Constraint among both phases: ";
                      if(c.both_phases)
                        os << "YES";
                      else
                        os << "NO";
                    }
                    os << endl;
                }
            }
        }
    }
}

int GA1CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[GA1];
}

void GA1CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_GA1)
    {
        int games = 0;
        for (auto m : c.meeting_group)
        {
            for (auto s : in.slot_group[c.slot_group_index])
            {
                if (!st.home[m.first][s])
                    continue;
                if (st.opponent[m.first][s] == m.second)
                    games++;
            }
        }
        if (games < c.k_min)
        {
            os << "GA1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at least " << c.k_min << " of the games " <<
            in.PrintMeetings(c.meeting_group) << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
            << " found " << games;
            if(in.phased) //se è un'istanza phased
            {
              os << " ## Constraint among both phases: ";
              if(c.both_phases)
                os << "YES";
              else
                os << "NO";
            }
            os << endl;  
        }
        else if (games > c.k_max)
        {
            os << "GA1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.k_min - games, games - c.k_max)) << "]: expected at most " << c.k_max << " of the games " <<
            in.PrintMeetings(c.meeting_group) << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
            << " found " << games;
            if(in.phased) //se è un'istanza phased
            {
              os << " ## Constraint among both phases: ";
              if(c.both_phases)
                os << "YES";
              else
                os << "NO";
            }
            os << endl;
        }
    }
}

int BR1CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[BR1];
}

void BR1CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_BR1)
    {
        for (auto t : in.team_group[c.team_group_index])
        {
            int breaks = 0;
            for (auto s : in.slot_group[c.slot_group_index])
            {
                if (c.mode == HOME && s > 0 && st.home[t][s - 1] && st.home[t][s])
                    breaks++;
                else if (c.mode == AWAY && s > 0 && !st.home[t][s - 1] && !st.home[t][s - 1])
                    breaks++;
                else if (c.mode == ANY && s > 0 && st.home[t][s - 1] == st.home[t][s])
                    breaks++;
            }
            if (breaks > c.k)
            {
                os << "BR1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, breaks - c.k) << "]: expected at most " << c.k << " breaks of team " << in.teams[t] << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
                << " found " << breaks;
                if(in.phased) //se è un'istanza phased
              {
                os << " ## Constraint among both phases: ";
                if(c.both_phases)
                  os << "YES";
                else
                  os << "NO";
              }
              os << endl;
            }
        }
    }
}

// FIXME: assumption, the breaks are considered with respect to the previous time-slot, to be checked since for BR2 there's an example with slot 0

int BR2CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[BR2];
}

void BR2CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_BR2)
    {
        int breaks = 0;
        for (auto t : in.team_group[c.team_group_index])
        {
            for (auto s : in.slot_group[c.slot_group_index])
            {
                // the only considered mode is ANY
                if (s > 0 && st.home[t][s - 1] == st.home[t][s])
                    breaks++;
            }
        }
        if (breaks > c.k)
        {
            os << "BR2" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, breaks - c.k) << "]: expected at most " << c.k << " overall breaks of teams " << in.PrintTeams(in.team_group[c.team_group_index]) << " in time slots " << in.PrintSlots(in.slot_group[c.slot_group_index])
            << " found " << breaks;
            if(in.phased) //se è un'istanza phased
            {
              os << " ## Constraint among both phases: ";
              if(c.both_phases)
                os << "YES";
              else
                os << "NO";
            }
            os << endl;
        }
    }
}

int FA2CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[FA2];
}

void FA2CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_FA2)
    {
        for (size_t i = 0; i < in.team_group[c.team_group_index].size() - 1; i++)
            for (size_t j = i + 1; j < in.team_group[c.team_group_index].size(); j++)
            {
                unsigned int t1 = in.team_group[c.team_group_index][i], t2 = in.team_group[c.team_group_index][j];
                int home_games_t1 = 0, home_games_t2 = 0;
                int current_home_games_difference = 0;
                int max_home_games_difference = 0;
                for (auto s : in.slot_group[c.slot_group_index])
                {
                    // the only considered mode is HOME
                    home_games_t1 += st.home[t1][s];
                    home_games_t2 += st.home[t2][s];

                    current_home_games_difference = fabs(home_games_t1 - home_games_t2);
                    if(current_home_games_difference > max_home_games_difference)
                        max_home_games_difference = current_home_games_difference;
                }
                os << "FA2" << (c.hard ? "*" : "") << "[" 
                    << c.penalty * max(0, max_home_games_difference - c.k) 
                    << "]: expected at most " << c.k  << " home game differences of teams " 
                    << in.teams[t1] << " and " << in.teams[t2] 
                    << " found " << max_home_games_difference; 
                    if(in.phased) //se è un'istanza phased
                    {
                      os << " ## Constraint among both phases: ";
                      if(c.both_phases)
                        os << "YES";
                      else
                        os << "NO";
                    }
                    os << endl;
                
            }
    }
}

int SE1CostComponent::ComputeCost(const STT_Solution& st) const
{
    return st.cost_components[SE1];
}

void SE1CostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for (const auto& c : in.constraints_SE1)
    {
        for (size_t i = 0; i < in.team_group[c.team_group_index].size() - 1; i++)
            for (size_t j = i + 1; j < in.team_group[c.team_group_index].size(); j++)
            {
                unsigned int t1 = in.team_group[c.team_group_index][i], t2 = in.team_group[c.team_group_index][j];
                int first = -1, second = -1;
                for (size_t s = 0; s < in.slots.size(); s++)
                {
                    // the only considered mode is SLOTS
                    if (st.opponent[t1][s] == t2 && first == -1)
                        first = s;
                    else if (st.opponent[t1][s] == t2)
                    {
                        second = s;
                        break;
                    }
                }
                int distance = second - (first + 1);
                if (distance < c.m_min)
                {
                    os << "SE1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.m_min - distance, distance - c.m_max)) << "]: expected at least " << c.m_min << " separation between games of teams " << in.teams[t1].id << " and " << in.teams[t2].id << " at " << first << " and " << second
                    << " found " << distance << endl;

                }
                if (distance > c.m_max)
                {
                    os << "SE1" << (c.hard ? "*" : "") << "[" << c.penalty * max(0, max(c.m_min - distance, distance - c.m_max)) << "]: expected at most " << c.m_max << " separation between games of teams " << in.teams[t1].id << " and " << in.teams[t2].id << " at " << first << " and " << second
                    << " found " << distance << endl;
                }
            }
    }
}

int PhasedCostComponent::ComputeCost(const STT_Solution& st) const
{
  return st.cost_phased;
}

void PhasedCostComponent::PrintViolations(const STT_Solution& st, std::ostream& os) const
{
    for(unsigned int i = 0; i < st.in.teams.size(); i++)
    {
      for(unsigned int j = i+1; j < st.in.teams.size(); j++)
      {
        if(st.SamePhase(st.match[i][j], st.match[j][i]))
        {
          os << "matches between teams " << i << " and " << j << " in the same phase, slots : " << st.match[i][j] << ", " << st.match[j][i] << endl;
        }
      }
    }
}



/***************************************************************************
 * 1  METHODS FOR STT_SwapHomes Neighborhood Explorer:
 ***************************************************************************/

STT_SwapHomesNeighborhoodExplorer::STT_SwapHomesNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
  : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapHomes>(in,sm,"STT_SwapHomesNeighborhoodExplorer")
{
}
void STT_SwapHomesNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapHomes& m) const
{
  m.t1 = Random::Uniform<int>(0,in.teams.size()-1);
  do 
    m.t2 = Random::Uniform<int>(0,in.teams.size()-1);
  while (m.t1 == m.t2);
  if (m.t1 > m.t2)
    swap(m.t1,m.t2);
}

void STT_SwapHomesNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapHomes& m) const
{
  unsigned int contatore = 0;
  if(st.in.forbid_hard_worsening_moves)
  {
    do
    {
      if(contatore > 100)
      {
        throw EmptyNeighborhood();
      }
      AnyRandomMove(st,m);
      contatore ++;
    } while (!FeasibleMove(st,m));
  }
  else
  {
    AnyRandomMove(st,m);
  }
}

bool STT_SwapHomesNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapHomes& m) const
{
  if(m.t1 < m.t2)
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      ExecuteMove(st_copy, m);
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

void STT_SwapHomesNeighborhoodExplorer::ExecuteMove(STT_Solution& st,const STT_SwapHomes& m) const
{
  st.InternalMakeSwapHomes(m.t1, m.t2);
}

void STT_SwapHomesNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapHomes& m) const
{   
  vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
  bool already_in_vector;


  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {

    // [m.t1][match[m.t1][m.t2]]
    involved_constraints[c_type] = in.team_slot_constraints[c_type][m.t1][st.match[m.t1][m.t2]];


    // [m.t2][match[m.t1][m.t2]]
    for (unsigned int i = 0; i<in.team_slot_constraints[c_type][m.t2][st.match[m.t1][m.t2]].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t2][st.match[m.t1][m.t2]][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
        involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t2][st.match[m.t1][m.t2]][i]);
    }

    // [m.t1][match[m.t2][m.t1]]
    for (unsigned int i = 0; i<in.team_slot_constraints[c_type][m.t1][st.match[m.t2][m.t1]].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t1][st.match[m.t2][m.t1]][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
        involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t1][st.match[m.t2][m.t1]][i]);
    }

    // [m.t2][match[m.t2][m.t1]]
    for (unsigned int i = 0; i<in.team_slot_constraints[c_type][m.t2][st.match[m.t2][m.t1]].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t2][st.match[m.t2][m.t1]][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
        involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t2][st.match[m.t2][m.t1]][i]);
    }
  }

  ExecuteMove(st, m);

  st.UpdateSelectionedCostsConstraints(involved_constraints);
  st.CalculateCostPhased();


  st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());
  if(st.display_OF_isset)
  {
    st.DisplayOFIfNeeded();
  }
}

void STT_SwapHomesNeighborhoodExplorer::FirstMove(const STT_Solution& st, STT_SwapHomes& m) const
{
  m.t1 = 0; 
  m.t2 = 1;
}
bool STT_SwapHomesNeighborhoodExplorer::NextMove(const STT_Solution& st, STT_SwapHomes& m) const
{
  if (m.t2 < in.teams.size() - 1) 
    {
      m.t2++;
      return true;
    }
  else if (m.t1 < in.teams.size() - 2)
    { 
      m.t1++; 
      m.t2 = m.t1 + 1; 
      return true;
    }
  else 
    return false;
}

/***************************************************************************
 * 2 METHODS FOR STT_SwapTeams Neighborhood Explorer:
 ***************************************************************************/

STT_SwapTeamsNeighborhoodExplorer::STT_SwapTeamsNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
    : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapTeams>(in,sm,"SwapTeamsNeighborhoodExplorer")
{
}
  
void STT_SwapTeamsNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapTeams& m) const
{
  m.t1 = Random::Uniform<int>(0,in.teams.size()-1);
  do 
    m.t2 = Random::Uniform<int>(0,in.teams.size()-1);
  while (m.t1 == m.t2);
  if (m.t1 > m.t2)
    swap(m.t1,m.t2);
}

void STT_SwapTeamsNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapTeams& m) const
{
  unsigned int contatore = 0;
  if(st.in.forbid_hard_worsening_moves)
  {
    do
    {
      if(contatore > 100)
      {
        throw EmptyNeighborhood();
      }
      AnyRandomMove(st,m);
      contatore ++;
    } while (!FeasibleMove(st,m));
  }
  else
  {
    AnyRandomMove(st,m);
  }
}

bool STT_SwapTeamsNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapTeams& m) const
{
  if(m.t1 < m.t2)
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      //SIMULO LA MOSSA
      ExecuteMove(st_copy, m);
      //FINE SUMULAZIONE MOSSA
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

void STT_SwapTeamsNeighborhoodExplorer::ExecuteMove(STT_Solution& st,const STT_SwapTeams& m) const
{
  unsigned r;

  for(r = 0; r < in.slots.size(); r++)
    //      if(st.opp[m.t1][r] != m.t2) // not the round of the t1-t2 match  // REMOVED 20-7-2005
    st.UpdateMatches(m.t1,m.t2,r);
  
  for(r = 0; r < in.slots.size(); r++)
  {
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t1,r);
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t2,r);
  }
}                            


void STT_SwapTeamsNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapTeams& m) const                                  
{
  ExecuteMove(st, m);

  //Carico in un vettore tutte le constraints da ricalcolare, 
  //per STT_SwapTeams carichiamo le constraints relative ai team t1 e t2, a tutti gli slots
  //NOTA: forse dovrei considerare anche gli oppoentni di t1 e t2 nei relativi slots in cui viene effettuato 
  //lo scambio? Risposta: NO, non è necessario
  vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
  bool already_in_vector;
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    involved_constraints[c_type] = in.team_constraints[c_type][m.t1];
    for (unsigned int i = 0; i<in.team_constraints[c_type][m.t2].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.team_constraints[c_type][m.t2][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
        involved_constraints[c_type].push_back(in.team_constraints[c_type][m.t2][i]);
    }
  }

  st.UpdateSelectionedCostsConstraints(involved_constraints);
  st.CalculateCostPhased();


  st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());
  if(st.display_OF_isset)
  {
    st.DisplayOFIfNeeded();
  }
}

void STT_SwapTeamsNeighborhoodExplorer::FirstMove(const STT_Solution& st, STT_SwapTeams& m) const
{ 
  m.t1 = 0; 
  m.t2 = 1; 
}

bool STT_SwapTeamsNeighborhoodExplorer::NextMove(const STT_Solution& st, STT_SwapTeams& m) const   
{
  if (m.t2 < in.teams.size() - 1) 
    {
      m.t2++;
      return true;
    }
  else if (m.t1 < in.teams.size() - 2)
    { 
      m.t1++; 
      m.t2 = m.t1 + 1; 
      return true;
    }
  else
    return false;      
}


/***************************************************************************
 * 3 METHODS FOR STT_SwapRounds Neighborhood Explorer:
 ***************************************************************************/

STT_SwapRoundsNeighborhoodExplorer::STT_SwapRoundsNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
  : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapRounds>(in,sm,"STT_SwapRoundsNeighborhoodExplorer")
{
}

void STT_SwapRoundsNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapRounds& m) const
{
    m.r1 = Random::Uniform<int>(0,in.slots.size()-1);

    //NOTA: se sono nel caso in cui non voglio mischiare le fasi
    if(!st.in.mix_phase_during_search)
    {
      //estraggo un valore solo sulla STESSA metà slots
      if (m.r1 < in.slots.size()/2)  // first leg
        {
          m.r2 = Random::Uniform<int>(0,in.slots.size()/2-2);
          if (m.r2 >= m.r1)
            m.r2++;
        }
      else
        {
          m.r2 = Random::Uniform<int>(in.slots.size()/2, in.slots.size() - 2);
          if (m.r2 >= m.r1)
            m.r2++;
        }
    }
    else
    {
       m.r2 = Random::Uniform<int>(0,in.slots.size()-2);
       if (m.r2 >= m.r1)
          m.r2++;
    }

    if (m.r1 > m.r2)
      swap(m.r1,m.r2);
}

void STT_SwapRoundsNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapRounds& m) const
{
  unsigned int contatore = 0;
  if(st.in.forbid_hard_worsening_moves)
  {
    do
    {
      if(contatore > 100)
      {
        throw EmptyNeighborhood();
      }
      AnyRandomMove(st,m);
      contatore ++;
    } while (!FeasibleMove(st,m));
  }
  else
  {
    AnyRandomMove(st,m);
  } 
}

bool STT_SwapRoundsNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapRounds& m) const
{
  if (m.r1 < m.r2)
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      //SIMULO LA MOSSA
      ExecuteMove(st_copy, m);
      //FINE SUMULAZIONE MOSSA
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }

  return false;
}

void STT_SwapRoundsNeighborhoodExplorer::ExecuteMove(STT_Solution& st,const STT_SwapRounds& m) const
{
  unsigned t, t1, t2;
  bool b1, b2;
  for (t = 0; t < in.teams.size(); t++)
  {
    t1 = st.opponent[t][m.r1];
    t2 = st.opponent[t][m.r2];
    b1 = st.home[t][m.r1]; 
    b2 = st.home[t][m.r2];
    st.UpdateStateCell(t,m.r1,t2,b2);
    st.UpdateStateCell(t,m.r2,t1,b1);
    // NOTE: it is crucial to store st.home[t][m.r1] in b1, because the first call of UpdateStateCell changes its value
  }

  for (t = 0; t < in.teams.size(); t++)
  {
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(t,m.r1);
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(t,m.r2);
  }
}

void STT_SwapRoundsNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapRounds& m) const
{

  ExecuteMove(st, m);

  //Carico in un vettore tutte le constraints da ricalcolare
  //per STT_SwapRounds carichiamo le constraints relative a tutti i team, agli slots r1 e r2
  vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
  bool already_in_vector;
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    involved_constraints[c_type] = in.slot_constraints[c_type][m.r1];
    for (unsigned int i = 0; i<in.slot_constraints[c_type][m.r2].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.slot_constraints[c_type][m.r2][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
        involved_constraints[c_type].push_back(in.slot_constraints[c_type][m.r2][i]);
    }
  }

  st.UpdateSelectionedCostsConstraints(involved_constraints);
  st.CalculateCostPhased();


  st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());
  if(st.display_OF_isset)
  {
    st.DisplayOFIfNeeded();
  }
}

void STT_SwapRoundsNeighborhoodExplorer::FirstMove(const STT_Solution& st,STT_SwapRounds& m) const
{
  m.r1 = 0;
  m.r2 = 1;
}

bool STT_SwapRoundsNeighborhoodExplorer::NextMove(const STT_Solution& st, STT_SwapRounds& m) const
{
    if (m.r2 < in.slots.size() - 1 && !((!st.in.mix_phase_during_search) && m.r2 == in.slots.size()/2-1)) 
    {
        m.r2++;
        return true;
    }
    else if (m.r1 < in.slots.size() - 2)
    {
        //NOTA: se in.phased ma sto attualmente violando la fase, allora la condizione di fattibilità è la stessa di quando siamo nel caso non phased
        m.r1 = m.r1 + 1 + static_cast<int>((!st.in.mix_phase_during_search) && m.r1 == in.slots.size()/2-2);
        m.r2 = m.r1 + 1;
        return true;
    }
    else
        return false;
}

/***************************************************************************
 * 4 METHODS FOR STT_SwapMatchesNotPhased Neighborhood Explorer:
 ***************************************************************************/

STT_SwapMatchesNotPhasedNeighborhoodExplorer::STT_SwapMatchesNotPhasedNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
  : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchesNotPhased>(in,sm,"STT_SwapMatchesNotPhasedNeighborhoodExplorer")
{
  max_move_length = max(8,static_cast<int>(in.slots.size()/2));
  max_move_lenght_partial_cost_components = 8;
}

void STT_SwapMatchesNotPhasedNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapMatchesNotPhased& m) const
{
  // if(!st.in.mix_phase_during_search)
  //   throw EmptyNeighborhood();

  unsigned int contatore = 0;
  do 
    {
      if(st.in.forbid_hard_worsening_moves && contatore > 100)
      {
        throw EmptyNeighborhood();
      }
      AnyRandomMove(st,m);
      contatore++;
    }
  while (!FeasibleMove(st,m));
}

void STT_SwapMatchesNotPhasedNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapMatchesNotPhased& m) const
{
  m.t1 = Random::Uniform<int>(0,in.teams.size()-1);
  do 
    m.t2 = Random::Uniform<int>(0,in.teams.size()-1);
  while (m.t1 == m.t2);
  do // avoid the rounds in which t1 and t2 play each other
    m.rs[0] = Random::Uniform<int>(0,in.slots.size()-1);
  while (st.opponent[m.t1][m.rs[0]] == m.t2);
  if (m.t1 > m.t2)
    swap(m.t1,m.t2);
}

bool STT_SwapMatchesNotPhasedNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapMatchesNotPhased& m) const
{ // if m.t1 > m.t2 then home and away are swapped

  if (
    m.t1 != m.t2  
    && st.opponent[m.t1][m.rs[0]] != m.t2 
    && ComputeAndCheckInvolvedRounds(st,const_cast<STT_SwapMatchesNotPhased&>(m))
  )
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      //SIMULO LA MOSSA
      FastExecuteMove(st_copy, m);
      //FINE SUMULAZIONE MOSSA
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

void STT_SwapMatchesNotPhasedNeighborhoodExplorer::FastExecuteMove(STT_Solution& st,const STT_SwapMatchesNotPhased& m) const
{
  for(unsigned int r = 0; r < m.rs.size(); r++)
  {
    st.UpdateMatches(m.t1,m.t2,m.rs[r]);
  }
  for(unsigned int r = 0; r < m.rs.size(); r++)
  {
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t1,m.rs[r]);
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t2,m.rs[r]);
  }
}

void STT_SwapMatchesNotPhasedNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapMatchesNotPhased& m) const
{
    vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
    bool already_in_vector;
  

    for(unsigned int r = 0; r < m.rs.size(); r++)
    {
      // adesso, ad ogni passo della repair chain aggiungo ad involved_constraints
      // le constraints che riguardano i team in oggetti nei turni della repair chain
      // NOTA: devo aggiungere anche i loro rivali? No, perché i rivali
      // non cambiano posizione H/A, cambiano solo avverario ma l'avversario è già
      // ricompreso nelle constraints che riguardano m.t1 o m.t2
      
      if(m.rs.size()<=max_move_lenght_partial_cost_components)
      {
        for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
        {
          // [m.t1][m.rs[r]]
          for (unsigned int i = 0; i < in.team_slot_constraints[c_type][m.t1][m.rs[r]].size(); i++)
          {
            already_in_vector = false;
            for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
            {
              if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t1][m.rs[r]][i])
              {
                already_in_vector = true;
                break;
              }
            }
            if(!already_in_vector)
              involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t1][m.rs[r]][i]);
          }

          // [m.t2][m.rs[r]]
          for (unsigned int i = 0; i<in.team_slot_constraints[c_type][m.t2][m.rs[r]].size(); i++)
          {
            already_in_vector = false;
            for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
            {
              if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t2][m.rs[r]][i])
              {
                already_in_vector = true;
                break;
              }
            }
            if(!already_in_vector)
            involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t2][m.rs[r]][i]);
          }
        }
      }
      st.UpdateMatches(m.t1,m.t2,m.rs[r]);
    }

    for(unsigned int r = 0; r < m.rs.size(); r++)
    {
      st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t1,m.rs[r]);
      st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t2,m.rs[r]);
    }


    if(m.rs.size()<=max_move_lenght_partial_cost_components)
    {
      st.UpdateSelectionedCostsConstraints(involved_constraints);
      st.CalculateCostPhased();
    }
    else
    {
      st.CalculateFullCost();
    }


    st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());
    if(st.display_OF_isset)
    {
      st.DisplayOFIfNeeded();
    }
}

bool STT_SwapMatchesNotPhasedNeighborhoodExplorer::ComputeAndCheckInvolvedRounds(const STT_Solution& st,
								     STT_SwapMatchesNotPhased& m) const
{ // computes the set of rounds involved in the move
    unsigned t_stop;
    bool h_stop;
    unsigned t;
    bool h;
    unsigned r;

    r = m.rs[0];
    t_stop = st.opponent[m.t1][r];
    h_stop = st.home[m.t1][r];
    t = st.opponent[m.t2][r];
    h = st.home[m.t2][r];

    m.rs.resize(1);
    // m.swap_homes.resize(1);

    // m.rs[0] = r; // resize non perde i dati

    while (t != t_stop || h != h_stop)
    {
        if(h)
        {
            r = st.match[m.t1][t];
        }   
        else
        {
            r = st.match[t][m.t1];
        }

        t = st.opponent[m.t2][r];
        h = st.home[m.t2][r];

        if (r < m.rs[0])
        {
            return false;
        }

        if(!st.in.mix_phase_during_search && !st.SamePhase(r,m.rs[0])) //if I decided not to allow mixing the phases, I verify if the repair chain breaks the phases
        {
          return false;
        }

        m.rs.push_back(r);

        if (m.rs.size() > max_move_length) 
        {
            return false;
        }
    }
    return true;
}


void STT_SwapMatchesNotPhasedNeighborhoodExplorer::FirstMove(const STT_Solution& st,STT_SwapMatchesNotPhased& m) const
{
    m.t1 = 0;
    m.t2 = 1;
    m.rs[0] = 0;  
    while (!FeasibleMove(st,m))
    {    
        if (!AnyNextMove(st,m))
        throw EmptyNeighborhood();
    }
}


bool STT_SwapMatchesNotPhasedNeighborhoodExplorer::NextMove(const STT_Solution& st,STT_SwapMatchesNotPhased& m) const
{
  do 
    {
      if (!AnyNextMove(st,m))
	return false;
    }
  while (!FeasibleMove(st,m));
  return true;
}
  
bool STT_SwapMatchesNotPhasedNeighborhoodExplorer::AnyNextMove(const STT_Solution& st,STT_SwapMatchesNotPhased& m) const
{
  if (m.rs[0] < in.slots.size()-1)
    m.rs[0]++;
  else if (m.t2 < in.teams.size() - 1) 
    {
      m.t2++;
      m.rs[0] = 0;
    }
  else if (m.t1 < in.teams.size() - 2)  // -1)
    { 
      m.t1++; 
      m.t2 = m.t1 + 1; 
      //        m.t2 = 0;
      m.rs[0] = 0;
    }
  else
    return false;
  return true;
}

/***************************************************************************
 * 5 METHODS FOR STT_SwapMatchesPhased Neighborhood Explorer:
 ***************************************************************************/

STT_SwapMatchesPhasedNeighborhoodExplorer::STT_SwapMatchesPhasedNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
  : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchesPhased>(in,sm,"STT_SwapMatchesPhasedNeighborhoodExplorer")
{
  max_move_length = max(8,static_cast<int>(in.slots.size()/2));
  max_move_lenght_partial_cost_components = 0; //NEEDS TO BE ZERO UNTIL BUG ON PARTIAL COSTS ON STT_SwapMatchesPhased IS SOLVED
}

void STT_SwapMatchesPhasedNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapMatchesPhased& m) const
{
  unsigned int contatore = 0;    
  do 
    {
      if(st.in.forbid_hard_worsening_moves && contatore > 100)
      {
        throw EmptyNeighborhood();
      }
      AnyRandomMove(st,m);
      contatore ++;
    }
  while (!FeasibleMove(st,m));
}

void STT_SwapMatchesPhasedNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapMatchesPhased& m) const
{
  m.t1 = Random::Uniform<int>(0,in.teams.size()-1);
  do 
    m.t2 = Random::Uniform<int>(0,in.teams.size()-1);
  while (m.t1 == m.t2);
  do // avoid the rounds in which t1 and t2 play each other
    m.rs[0] = Random::Uniform<int>(0,in.slots.size()-1);
  while (st.opponent[m.t1][m.rs[0]] == m.t2 || st.is_return_match[m.t1][m.rs[0]] != st.is_return_match[m.t2][m.rs[0]]);
  if (m.t1 > m.t2)
    swap(m.t1,m.t2);
}

bool STT_SwapMatchesPhasedNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapMatchesPhased& m) const
{ 

  if (
    m.t1 != m.t2  
    && st.opponent[m.t1][m.rs[0]] != m.t2 
    && st.is_return_match[m.t1][m.rs[0]] == st.is_return_match[m.t2][m.rs[0]] //entrambi devono stare giocando un'andata oppure un ritorno
    && ComputeAndCheckInvolvedRounds(st,const_cast<STT_SwapMatchesPhased&>(m))
  )
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      //SIMULO LA MOSSA
      FastExecuteMove(st_copy, m);
      //FINE SUMULAZIONE MOSSA
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

void STT_SwapMatchesPhasedNeighborhoodExplorer::FastExecuteMove(STT_Solution& st,const STT_SwapMatchesPhased& m) const
{
  for(unsigned int r = 0; r < m.rs.size(); r++)
  {
    st.UpdateMatches(m.t1,m.t2, m.rs[r], m.swap_homes_t2[r], m.swap_homes_t1[r]);
  }
  for(unsigned int r = 0; r < m.rs.size(); r++)
  {
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t1,m.rs[r]);
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t2,m.rs[r]);
  }
}

void STT_SwapMatchesPhasedNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapMatchesPhased& m) const
{
    vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
    bool already_in_vector;

    for(unsigned int r = 0; r < m.rs.size(); r++)
    {
      // adesso, ad ogni passo della repair chain aggiungo ad involved_constraints
      // le constraints che riguardano i team in oggetti nei turni della repair chain
      // NOTA: devo aggiungere anche i loro rivali? No, perché i rivali
      // non cambiano posizione H/A, cambiano solo avverario ma l'avversario è già
      // ricompreso nelle constraints che riguardano m.t1 o m.t2
      
      if(m.rs.size()<=max_move_lenght_partial_cost_components)
      {
        for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
        {
          // [m.t1][m.rs[r]]
          for (unsigned int i = 0; i < in.team_slot_constraints[c_type][m.t1][m.rs[r]].size(); i++)
          {
            already_in_vector = false;
            for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
            {
              if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t1][m.rs[r]][i])
              {
                already_in_vector = true;
                break;
              }
            }
            if(!already_in_vector)
              involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t1][m.rs[r]][i]);
          }

          // [m.t2][m.rs[r]]
          for (unsigned int i = 0; i<in.team_slot_constraints[c_type][m.t2][m.rs[r]].size(); i++)
          {
            already_in_vector = false;
            for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
            {
              if(involved_constraints[c_type][j] == in.team_slot_constraints[c_type][m.t2][m.rs[r]][i])
              {
                already_in_vector = true;
                break;
              }
            }
            if(!already_in_vector)
            involved_constraints[c_type].push_back(in.team_slot_constraints[c_type][m.t2][m.rs[r]][i]);
          }
        }
      }

      st.UpdateMatches(m.t1,m.t2, m.rs[r], m.swap_homes_t2[r], m.swap_homes_t1[r]);
    }
    for(unsigned int r = 0; r < m.rs.size(); r++)
    {
      st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t1,m.rs[r]);
      st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.t2,m.rs[r]);
    }

    if(m.rs.size()<=max_move_lenght_partial_cost_components)
    {
      st.UpdateSelectionedCostsConstraints(involved_constraints);
      st.CalculateCostPhased();
    }
    else
    {
      st.CalculateFullCost();
    }

    st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());
    if(st.display_OF_isset)
    {
      st.DisplayOFIfNeeded();
    }
}

bool STT_SwapMatchesPhasedNeighborhoodExplorer::ComputeAndCheckInvolvedRounds(const STT_Solution& st,
								     STT_SwapMatchesPhased& m) const
{ // computes the set of rounds involved in the move

    // qui, a differenza del caso notphased, lavoriamo solo in uno dei due gironi (andata o ritorno)
    // e non abbiamo, pertanto, bisogno di h e h_stop
    unsigned t_stop;
    unsigned t;
    unsigned r = m.rs[0];
    bool elected_phase = st.is_return_match[m.t1][m.rs[0]];
    
    t_stop = st.opponent[m.t1][r];
    t = st.opponent[m.t2][r];
    m.rs.resize(1);
    m.swap_homes_t1[0] = false;
    m.swap_homes_t2[0] = false;
    m.swap_homes_t1.resize(1);
    m.swap_homes_t2.resize(1);


    //devo calcolarlo anche al turno 0
    if(st.home[m.t1][m.rs[0]]) //se t1 gioca in casa, allora la nuova partita sarà t2 - opponent t1
    {
      if(st.IsReturnMatch(m.t2,st.opponent[m.t1][m.rs[0]]) != st.IsReturnMatch(m.t1,st.opponent[m.t1][m.rs[0]]))
      {
        m.swap_homes_t2[m.swap_homes_t2.size()-1] = true;
      }
    }
    else  //se t1 gioca in trasferta, la nuova partita sarà 
    {
      if(st.IsReturnMatch(st.opponent[m.t1][m.rs[0]], m.t2) != st.IsReturnMatch(st.opponent[m.t1][m.rs[0]], m.t1))
      {
        m.swap_homes_t2[m.swap_homes_t2.size()-1] = true;
      }
    }

    if(st.home[m.t2][m.rs[0]]) //se t2 gioca in casa, allora la nuova partita sarà t1 - opponent t2
    {
      if(st.IsReturnMatch(m.t1,st.opponent[m.t2][m.rs[0]]) != st.IsReturnMatch(m.t2,st.opponent[m.t2][m.rs[0]]))
      {
        m.swap_homes_t1[m.swap_homes_t1.size()-1] = true;
      }
    }
    else  //se t1 gioca in trasferta, la nuova partita sarà 
    {
      if(st.IsReturnMatch(st.opponent[m.t2][m.rs[0]], m.t1) != st.IsReturnMatch(st.opponent[m.t2][m.rs[0]], m.t2))
      {
        m.swap_homes_t1[m.swap_homes_t1.size()-1] = true;
      }
    }

    
    if((st.is_return_match[m.t2][m.rs[0]] != elected_phase))
    {
      return false;
    }

    while (t != t_stop)// || h != h_stop)
    {
        r = st.match[m.t1][t];
        if(st.is_return_match[m.t1][r] != elected_phase)
          r = st.match[t][m.t1];

        if (r < m.rs[0])
        {
            return false;
        }

        //verifico adesso se in quel turno r, la partita giocata da t2 e da t (avversario di t2 nel vecchio m.rs[r-1]) siano concordi entrambe nell'essere un'andata o un ritorno.
        //se non lo è, dichiaro la mossa non fattibile
        if((st.is_return_match[m.t2][r] != elected_phase))
        {
          return false;
        }
        
        t = st.opponent[m.t2][r];
        m.rs.push_back(r);
        if (m.rs.size() > max_move_length) 
        {
            return false;
        }

        //se ho deciso di aggiungere un nuovo step alla repair chain, calcolo i valori di swap_homes_t1 e swap_homes_t2
        m.swap_homes_t2.push_back(false);
        if(st.home[m.t1][r]) //se t1 gioca in casa, allora la nuova partita sarà t2 - opponent t1
        {
          if(st.IsReturnMatch(m.t2,st.opponent[m.t1][r]) != st.IsReturnMatch(m.t1,st.opponent[m.t1][r]))
          {
            m.swap_homes_t2[m.swap_homes_t2.size()-1] = true;
          }
        }
        else  //se t1 gioca in trasferta, la nuova partita sarà 
        {
          if(st.IsReturnMatch(st.opponent[m.t1][r], m.t2) != st.IsReturnMatch(st.opponent[m.t1][r], m.t1))
          {
            m.swap_homes_t2[m.swap_homes_t2.size()-1] = true;
          }
        }

        m.swap_homes_t1.push_back(false);

        if(st.home[m.t2][r]) //se t2 gioca in casa, allora la nuova partita sarà t1 - opponent t2
        {
          if(st.IsReturnMatch(m.t1,st.opponent[m.t2][r]) != st.IsReturnMatch(m.t2,st.opponent[m.t2][r]))
          {
            m.swap_homes_t1[m.swap_homes_t1.size()-1] = true;
          }
        }
        else  //se t1 gioca in trasferta, la nuova partita sarà 
        {
          if(st.IsReturnMatch(st.opponent[m.t2][r], m.t1) != st.IsReturnMatch(st.opponent[m.t2][r], m.t2))
          {
            m.swap_homes_t1[m.swap_homes_t1.size()-1] = true;
          }
        }

    }
    //devo fare quest'ultima verifica per l'ultimo r
    if(st.is_return_match[t][r] != elected_phase)
    {
      return false;
    }
    return true;
}


void STT_SwapMatchesPhasedNeighborhoodExplorer::FirstMove(const STT_Solution& st,STT_SwapMatchesPhased& m) const
{
    m.t1 = 0;
    m.t2 = 1;
    m.rs[0] = 0;  
    while (!FeasibleMove(st,m))
    {    
        if (!AnyNextMove(st,m))
        throw EmptyNeighborhood();
    }
}


bool STT_SwapMatchesPhasedNeighborhoodExplorer::NextMove(const STT_Solution& st,STT_SwapMatchesPhased& m) const
{
  do 
    {
      if (!AnyNextMove(st,m))
	return false;
    }
  while (!FeasibleMove(st,m));
  return true;
}
  
bool STT_SwapMatchesPhasedNeighborhoodExplorer::AnyNextMove(const STT_Solution& st,STT_SwapMatchesPhased& m) const
{
  if (m.rs[0] < in.slots.size()-1)
    m.rs[0]++;
  else if (m.t2 < in.teams.size() - 1) 
    {
      m.t2++;
      m.rs[0] = 0;
    }
  else if (m.t1 < in.teams.size() - 2)  // -1)
    { 
      m.t1++; 
      m.t2 = m.t1 + 1; 
      //        m.t2 = 0;
      m.rs[0] = 0;
    }
  else
    return false;
  return true;
}

/***************************************************************************
 * 6 METHODS FOR STT_SwapMatchRound Neighborhood Explorer:
 ***************************************************************************/

STT_SwapMatchRoundNeighborhoodExplorer::STT_SwapMatchRoundNeighborhoodExplorer(const STT_Input& in, SolutionManager<STT_Input, STT_Solution>& sm)
  : NeighborhoodExplorer<STT_Input,STT_Solution,STT_SwapMatchRound>(in,sm,"STT_SwapMatchRoundNeighborhoodExplorer")
{
  //max_move_length = UINT_MAX;
  //max_move_length = 40;
  max_move_length = max(4,static_cast<int>((in.teams.size()/2)-1)); //non hanno senso repair chain + lunghe perché è una swaprounds altrimenti
}

void STT_SwapMatchRoundNeighborhoodExplorer::RandomMove(const STT_Solution& st, STT_SwapMatchRound& m) const
{
  unsigned int contatore = 0;
  do 
  {
    if(!st.in.mix_phase_during_search)
      if (contatore > 30)
        throw EmptyNeighborhood();
    
    if(st.in.forbid_hard_worsening_moves && contatore > 100)
    {
      throw EmptyNeighborhood();
    }
    AnyRandomMove(st,m);
    contatore++;
  }
  while (!FeasibleMove(st,m));
}

void STT_SwapMatchRoundNeighborhoodExplorer::AnyRandomMove(const STT_Solution& st, STT_SwapMatchRound& m) const
{
    m.ts[0] = Random::Uniform<int>(0,in.teams.size()-1);
    m.r1 = Random::Uniform<int>(0,in.slots.size()-1);
    
    do
    {
        // //NOTA: se in.phased ma sto attualmente violando la fase, allora la condizione di fattibilità è la stessa di quando siamo nel caso non phased
        if(!st.in.mix_phase_during_search)
        {
            if(m.r1<in.slots.size()/2)
            {
                m.r2 = Random::Uniform<int>(0,in.slots.size()/2-1);
            }
            else
            {
                m.r2 = Random::Uniform<int>(in.slots.size()/2,in.slots.size()-1);    
            }
        }
        else
        {
            m.r2 = Random::Uniform<int>(0,in.slots.size()-1);
        }
    } 
    while (m.r1 == m.r2); 
    if (m.r1 > m.r2)
        swap(m.r1,m.r2);
}

bool STT_SwapMatchRoundNeighborhoodExplorer::FeasibleMove(const STT_Solution& st, const STT_SwapMatchRound& m) const
{
  if(m.r1 < m.r2 && ComputeAndCheckInvolvedTeams(st,m,max_move_length))
  {
    if(st.in.forbid_hard_worsening_moves) //se siamo nel secondo stage
    {
      STT_Solution st_copy(st.in);
      st_copy = st;
      //SIMULO LA MOSSA
      ExecuteMove(st_copy, m);
      //FINE SUMULAZIONE MOSSA
      for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
      {
        for(unsigned int k = 0; k < st_copy.in.constraints_hard_indexes[c_type].size(); k++)
        {
          if(st_copy.CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), st_copy.in.constraints_hard_indexes[c_type][k]) > 0)
          {
            return false;
          }
        }
      }
    }
    return true;
  }
  return false;
}

void STT_SwapMatchRoundNeighborhoodExplorer::ExecuteMove(STT_Solution& st,const STT_SwapMatchRound& m) const
{
  unsigned t1, t2;
  bool b1, b2;
  for (unsigned int t = 0; t < m.ts.size(); t++)
  {
    t1 = st.opponent[m.ts[t]][m.r1];
    t2 = st.opponent[m.ts[t]][m.r2];
    b1 = st.home[m.ts[t]][m.r1]; 
    b2 = st.home[m.ts[t]][m.r2];
    st.UpdateStateCell(m.ts[t],m.r1,t2,b2);
    st.UpdateStateCell(m.ts[t],m.r2,t1,b1);
  }

  for (unsigned int t = 0; t < m.ts.size(); t++)
  {
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.ts[t],m.r1);
    st.PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(m.ts[t],m.r2);
  }
}

void STT_SwapMatchRoundNeighborhoodExplorer::MakeMove(STT_Solution& st,const STT_SwapMatchRound& m) const
{
  vector<vector<unsigned int>> involved_constraints(N_CONSTRAINTS, vector<unsigned>(0,0));
  bool already_in_vector;

  // popolo il vettore involved_constraints, inseriamo tutti le constraints dei due round
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    // [m.r1]
    involved_constraints[c_type] = in.slot_constraints[c_type][m.r1];
    // [m.r2]
    for (unsigned int i = 0; i<in.slot_constraints[c_type][m.r2].size(); i++)
    {
      already_in_vector = false;
      for(unsigned int j = 0; j<involved_constraints[c_type].size(); j++)
      {
        if(involved_constraints[c_type][j] == in.slot_constraints[c_type][m.r2][i])
        {
          already_in_vector = true;
          break;
        }
      }
      if(!already_in_vector)
      involved_constraints[c_type].push_back(in.slot_constraints[c_type][m.r2][i]);
    }
  }

  ExecuteMove(st, m);

  st.UpdateSelectionedCostsConstraints(involved_constraints);
  st.CalculateCostPhased();

  st.UpdateMoveCounterAndBestSolution(st.ReturnTotalCost());

  if(st.display_OF_isset)
  {
    st.DisplayOFIfNeeded();
  }
}

bool STT_SwapMatchRoundNeighborhoodExplorer::ComputeAndCheckInvolvedTeams(const STT_Solution& st,
								      const STT_SwapMatchRound& m, unsigned max) const
{ // compute the set of teams involved in the move
  unsigned t1 = m.ts[0];
  m.ts.resize(1);
  m.ts[0] = t1;
  do 
    {
      t1 = st.opponent[t1][m.r2];
      if (t1 < m.ts[0]) return false;
      m.ts.push_back(t1);
      t1 = st.opponent[t1][m.r1];
      if (t1 < m.ts[0]) return false;
      if (t1 == m.ts[0]) break;
      m.ts.push_back(t1);
      if (m.ts.size() > max) 
	return false;
    }
  while (t1 != m.ts[0]);
  return true;
}

void STT_SwapMatchRoundNeighborhoodExplorer::FirstMove(const STT_Solution& st,STT_SwapMatchRound& m) const
{
   m.ts[0] = 0; 
   m.r1 = 0; 
   m.r2 = 1;  
   while (!FeasibleMove(st,m))
     if (!AnyNextMove(st,m))
       throw EmptyNeighborhood();
}

bool STT_SwapMatchRoundNeighborhoodExplorer::NextMove(const STT_Solution& st, STT_SwapMatchRound& m) const
{
  do 
    if (!AnyNextMove(st,m))
      return false;
  while (!FeasibleMove(st,m));
  return true;
}

bool STT_SwapMatchRoundNeighborhoodExplorer::AnyNextMove(const STT_Solution& st, STT_SwapMatchRound& m) const
{
  //NOTA: se in.phased ma sto attualmente violando la fase, allora la condizione di fattibilità è la stessa di quando siamo nel caso non phased

  if (m.r2 < in.slots.size()-1 && !((!st.in.mix_phase_during_search) && m.r2 == in.slots.size()/2-1))
    {
      m.r2++;
      return true;
    }
  else if (m.r1 < in.slots.size() - 2) 
    {
      m.r1 = m.r1 + 1 + static_cast<int>((!st.in.mix_phase_during_search) && m.r1 == in.slots.size()/2-2);
      m.r2 = m.r1+1;
      return true;
    }
  else if (m.ts[0] < in.teams.size() - 1)
    { 
      m.ts[0]++; 
      m.r1 = 0;
      m.r2 = 1;
      return true;
    }
  else
    return false;
}

//non so se serve, ma lo metto comunque qui
inline bool intersect(const std::vector<unsigned>& l1, const std::vector<unsigned>& l2) 
{
  for (unsigned i = 0; i < l1.size(); i++) 
    for (unsigned j = 0; j < l2.size(); j++)
      if (l1[i] == l2[j])
	return true;
  return false;
}
