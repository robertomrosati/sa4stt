#include "stt_basics.hh"
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <random>

ostream& operator<<(ostream& os, const STT_Solution& st)
{
    size_t n = st.in.teams.size(), r = st.in.slots.size();
    unsigned int w = (n > 10) ? 2 : 1; // for nice visualization
     for (size_t i = 0; i < n; i++)
     {
       for (size_t j = 0; j < r; j++)
       {
         if (st.home[i][j])
           os << '+';
         else
           os << '-';
         os << setw(w) << st.opponent[i][j] << " ";
       }
       os << (st.print_solution_on_one_line ? " " : " \n");
     }
     //st.PrintIsReturnMatrix();
     return os;
}

istream& operator>>(istream& is, STT_Solution& st)
{
  if(is.peek() == '<')
  {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load(is);
    unsigned int i, j, s;
    //Alcuni controlli d'integrità
    if(!result)
    {
      ostringstream msg;
      msg << "initial XML solution parsed with errors" << endl;
      msg << "Error description: " << result.description() << endl;
      msg << "Error offset: " << result.offset;
      throw logic_error(msg.str()); 
    }

    if(doc.select_node("/Solution/MetaData/InstanceName").node().text().get() != st.in.name)
    {
      ostringstream msg;
      msg << "Initial XML solution named \"" << doc.select_node("/Solution/MetaData/InstanceName").node().text().get();
      msg << "\" whilst current instance is named \"" << st.in.name << "\"" << endl;
      throw logic_error(msg.str()); 
    }

    //carico i match
    for(auto n : doc.select_nodes("/Solution/Games/ScheduledMatch"))
    {
      //I read data from xml
      i = n.node().attribute("home").as_uint();
      j = n.node().attribute("away").as_uint();
      s = n.node().attribute("slot").as_uint();

      //I write data into STT_Solution (and redundants)
      st.opponent[i][s] = j;
      st.opponent[j][s] = i;
      st.home[i][s] = true;
      st.home[j][s] = false;
      st.match[i][j] = s;
    }
  }
  else
  {
    size_t n = st.in.teams.size(), r = st.in.slots.size();
    char ch;
    
    for (size_t i = 0; i < n; i++)
        for (size_t s = 0; s < r; s++)
        {
            is >> ch >> st.opponent[i][s];
            if((st.home[i][s] = (ch == '+')))
            {
              st.match[i][st.opponent[i][s]] = s;
            }
        }
  }
  st.PopulateIsReturnMatrix();
  //calcolo i costi materializzati
  st.last_best_solution = st.CalculateFullCost();
  return is;
}

bool operator==(const STT_Solution& sol1, const STT_Solution& sol2)
{
  return sol1.opponent == sol2.opponent && sol1.home == sol2.home && sol1.match == sol2.match; //non verifico is_return_match, perché (salvo incosistenze tra i dati ridondanti) la verifica di questi tre componenti è sufficiente
}

void STT_Solution::PopulateIsReturnMatrix()
{
  //adesso popolo il vettore delle andate e dei ritorni
  for(unsigned s = 0; s<in.slots.size(); s++)
  {
    for (unsigned t = 0; t<in.teams.size(); t++)
    {
      PopulateCellInsideIsReturnMatrix(t,s);
    }
  }
}

unsigned STT_Solution::SpecularMatch(unsigned t1, unsigned t2) const
{
  return match[t2][t1];
}


void STT_Solution::PopulateCellInsideIsReturnMatrix(unsigned t, unsigned s)
{
  if(home[t][s]) //se t gioca in casa
  {
    if(match[t][opponent[t][s]] > match[opponent[t][s]][t]) 
    {
      is_return_match[t][s] = true;    
    }
    else
    {
      is_return_match[t][s] = false;
    }
  }
  else //altrimenti, se t gioca in trasferta
  {
    if(match[opponent[t][s]][t] > match[t][opponent[t][s]])
    {
      is_return_match[t][s] = true;
    }
    else
    {
      is_return_match[t][s] = false;
    }
  }
}

void STT_Solution::PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(unsigned t, unsigned s)
{
  //in addition to fixing team t in slot s, fixes also opponent of t in slot s, and the related go/return
  //per ogni turno coinvolto nella 
  PopulateCellInsideIsReturnMatrix(t,s);
  PopulateCellInsideIsReturnMatrix(opponent[t][s],s);

  //devo cercare anche i ritorni e correggerli se necessario
  if(home[t][s]) //se t gioca in casa, l'altra partità sarà oppt-t
  {
    PopulateCellInsideIsReturnMatrix(t,match[opponent[t][s]][t]);
    PopulateCellInsideIsReturnMatrix(opponent[t][s],match[opponent[t][s]][t]);
  }
  else //se t gioca in trasferta, l'altra partità sarà t-oppt
  {
    PopulateCellInsideIsReturnMatrix(t,match[t][opponent[t][s]]);
    PopulateCellInsideIsReturnMatrix(opponent[t][s],match[t][opponent[t][s]]);
  }
}

int STT_Solution::PrintIsReturnMatrix(bool suppress_print) const
{
  unsigned andate, ritorni;
  andate = 0;
  ritorni = 0;
  if(!suppress_print)
  {
    cout << endl;
  }
  for(unsigned t = 0; t < in.teams.size(); t++)
  {
    for(unsigned s = 0; s < in.slots.size(); s++)
    {
      if(!suppress_print)
      {
        cout << (is_return_match[t][s] ? "  1 " : "  0 ");
      }
      is_return_match[t][s] ? ritorni++ : andate++;
    }
    if(!suppress_print)
    {
      cout << endl;
    }
  }
  if(!suppress_print)
  {
    cout << "andate = " << andate << ", ritorni = " << ritorni << endl;
  }
  return abs(static_cast<int>(andate)-static_cast<int>(ritorni));
}

bool STT_Solution::InternalCheckConsistency() const
{
    unsigned count_consistency = 0;
    bool andata_trovata;
    unsigned turno_andata;
    unsigned numero_incontri;
    unsigned i, j, s;

    //controllo che due team si incontrino esattamente due volte
    /*per ogni team in i*/
    for(i=0; i<in.teams.size()-1; i++)
    {
        /*per ogni team in j, escluso i*/
        for(j=i+1; j<in.teams.size(); j++)
        {
            numero_incontri = 0;
            /*per ogni slot*/
            for(s=0; s<in.slots.size(); s++)
            {
                if(opponent[i][s] == j)
                {
                    numero_incontri++;              
                }
            }

            if(numero_incontri!=2)
            {
                count_consistency++;
                cout << "unconsistency found: teams " << i << " and " << j <<
                    " play " << numero_incontri << " times against each other " << endl;
            }
        }
    }

    if(count_consistency>0)
    {
        cout << "ATTENTION: we have more than 0 unconsistencies of the above type, " <<
                "following information might not be correct" << endl;
    }

    /*per ogni team in i*/
    for(i=0; i<in.teams.size()-1; i++)
    {
        /*per ogni team in j, escluso i*/
        for(j=i+1; j<in.teams.size(); j++)
        {
            /*per ogni slot*/
            andata_trovata = false;
            for(s=0; s<in.slots.size(); s++)
            {
                if(opponent[i][s] == j)
                {
                    if(!andata_trovata)
                    {
                        andata_trovata = true;
                        turno_andata = s;
                    }
                    else
                    {
                        if(home[i][turno_andata] == home[i][s])
                        {
                            count_consistency++;
                            cout << "unconsistency found: teams " << i << " and " << j <<
                                " play in the same home-away order at rounds " << 
                                turno_andata << " and " << s << ")" << endl;
                        }
                        break;
                    }
                }
            }
        }
    }

    //check phase consistency
    if(in.phased && !in.mix_phase_during_search)
    {
       /*per ogni team in i*/
        for(i=0; i<in.teams.size()-1; i++)
        {
            /*per ogni team in j, escluso i*/
            for(j=i+1; j<in.teams.size(); j++)
            {
                if(
                      (match[i][j] < in.slots.size()/2 && match[j][i] < in.slots.size()/2)
                                                 ||
                      (match[i][j] >= in.slots.size()/2 && match[j][i] >= in.slots.size()/2)
                )
                {
                    count_consistency++;
                    cout << "unconsistency found: teams " << i << " and " << j <<
                                " play twice in the same phase (rounds: " << 
                                match[i][j] << " and " << match[j][i] << ")" << endl;
                }
            }
        }
    }

    //check in.round consistency (check that a team doesn't play more them once in the same round)
    for(s=0; s<in.slots.size(); s++)
    {
        for(i=0; i<in.teams.size(); i++)
        {
            numero_incontri = 0;
            for(j=0; j<in.teams.size(); j++)
            {
                if(i!=j)
                {
                    if(match[i][j] == s || match[j][i] == s)
                    {
                        numero_incontri++;
                    }
                }
            }
            if(numero_incontri != 1) //each team is expected to play just once in each slot
            {
                cout << "unconsistency found: team " << i << " plays " <<
                            numero_incontri << " times at round: " << 
                            s << endl;
            }
            count_consistency+=abs(static_cast<int>(numero_incontri)-1);
        }
    }

    //controllo adesso la consistenza del vettore di andate e ritorni
    for(s=0; s<in.slots.size(); s++)
    {
      for(i=0; i<in.teams.size(); i++)
      {
        if(home[i][s])
        {
          if(is_return_match[i][s] == is_return_match[i][match[opponent[i][s]][i]])
          {
            cout << "unconsistency found: team " << i << " plays two times in the same status go or return against team " 
                  << opponent[i][s] << " at rounds " << s << " and " << match[opponent[i][s]][i] << ", they have both values is_return_match = "
                  << is_return_match[i][s] << endl;
            count_consistency+=1;
          }
        }
        else
        {
          if(is_return_match[i][s] == is_return_match[i][match[i][opponent[i][s]]])
          {
            cout << "unconsistency found: team " << i << " plays two times in the same status go or return against team " 
                  << opponent[i][s] << " at rounds " << s << " and " << match[i][opponent[i][s]] << ", they have both values is_return_match = "
                  << is_return_match[i][s] << endl;
            count_consistency+=1;
          }
        }
      }
    }

    //check cost consistency (check that costs are aligned)
    //copio i costi
    vector<int> cost_components_copied = cost_components;
    //ricalcolo i costi con le funzioni canoniche
    STT_Solution solution_copy = *this;
    solution_copy.CalculateFullCost();
    for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
      if(cost_components[c_type]!=solution_copy.cost_components[c_type])
      {
        
        cout << "unconsistency found, cost_component " << static_cast<Constraints::ConstraintType>(c_type) << " not aligned, " <<
                      "saved cost = " << cost_components[c_type] <<
                      "actual cost = " <<solution_copy.cost_components[c_type] << endl;
        count_consistency++;
      }
    
    for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
      if(cost_components_hard[c_type]!=solution_copy.cost_components_hard[c_type])
      {
        
        cout << "unconsistency found, cost_components_hard " << static_cast<Constraints::ConstraintType>(c_type) << " not aligned, " <<
                      "saved cost = " << cost_components_hard[c_type] <<
                      "actual cost = " <<solution_copy.cost_components_hard[c_type] << endl;
        count_consistency++;
      }
    
    //se il numero di andate e ritorni non coincide, stampo la matrice
    if(PrintIsReturnMatrix(true) != 0)
        PrintIsReturnMatrix();
    
    cout << "Check consistency ended, number of unconcistencies found: " << count_consistency << endl;
    return (count_consistency == 0);
}

string STT_Solution::PrintCostComponentsCompacted() const
{
  string s;
  s = "CA1, CA2, CA3, CA4, GA1, BR1, BR2, FA2, SE1\n";
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    s+= to_string(cost_components[c_type]) + ", ";
  }
  s+= to_string(cost_components[SE1]) + "\n";
  return s;
}

void STT_Solution::CanonicalPattern(bool permute, bool mix_initial_phase)
{
  // generate the canonical pattern
  size_t n = in.teams.size(), r = in.slots.size();
  size_t matches = n / 2;
  size_t second_half_round = r / 2;
  
  vector<vector<pair<unsigned int, unsigned int>>> pattern(r, vector<pair<unsigned int, unsigned int>>(matches));
  
  
  for (size_t i = 0; i < r / 2; i++)
    if (i % 2 == 0)
      {
        pattern[i][0].first = i;
        pattern[i][0].second = n - 1;
        pattern[i + second_half_round][0].first = n - 1;
        pattern[i + second_half_round][0].second = i;
      }
    else
      {
        pattern[i][0].first = n - 1;
        pattern[i][0].second = i;
        pattern[i + second_half_round][0].first = i;
        pattern[i + second_half_round][0].second = n - 1;
      }
  
  for (size_t i = 0; i < r / 2; i++)
    for (size_t k = 1; k < matches; k++)
      {
        unsigned int a, b;
        a = (i + k) % (n - 1);
        b = (i + n - 1 - k) % (n - 1); // add teams - 1 to prevent negative numbers
        if (k % 2 == 0)
          {
            pattern[i][k].first = a;
            pattern[i][k].second = b;
            pattern[i + second_half_round][k].first = b;
            pattern[i + second_half_round][k].second = a;
          }
        else
          {
            pattern[i][k].first = b;
            pattern[i][k].second = a;
            pattern[i + second_half_round][k].first = a;
            pattern[i + second_half_round][k].second = b;
          }
      }
  std::vector<size_t> tp(n), rp(r);
  iota(begin(tp), end(tp), 0);
  iota(begin(rp), end(rp), 0);
  // permute the teams in the pattern
  if (permute)
    {
      Shuffle(begin(tp), end(tp)); 

      if (mix_initial_phase)
	      Shuffle(begin(rp), end(rp)); 
      else
      {
        Shuffle(begin(rp), begin(rp) + r / 2); 
        Shuffle(begin(rp) + r / 2 + 1, end(rp)); 
      }

    }
  
  for (size_t k = 0; k < r; k++)
    for (size_t i = 0; i < matches; i++)
      {
        unsigned int a, b;
        a = pattern[k][i].first;
        b = pattern[k][i].second;
        opponent[tp[a]][rp[k]] = tp[b];
        opponent[tp[b]][rp[k]] = tp[a];
        match[tp[a]][tp[b]] = rp[k];
        home[tp[a]][rp[k]] = true;
        home[tp[b]][rp[k]] = false;
      }
  
  PopulateIsReturnMatrix();  
}

void STT_Solution::Shuffle(vector<size_t>::iterator start, vector<size_t>::iterator stop)
{ // We do not use the shuffle from stardard library in order to use the Random of easylocal,
  // so that each run can be reproduced giving the seed
  for (vector<size_t>::iterator it = start; it < stop; it++)
    {
      size_t v = Random::Uniform<size_t>(0, stop-it-1);
      swap(*it,*(it + v));
    }
}

void STT_Solution::UpdateStateCell(unsigned t1, unsigned r, unsigned t2, bool home_game)
{   // update the state data of t1 for assigning the match with t2 at round r
    // (it corresponds to the old "UpdateStateCell in TTP")
    
    // update match
    if (home_game) // to prevent to do it twice, we do it only when it is a home game
    {
      //UpdateLocalCostsDeleteMatch(t1, t2, match[t1][t2]); //modifico i costi legati al match che elimino
      match[t1][t2] = r;
      //UpdateLocalCostsInsertMatch(t1, t2, match[t1][t2]); //modifico i costi legati al match che aggiungo 
    }    
    
    // update redundant data
    home[t1][r] = home_game;
    opponent[t1][r] = t2;
}

void STT_Solution::UpdateMatches(unsigned t1, unsigned t2, unsigned r, bool rev1, bool rev2) //rev1, rev2 false by default
{ // update the state swapping the opponent of t1 and t2
  // if rev1 = true invert the home position of t1, if rev2 = true invert the home position of t2
  
  unsigned ot1, ot2;
  bool b1, b2;
  ot1 = opponent[t1][r];
  ot2 = opponent[t2][r];
  b1 = home[t1][r];
  b2 = home[t2][r];

  if (ot1 == t2) // they play each other
  {
    UpdateStateCell(t1,r,t2,b2);
    UpdateStateCell(t2,r,t1,b1);
  }
  else
  {
    if (t1 > t2)
    {
      b1 = !b1;
      b2 = !b2;
    }
    if (rev1)
      b1 = !b1;
    if (rev2)
      b2 = !b2;

    UpdateStateCell(t1,r,ot2,b2);
    UpdateStateCell(ot2,r,t1,!b2);
    UpdateStateCell(t2,r,ot1,b1);
    UpdateStateCell(ot1,r,t2,!b1);    
  }

  //aggiorno anche is_return_match, involved teams are: t1, t2, opponent of t1, opponent of t2
  //NOTA: proviamo intanto a farlo qui, ma potrebbe essere che creiamo facilmente delle incosistenze
  //dentro la matrice 

  //NOTA: quanto scritto alla nota precedente per adesso non si applica, l'ho fatto nelle relative MakeMove.
}

void STT_Solution::InternalMakeSwapHomes(unsigned t1, unsigned t2)
{
      unsigned r1, r2;
      r1 = match[t1][t2];
      r2 = match[t2][t1];
      bool at_home = home[t1][r1];
      UpdateStateCell(t1, r1, t2, !at_home);
      UpdateStateCell(t1, r2, t2, at_home);
      UpdateStateCell(t2, r1, t1, at_home);
      UpdateStateCell(t2, r2, t1, !at_home);
      //PopulateCellInsideIsReturnMatrixAndFixRelatedMatches(t1,r1); //this will automatically fix all 2 matches (descrpition of the method:   //in addition to fixing team t in slot s, fixes also opponent of t in slot s, and the related go/return)
}

vector<vector<vector<int>>> STT_Solution::GetCplexWarmSolution()
{
  vector<vector<vector<int>>> x(in.teams.size(),  vector<vector<int>>(in.teams.size(), vector<int>(in.slots.size(), 0)));
  unsigned i, j, s;
  for (i = 0; i < in.teams.size(); i++)
    for (j = 0; j < in.teams.size(); j++)
      for (s = 0; s < in.slots.size(); s++)
        if(match[i][j] == s)
          x[i][j][s]= 1;
  return x;
}

int STT_Solution::ReturnTotalCost()
{
  return total_cost_components + cost_phased;
}

int STT_Solution::CalculateFullCost()
{
  total_cost_components = 0;
  total_cost_components_hard = 0;
  for (unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    total_cost_components += CalculateCostComponent(c_type);
    total_cost_components_hard += CalculateCostComponentHard(c_type);
  }
  CalculateCostPhased();
  return total_cost_components + cost_phased;
}

int STT_Solution::CalculateCostComponent(unsigned int c_type)
{
  int cost = 0;
  for(unsigned int c = 0; c < in.ConstraintsVectorSize(c_type); c++)
  {
      cost_single_constraints[c_type][c] = CalculateCostSingleConstraint(c_type, c);
      cost += cost_single_constraints[c_type][c];
  }
  cost_components[c_type] = cost;
  return cost_components[c_type];
}

int STT_Solution::CalculateCostComponentHard(unsigned int c_type)
{
  int cost_hard = 0;
  for(unsigned int c = 0; c < in.ConstraintsVectorSize(c_type); c++)
  {  
      if(in.IsHard(c_type, c))
      {
        cost_single_constraints[c_type][c] = CalculateCostSingleConstraint(c_type, c);
        cost_hard += cost_single_constraints[c_type][c];
      }
  }
  cost_components_hard[c_type] = cost_hard;
  return cost_components_hard[c_type];
}

int STT_Solution::CalculateCostSingleConstraint(unsigned int c_type, unsigned int c) const
{
  int cost = 0;

    if(c_type == CA1)
    {
      for (auto t : in.team_group[in.constraints_CA1[c].team_group_index])
      {
        int games = 0;
        for (auto s : in.slot_group[in.constraints_CA1[c].slot_group_index])
        {
          if (in.constraints_CA1[c].mode == HOME && home[t][s])
            games++;
          else if (in.constraints_CA1[c].mode == AWAY && !home[t][s])
            games++;
        }
        cost += (in.constraints_CA1[c].hard ? stt_hard_weight*in.hard_weights[CA1] : 1) 
                * in.constraints_CA1[c].penalty 
                * max(0, max(in.constraints_CA1[c].k_min - games, games - in.constraints_CA1[c].k_max));
      }
      return cost;
    }
    else if(c_type == CA2)
    {
      for (auto t1 : in.team_group[in.constraints_CA2[c].team_group_1_index])
      {
        int games = 0;
        for (auto t2 : in.team_group[in.constraints_CA2[c].team_group_2_index])
          for (auto s : in.slot_group[in.constraints_CA2[c].slot_group_index])
          {
            if (opponent[t1][s] != t2)
              continue;
            if (in.constraints_CA2[c].mode == HOME && home[t1][s])
              games++;
            else if (in.constraints_CA2[c].mode == AWAY && !home[t1][s])
              games++;
            else if (in.constraints_CA2[c].mode == ANY)
              games++;
          }
        cost += (in.constraints_CA2[c].hard ? stt_hard_weight*in.hard_weights[CA2] : 1) 
                * in.constraints_CA2[c].penalty 
                * max(0, max(in.constraints_CA2[c].k_min - games, games - in.constraints_CA2[c].k_max));
      }
      return cost;
    }
    else if(c_type == CA3)
    {
      for (auto t1 : in.team_group[in.constraints_CA3[c].team_group_1_index])
      {
        vector<unsigned int> games(in.slots.size(), 0);
        for (auto t2 : in.team_group[in.constraints_CA3[c].team_group_2_index])
        {
          for (size_t s = 0; s < in.slots.size(); s++)
          {
            if (opponent[t1][s] != t2)
              continue;
            if (in.constraints_CA3[c].mode == HOME && home[t1][s])
              games[s]++;
            else if (in.constraints_CA3[c].mode == AWAY && !home[t1][s])
              games[s]++;
            else if (in.constraints_CA3[c].mode == ANY)
              games[s]++;
          }
        }
        for (size_t s = 0; s + in.constraints_CA3[c].k - 1 < in.slots.size(); s++)
        {
          int total = 0;
          for (int i = 0; i < in.constraints_CA3[c].k; i++)
            total += games[s + i];
          cost += (in.constraints_CA3[c].hard ? stt_hard_weight*in.hard_weights[CA3] : 1) 
                  * in.constraints_CA3[c].penalty 
                  * max(0, max(in.constraints_CA3[c].k_min - total, total - in.constraints_CA3[c].k_max));
        }
      }
      return cost;
    }
    else if(c_type == CA4)
    {
      if (in.constraints_CA4[c].mode2 == CA4Spec::GLOBAL)
      {
        int games = 0;
        for (auto t1 : in.team_group[in.constraints_CA4[c].team_group_1_index])
        {
          for (auto t2 : in.team_group[in.constraints_CA4[c].team_group_2_index])
            for (auto s : in.slot_group[in.constraints_CA4[c].slot_group_index])
            {
              if (opponent[t1][s] != t2)
                continue;
              if (in.constraints_CA4[c].mode1 == HOME && home[t1][s])
                games++;
              else if (in.constraints_CA4[c].mode1 == AWAY && !home[t1][s])
                games++;
              else if (in.constraints_CA4[c].mode1 == ANY)
                games++;
            }
        }
        cost += (in.constraints_CA4[c].hard ? stt_hard_weight*in.hard_weights[CA4] : 1) 
                * in.constraints_CA4[c].penalty 
                * max(0, max(in.constraints_CA4[c].k_min - games, games - in.constraints_CA4[c].k_max));
      }
      else // (in.constraints_CA4[c].mode2 == CA4Spec::EVERY)
      {
        for (auto s : in.slot_group[in.constraints_CA4[c].slot_group_index])
        {
          int games = 0;
          for (auto t1 : in.team_group[in.constraints_CA4[c].team_group_1_index])
          {
            for (auto t2 : in.team_group[in.constraints_CA4[c].team_group_2_index])
            {
              if (opponent[t1][s] != t2)
                continue;
              if (in.constraints_CA4[c].mode1 == HOME && home[t1][s])
                games++;
              else if (in.constraints_CA4[c].mode1 == AWAY && !home[t1][s])
                games++;
              else if (in.constraints_CA4[c].mode1 == ANY)
                games++;
            }
          }
          cost += (in.constraints_CA4[c].hard ? stt_hard_weight*in.hard_weights[CA4] : 1) 
                  * in.constraints_CA4[c].penalty 
                  * max(0, max(in.constraints_CA4[c].k_min - games, games - in.constraints_CA4[c].k_max));
        }
      }
      return cost;
    }
    else if(c_type == GA1)
    {
      int games = 0;
      for (auto m : in.constraints_GA1[c].meeting_group)
      {
        for (auto s : in.slot_group[in.constraints_GA1[c].slot_group_index])
        {
          if (!home[m.first][s])
            continue;
          if (opponent[m.first][s] == m.second)
            games++;
        }
      }
      cost += (in.constraints_GA1[c].hard ? stt_hard_weight*in.hard_weights[GA1] : 1) 
              * in.constraints_GA1[c].penalty 
              * max(0, max(in.constraints_GA1[c].k_min - games, games - in.constraints_GA1[c].k_max));
      
      return cost;
    }
    else if(c_type == BR1)
    {
      for (auto t : in.team_group[in.constraints_BR1[c].team_group_index])
      {
        int breaks = 0;
        for (auto s : in.slot_group[in.constraints_BR1[c].slot_group_index])
        {
          if (in.constraints_BR1[c].mode == HOME && s > 0 && home[t][s - 1] && home[t][s])
            breaks++;
          else if (in.constraints_BR1[c].mode == AWAY && s > 0 && !home[t][s - 1] && !home[t][s - 1])
            breaks++;
          else if (in.constraints_BR1[c].mode == ANY && s > 0 && home[t][s - 1] == home[t][s])
            breaks++;
        }
        cost += (in.constraints_BR1[c].hard ? stt_hard_weight*in.hard_weights[BR1] : 1) 
                * in.constraints_BR1[c].penalty 
                * max(0, breaks - in.constraints_BR1[c].k); 
      }
      return cost;
    }
    else if(c_type == BR2)
    {
      int breaks = 0;
      for (auto t : in.team_group[in.constraints_BR2[c].team_group_index])
      {
          for (auto s : in.slot_group[in.constraints_BR2[c].slot_group_index])
          {
              // the only considered mode is ANY
              if (s > 0 && home[t][s - 1] == home[t][s])
                  breaks++;
          }
      }
      cost += 
             (in.constraints_BR2[c].hard ? stt_hard_weight*in.hard_weights[BR2] : 1) 
             * in.constraints_BR2[c].penalty 
             * max(0, breaks - in.constraints_BR2[c].k);
      
      return cost;
    }
    else if(c_type == FA2)
    {
      for (size_t i = 0; i < in.team_group[in.constraints_FA2[c].team_group_index].size() - 1; i++)
      {
        
        for (size_t j = i + 1; j < in.team_group[in.constraints_FA2[c].team_group_index].size(); j++)
        {
          unsigned int t1 = in.team_group[in.constraints_FA2[c].team_group_index][i], t2 = in.team_group[in.constraints_FA2[c].team_group_index][j];
          int home_games_t1 = 0, home_games_t2 = 0;
          int current_home_games_difference = 0;
          int max_home_games_difference = 0;
          for (auto s : in.slot_group[in.constraints_FA2[c].slot_group_index])
          {
            // the only considered mode is HOME
            home_games_t1 += home[t1][s];
            home_games_t2 += home[t2][s];
            current_home_games_difference = fabs(home_games_t1 - home_games_t2);
            if(current_home_games_difference > max_home_games_difference)
              max_home_games_difference = current_home_games_difference;
            // According to the documentation: each pair of teams in teams triggers a deviation equal to the largest difference in played home games more than intp for each slot in slots.
          }
          if(max_home_games_difference > in.constraints_FA2[c].k)
          {
            cost += (in.constraints_FA2[c].hard ? stt_hard_weight*in.hard_weights[FA2] : 1)
            * in.constraints_FA2[c].penalty
            * max(0, max_home_games_difference - in.constraints_FA2[c].k);
          }
        }
      }
      return cost;
    }
    else if(c_type == SE1)
    {
      for (size_t i = 0; i < in.team_group[in.constraints_SE1[c].team_group_index].size() - 1; i++)
      {
        for (size_t j = i + 1; j < in.team_group[in.constraints_SE1[c].team_group_index].size(); j++)
        {
          unsigned int t1 = in.team_group[in.constraints_SE1[c].team_group_index][i], t2 = in.team_group[in.constraints_SE1[c].team_group_index][j];
          int first = -1, second = -1;
          for (size_t s = 0; s < in.slots.size(); s++)
          {
            // the only considered mode is SLOTS
            if (opponent[t1][s] == t2 && first == -1)
              first = s;
            else if (opponent[t1][s] == t2)
            {
              second = s;
              break;
            }
          }
          int distance = second - (first + 1);
          // According to the documentation: each pair of teams in teams triggers a deviation equal to the largest difference in played home games more than intp over all time slots in slots.
          cost += (in.constraints_SE1[c].hard ? stt_hard_weight*in.hard_weights[SE1] : 1) 
                  * in.constraints_SE1[c].penalty 
                  * max(0, max(in.constraints_SE1[c].m_min - distance, distance - in.constraints_SE1[c].m_max));
        }
      }
      return cost;
    }
  return cost;
}

float STT_Solution::GreedyCalculateCost(unsigned int r) const
{
  float total = 0.0;
  for (size_t i = 0; i < in.constraints_CA1.size(); i++)
    total += GreedyCalculateCostSingleConstraint(CA1, i, r);
  for (size_t i = 0; i < in.constraints_CA2.size(); i++)
    total += GreedyCalculateCostSingleConstraint(CA2, i, r);
  for (size_t i = 0; i < in.constraints_CA3.size(); i++)
    total += GreedyCalculateCostSingleConstraint(CA3, i, r);
  for (size_t i = 0; i < in.constraints_CA4.size(); i++)
    total += GreedyCalculateCostSingleConstraint(CA4, i, r);
  for (size_t i = 0; i < in.constraints_GA1.size(); i++)
    total += GreedyCalculateCostSingleConstraint(GA1, i, r);
  for (size_t i = 0; i < in.constraints_BR1.size(); i++)
    total += GreedyCalculateCostSingleConstraint(BR1, i, r);
  for (size_t i = 0; i < in.constraints_BR2.size(); i++)
    total += GreedyCalculateCostSingleConstraint(BR2, i, r);
  for (size_t i = 0; i < in.constraints_FA2.size(); i++)
    total += GreedyCalculateCostSingleConstraint(FA2, i, r);
  for (size_t i = 0; i < in.constraints_SE1.size(); i++)
    total += GreedyCalculateCostSingleConstraint(SE1, i, r);
  
  return total;
}

float STT_Solution::GreedyCalculateCostSingleConstraint(Constraints::ConstraintType c_type, unsigned int c, unsigned int r) const
{
  if (c_type == CA1)
  {
    float amount = 0.0;
    for (auto t : in.team_group[in.constraints_CA1[c].team_group_index])
    {
      int games = 0, considered_slots = 0, total_slots = 0;
      for (auto s : in.slot_group[in.constraints_CA1[c].slot_group_index])
      {
        total_slots++;
        if (s > r)
          continue;
        considered_slots++;
        if (in.constraints_CA1[c].mode == HOME && home[t][s])
          games++;
        else if (in.constraints_CA1[c].mode == AWAY && !home[t][s])
          games++;
      }
//      assert(total_slots == in.slot_group[in.constraints_CA1[c].slot_group_index].size());
      if (considered_slots == total_slots || games > in.constraints_CA1[c].k_max)
        // all slots have been considered, or they already are above the maximum
        amount += max(0, max(in.constraints_CA1[c].k_min - games, games - in.constraints_CA1[c].k_max));
      else
      {
        unsigned int available_slots = (in.slots.size() - 1 - r), remaining_slots = (total_slots - considered_slots);
        // some slots have not been considered yet, therefore an estimate should be given
        if (games < in.constraints_CA1[c].k_min && available_slots < remaining_slots)
          // not enough available slots anymore
          amount += (in.constraints_CA1[c].k_min - games) * float(available_slots / remaining_slots);
        else if (games >= in.constraints_CA1[c].k_min) // this is just a greedy measure
          amount += games / float(max(in.constraints_CA1[c].k_max, 1));
      }
    }
    return (in.constraints_CA1[c].hard ? stt_hard_weight*in.hard_weights[CA1] : 1) * in.constraints_CA1[c].penalty * amount;
  }
  else if (c_type == CA2)
  {
    float amount = 0.0;
    for (auto t1 : in.team_group[in.constraints_CA2[c].team_group_1_index])
    {
      int games = 0, considered_slots = 0, total_slots = 0;
      for (auto t2 : in.team_group[in.constraints_CA2[c].team_group_2_index])
      {
        for (auto s : in.slot_group[in.constraints_CA2[c].slot_group_index])
        {
          total_slots++;
          if (s > r)
            continue;
          considered_slots++;
          if (opponent[t1][s] != t2)
            continue;
          if (in.constraints_CA2[c].mode == HOME && home[t1][s])
            games++;
          else if (in.constraints_CA2[c].mode == AWAY && !home[t1][s])
            games++;
          else if (in.constraints_CA2[c].mode == ANY)
            games++;
        }
      }
//      assert(total_slots == in.slot_group[in.constraints_CA2[c].slot_group_index].size() * in.team_group[in.cgonstraints_CA2[c].team_group_2_index].size());
      if (considered_slots == total_slots || games > in.constraints_CA2[c].k_max)
      {
        // all slots have been considered, or they already are above the maximum
        amount += max(0, max(in.constraints_CA2[c].k_min - games, games - in.constraints_CA2[c].k_max));
      }
      else
      {
        unsigned int available_slots = (in.slots.size() - 1 - r) * in.team_group[in.constraints_CA2[c].team_group_2_index].size(), remaining_slots = (total_slots - considered_slots);
        // some slots have not been considered yet, therefore an estimate should be given
        if (games < in.constraints_CA2[c].k_min && available_slots < remaining_slots)
          // not enough available slots anymore
          amount += (in.constraints_CA2[c].k_min - games) * float(available_slots / remaining_slots);
        else if (games >= in.constraints_CA2[c].k_min) // this is just a greedy measure
          amount += games / float(max(in.constraints_CA2[c].k_max, 1));
      }
    }
    return (in.constraints_CA2[c].hard ? stt_hard_weight*in.hard_weights[CA2] : 1) * in.constraints_CA2[c].penalty * amount;
  }
  else if (c_type == CA3)
  {
    float amount = 0.0;
    for (auto t1 : in.team_group[in.constraints_CA3[c].team_group_1_index])
    {
      vector<unsigned int> games(in.slots.size(), 0);
      for (auto t2 : in.team_group[in.constraints_CA3[c].team_group_2_index])
      {
        for (size_t s = 0; s <= r; s++)
        {
          if (opponent[t1][s] != t2)
            continue;
          if (in.constraints_CA3[c].mode == HOME && home[t1][s])
            games[s]++;
          else if (in.constraints_CA3[c].mode == AWAY && !home[t1][s])
            games[s]++;
          else if (in.constraints_CA3[c].mode == ANY)
            games[s]++;
        }
      }
      for (size_t s = 0; s <= r; s++)
      {
        int total = 0;
        for (int i = 0; i < in.constraints_CA3[c].k && s + i <= r; i++)
           total += games[s + i];
        
        amount += max(0, max(in.constraints_CA3[c].k_min - total, total - in.constraints_CA3[c].k_max));
      }
    }
    return (in.constraints_CA3[c].hard ? stt_hard_weight*in.hard_weights[CA3] : 1) * in.constraints_CA3[c].penalty * amount;
  }
  else if (c_type == CA4)
  {
    float amount = 0.0;
    if (in.constraints_CA4[c].mode2 == CA4Spec::GLOBAL)
    {
      int games = 0;
      unsigned int considered_slots = 0, total_slots = 0;
      for (auto t1 : in.team_group[in.constraints_CA4[c].team_group_1_index])
      {
        for (auto t2 : in.team_group[in.constraints_CA4[c].team_group_2_index])
        {
          for (auto s : in.slot_group[in.constraints_CA4[c].slot_group_index])
          {
            total_slots++;
            if (s > r)
              continue;
            considered_slots++;
            if (opponent[t1][s] != t2)
              continue;
            if (in.constraints_CA4[c].mode1 == HOME && home[t1][s])
              games++;
            else if (in.constraints_CA4[c].mode1 == AWAY && !home[t1][s])
              games++;
            else if (in.constraints_CA4[c].mode1 == ANY)
              games++;
          }
        }
      }
//      assert(total_slots == in.slot_group[in.constraints_CA4[c].slot_group_index].size() * in.team_group[in.constraints_CA4[c].team_group_2_index].size() * in.team_group[in.constraints_CA4[c].team_group_1_index].size());
      if (considered_slots == total_slots || games > in.constraints_CA4[c].k_max)
      {
        // all slots have been considered, or they already are above the maximum
        amount += max(0, max(in.constraints_CA4[c].k_min - games, games - in.constraints_CA4[c].k_max));
      }
      else
      {
        unsigned int available_slots = (in.slots.size() - 1 - r) * in.team_group[in.constraints_CA4[c].team_group_2_index].size(), remaining_slots = (total_slots - considered_slots);
        // some slots have not been considered yet, therefore an estimate should be given
        if (games < in.constraints_CA4[c].k_min && available_slots < remaining_slots)
          // not enough available slots anymore
          amount += (in.constraints_CA4[c].k_min - games) * float(available_slots / remaining_slots);
        else if (games >= in.constraints_CA4[c].k_min) // this is just a greedy measure
          amount += games / float(max(in.constraints_CA4[c].k_max, 1));
      }
    }
    else // (in.constraints_CA4[c].mode2 == CA4Spec::EVERY)
    {
      for (auto s : in.slot_group[in.constraints_CA4[c].slot_group_index])
      {
        if (s > r)
          continue;
        int games = 0;
        for (auto t1 : in.team_group[in.constraints_CA4[c].team_group_1_index])
        {
          for (auto t2 : in.team_group[in.constraints_CA4[c].team_group_2_index])
          {
            if (opponent[t1][s] != t2)
              continue;
            if (in.constraints_CA4[c].mode1 == HOME && home[t1][s])
              games++;
            else if (in.constraints_CA4[c].mode1 == AWAY && !home[t1][s])
              games++;
            else if (in.constraints_CA4[c].mode1 == ANY)
              games++;
          }
        }
        amount += max(0, max(in.constraints_CA4[c].k_min - games, games - in.constraints_CA4[c].k_max));
      }
    }
    return (in.constraints_CA4[c].hard ? stt_hard_weight*in.hard_weights[CA4] : 1) * in.constraints_CA4[c].penalty * amount;
  }
  else if(c_type == GA1)
  {
    float amount = 0.0;
    int games = 0;
    unsigned int considered_slots = 0, total_slots = 0;
    for (auto m : in.constraints_GA1[c].meeting_group)
    {
      for (auto s : in.slot_group[in.constraints_GA1[c].slot_group_index])
      {
        total_slots++;
        if (s > r)
          continue;
        considered_slots++;
        if (!home[m.first][s])
          continue;
        if (opponent[m.first][s] == m.second)
          games++;
      }
    }
    if (considered_slots == total_slots || games > in.constraints_GA1[c].k_max)
    {
      // all slots have been considered, or they already are above the maximum
      amount += max(0, max(in.constraints_GA1[c].k_min - games, games - in.constraints_GA1[c].k_max));
    }
    else
    {
      unsigned int available_slots = (in.slots.size() - 1 - r) * in.constraints_GA1[c].meeting_group.size(), remaining_slots = (total_slots - considered_slots);
      // some slots have not been considered yet, therefore an estimate should be given
      if (games < in.constraints_GA1[c].k_min && available_slots < remaining_slots)
        // not enough available slots anymore
        amount += (in.constraints_GA1[c].k_min - games) * float(available_slots / remaining_slots);
      else if (games >= in.constraints_GA1[c].k_min) // this is just a greedy measure
        amount += games / float(max(in.constraints_GA1[c].k_max, 1));
    }
    return (in.constraints_GA1[c].hard ? stt_hard_weight*in.hard_weights[GA1] : 1) * in.constraints_GA1[c].penalty * amount;
  }
  else if(c_type == BR1)
  {
    float amount = 0.0;
    for (auto t : in.team_group[in.constraints_BR1[c].team_group_index])
    {
      int breaks = 0;
      for (auto s : in.slot_group[in.constraints_BR1[c].slot_group_index])
      {
        if (s > r)
          continue;
        if (in.constraints_BR1[c].mode == HOME && s > 0 && home[t][s - 1] && home[t][s])
          breaks++;
        else if (in.constraints_BR1[c].mode == AWAY && s > 0 && !home[t][s - 1] && !home[t][s - 1])
          breaks++;
        else if (in.constraints_BR1[c].mode == ANY && s > 0 && home[t][s - 1] == home[t][s])
          breaks++;
      }
      if (breaks > in.constraints_BR1[c].k)
      {
        // the number of breaks is already are above the maximum
        amount += max(0, breaks - in.constraints_BR1[c].k);
      }
      else
      {
        // this is just a greedy measure for keeping them minimal
        amount += breaks / float(max(in.constraints_BR1[c].k, 1));
      }
    }
    return (in.constraints_BR1[c].hard ? stt_hard_weight*in.hard_weights[BR1] : 1) * in.constraints_BR1[c].penalty * amount;
  }
  else if(c_type == BR2)
  {
    float amount = 0.0;
    int breaks = 0;
    for (auto t : in.team_group[in.constraints_BR2[c].team_group_index])
    {
      for (auto s : in.slot_group[in.constraints_BR2[c].slot_group_index])
      {
        if (s > r)
          continue;
        // the only considered mode is ANY
        if (s > 0 && home[t][s - 1] == home[t][s])
          breaks++;
      }
    }
    if (breaks > in.constraints_BR2[c].k)
    {
      // the number of breaks is already are above the maximum
      amount += max(0, breaks - in.constraints_BR2[c].k);
    }
    else
    {
      // this is just a greedy measure for keeping them minimal
      amount += breaks / float(max(in.constraints_BR2[c].k, 1));
    }
    return (in.constraints_BR2[c].hard ? stt_hard_weight*in.hard_weights[BR2] : 1) * in.constraints_BR2[c].penalty * amount;
  }
  else if (c_type == FA2)
  {
    float amount = 0.0;
    for (size_t i = 0; i < in.team_group[in.constraints_FA2[c].team_group_index].size() - 1; i++)
    {
      for (size_t j = i + 1; j < in.team_group[in.constraints_FA2[c].team_group_index].size(); j++)
      {
        unsigned int t1 = in.team_group[in.constraints_FA2[c].team_group_index][i], t2 = in.team_group[in.constraints_FA2[c].team_group_index][j];
        int home_games_t1 = 0, home_games_t2 = 0;
        int current_home_games_difference = 0;
        for (auto s : in.slot_group[in.constraints_FA2[c].slot_group_index])
        {
          if (s > r)
            continue;
          // the only considered mode is HOME
          home_games_t1 += home[t1][s];
          home_games_t2 += home[t2][s];
          current_home_games_difference = fabs(home_games_t1 - home_games_t2);
          if (current_home_games_difference > in.constraints_FA2[c].k)
          {
            // the value is already are above the maximum
            amount += max(0, current_home_games_difference - in.constraints_FA2[c].k);
          }
          else
          {
            // this is just a greedy measure for keeping the value minimal
            amount += current_home_games_difference / float(max(in.constraints_FA2[c].k, 1));
          }
          // According to the documentation: each pair of teams in teams triggers a deviation equal to the largest difference in played home games more than intp for each slot in slots.
        }
      }
    }
    return (in.constraints_FA2[c].hard ? stt_hard_weight*in.hard_weights[FA2] : 1) * in.constraints_FA2[c].penalty * amount;
  }
  else if (c_type == SE1)
  {
    float amount = 0.0;
    for (size_t i = 0; i < in.team_group[in.constraints_SE1[c].team_group_index].size() - 1; i++)
    {
      for (size_t j = i + 1; j < in.team_group[in.constraints_SE1[c].team_group_index].size(); j++)
      {
        unsigned int t1 = in.team_group[in.constraints_SE1[c].team_group_index][i], t2 = in.team_group[in.constraints_SE1[c].team_group_index][j];
        int first = -1, second = -1;
        unsigned int considered_slots = r, total_slots = in.slots.size();
        for (size_t s = 0; s <= r; s++)
        {
          // the only considered mode is SLOTS
          if (opponent[t1][s] == t2 && first == -1)
            first = s;
          else if (opponent[t1][s] == t2)
          {
            second = s;
            break;
          }
        }
        int distance = second - (first + 1);
        // According to the documentation: each pair of teams in teams triggers a deviation equal to the largest difference in played home games more than intp over all time slots in slots.
        if (considered_slots == total_slots || distance > in.constraints_SE1[c].m_max)
        {
          // all slots have been considered, or they already are above the maximum
          amount += max(0, max(in.constraints_SE1[c].m_min - distance, distance - in.constraints_SE1[c].m_max));
        }
        else
        {
          unsigned int available_slots = (in.slots.size() - 1 - r), remaining_slots = (total_slots - considered_slots);
          // some slots have not been considered yet, therefore an estimate should be given
          if (distance < in.constraints_SE1[c].m_min && available_slots < remaining_slots)
            // not enough available slots anymore
            amount += (in.constraints_SE1[c].m_min - distance) * float(available_slots / remaining_slots);
          else if (distance >= in.constraints_SE1[c].m_min) // this is just a greedy measure
            amount += distance / float(max(in.constraints_SE1[c].m_max, 1));
        }
      }
    }
    return (in.constraints_SE1[c].hard ? stt_hard_weight*in.hard_weights[SE1] : 1) * in.constraints_SE1[c].penalty * amount;
  }
  
  return std::numeric_limits<float>::infinity();
}

int STT_Solution::CalculateCostPhased()
{
    //qui possiamo fare che ogni coppia di partite (andata e ritorno) che si trovano nella stessa fase è una violazione
    int cost = 0;
    
    if(in.phased)
    {
      for(unsigned int i = 0; i < in.teams.size()-1; i++)
      {
        for(unsigned int j = i+1; j < in.teams.size(); j++)
        {
          if(SamePhase(match[i][j], match[j][i]))
          {
            cost+=stt_phased_weight*2; //in order to get the same value of the online validator of the ITC2021 competition, I count it twice 
          }
        }
      }
    }
    cost_phased = cost;
    return cost_phased;
}

void STT_Solution::UpdateSelectionedCostsConstraints(vector<vector<unsigned>> involved_constraints)
{
  //APPLICO le funzioni di costo alle constraints di tipo CA1
  int delta, delta_hard, new_cost, old_cost;
  for(unsigned int c_type = CA1; c_type <= SE1; c_type++)
  {
    if(c_type!=FA2) //NON APPLICO QUESTO CALCOLO PER FA2, CHE ESEGUO SEPARATAMENTE
    {
      delta = 0;
      delta_hard = 0;
      for(unsigned int c = 0; c < involved_constraints[c_type].size(); c++)
      {
        old_cost = cost_single_constraints[c_type][involved_constraints[c_type][c]];
        new_cost = CalculateCostSingleConstraint(static_cast<Constraints::ConstraintType>(c_type), involved_constraints[c_type][c]);
        delta = delta - old_cost + new_cost;
        if(in.IsHard(c_type, involved_constraints[c_type][c]))
          delta_hard = delta_hard - old_cost + new_cost;
        cost_single_constraints[c_type][involved_constraints[c_type][c]] = new_cost;
      } 
      cost_components[c_type] = cost_components[c_type] + delta;
      cost_components_hard[c_type] = cost_components_hard[c_type] + delta_hard;
      total_cost_components = total_cost_components + delta;
      total_cost_components_hard = total_cost_components_hard + delta_hard;
    }
  }

  //per adesso FA2 lo ricalcoliamo sempre per completo, visto che quando è presente riguarda sempre tutti i team e tutti gli slots.
  //TO DO per una prossima fase: gestire anche FA2 solo per involved_constraints
  //Questi tre forse ci conviene farlo anche per BR2 e SE1, visto che ce n'è al più uno solo per istanza
  total_cost_components = total_cost_components - cost_components[FA2];
  total_cost_components_hard = total_cost_components_hard - cost_components_hard[FA2];
  CalculateCostComponent(FA2);
  CalculateCostComponentHard(FA2);
  total_cost_components = total_cost_components + cost_components[FA2];
  total_cost_components_hard = total_cost_components_hard + cost_components_hard[FA2];
}

//versione unsigned
vector<unsigned> STT_Solution::getInvolvedConstraints(Constraints::ConstraintType c_type, unsigned t1, unsigned t2, unsigned s) const
{
  vector<unsigned> involved_constraints(0,0);
  bool already_added;
  for (unsigned c : in.team_slot_constraints[c_type][t1][s])
  {
    involved_constraints.push_back(c);
  }

  for (unsigned c : in.team_slot_constraints[c_type][t2][s])
  {
    //al secondo giro, evito di aggiungere quelle che avevo già aggiunto per [t1][s]
    already_added = false;
    for(unsigned i = 0; i < involved_constraints.size(); i++)
      if(involved_constraints[i] == c)
      {
        already_added = true;
        break;
      }
    if(!already_added)
      involved_constraints.push_back(c);
  }
  return involved_constraints;
}

//versione vettoriale
vector<unsigned> STT_Solution::getInvolvedConstraints(Constraints::ConstraintType c_type, vector<unsigned> involved_teams, vector<unsigned> involved_slots) const
{
  vector<unsigned> involved_constraints(0,0);
  bool already_added;

  for(unsigned t = 0; t < involved_teams.size(); t++)
    for(unsigned s = 0; t < involved_slots.size(); s++)
      for (unsigned c : in.team_slot_constraints[c_type][involved_teams[t]][involved_slots[s]])
      {
        if(s == 0 && t == 0)
          involved_constraints.push_back(c);
        else
        {
          //al secondo giro, evito di aggiungere quelle che avevo già aggiunto per [t1][s]
          already_added = false;
          for(unsigned i = 0; i < involved_constraints.size(); i++)
            if(involved_constraints[i] == c)
            {
              already_added = true;
              break;
            }
          if(!already_added)
            involved_constraints.push_back(c);
        }
      }

  return involved_constraints;
}

void STT_Solution::UpdateLocalCostsDeleteMatch(unsigned t1, unsigned t2, unsigned old_r)
{
    vector<unsigned> involved_teams(0,0);
    vector<unsigned> involved_slots(0,0);
    involved_teams.push_back(t1);
    involved_teams.push_back(t2);
    involved_slots.push_back(old_r);
      
    //CA1
    vector<unsigned> involved_constraints = getInvolvedConstraints(CA1, t1, t2, old_r);


}

void STT_Solution::UpdateLocalCostsInsertMatch(unsigned t1, unsigned t2, unsigned r)
{
  //CA1
  vector<unsigned> involved_constraints = getInvolvedConstraints(CA1, t1, t2, r);
}

void STT_Solution::RecalculateConstraintsForTeamsMatches(vector<unsigned> involved_teams, vector<unsigned> involved_matches)
{
  //costo prima del ricalcolo
  
  //CA1
  vector<unsigned> involved_constraints = getInvolvedConstraints(CA1, involved_teams, involved_matches);
}

bool STT_Solution::SamePhase(unsigned int r1, unsigned int r2) const
{
  //se r1 è nel primo girone e r2 nel secondo, o se r1 è nel secondo girone e r2 nel primo
  if((r1 < in.slots.size()/2 && r2 >= in.slots.size()/2) || (r1 >= in.slots.size()/2 && r2 < in.slots.size()/2))
  {
    return false;
  }
  return true;
}

void STT_Solution::UpdateMoveCounterAndBestSolution(int new_cost)
{
  if(new_cost < last_best_solution) //se vogliamo considerare anche le sideway moves cambiare < con <=
  {
    last_best_solution = new_cost;
    last_best_counter = move_counter;
  }
  move_counter++;
}

void STT_Solution::DisplayOFIfNeeded()
{
  if(move_counter % 1000 == 0 || move_counter < 100)
  {
    cout << "moves = " << move_counter << " | total_cost = " << total_cost_components + cost_phased
    << ", obj = " << total_cost_components - total_cost_components_hard 
    << ", hard = " << total_cost_components_hard  
    << ", phased = " << cost_phased
    << "| best = " << last_best_solution
    << "| CA1: " << cost_components[CA1] 
      << ", CA2: " << cost_components[CA2] 
      << ", CA3: " << cost_components[CA3] 
      << ", CA4: " << cost_components[CA4] 
      << ", GA1: " << cost_components[GA1] 
      << ", BR1: " << cost_components[BR1] 
      << ", BR2: " << cost_components[BR2] 
      << ", FA2: " << cost_components[FA2] 
      << ", SE1: " << cost_components[SE1] << endl;
    //InternalCheckConsistency();
  }
  
}

// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 1: swap homes              ********************
// ***************************************************************************************

bool operator==(const STT_SwapHomes &m1, const STT_SwapHomes &m2)
{
  return m1.t1 == m2.t1 && m1.t2 == m2.t2;
}
bool operator!=(const STT_SwapHomes &m1, const STT_SwapHomes &m2)
{
  return (m1.t1 != m2.t1 || m1.t2 != m2.t2);
}
bool operator<(const STT_SwapHomes &m1, const STT_SwapHomes &m2)
{
  return m1.t1 < m2.t1 || (m1.t1 == m2.t1  && m1.t2 < m2.t2);
}
ostream& operator<<(ostream& os, const STT_SwapHomes& m)
{
  return os << '<' << m.t1 << '-' << m.t2 << '>';
}

istream& operator>>(istream& is, STT_SwapHomes& m)
{
  char ch;
  is  >> ch >> m.t1 >> ch >> m.t2 >> ch ;
  return is;
}

// ***************************************************************************************
// **********       METHODS FOR NEIGHBORHOOD 1+: Improved swap homes       ***************
// ***************************************************************************************

bool operator==(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2)
{
  return m1.t1 == m2.t1 && m1.t2 == m2.t2 && m1.r1 == m2.r1 && m1.r2 == m2.r2;
}
bool operator!=(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2)
{
  return (m1.t1 != m2.t1 || m1.t2 != m2.t2 || m1.r1 != m2.r1 || m1.r2 != m2.r2);
}
bool operator<(const STT_ImprovedSwapHomes &m1, const STT_ImprovedSwapHomes &m2)
{
  return m1.t1 < m2.t1 || (m1.t1 == m2.t1  && m1.t2 < m2.t2); // PER ADESSO CONSIDERIAMO COMUNQUE SOLO I TEAM? || (m1.t1 == m2.t1  && m1.t2 == m2.t2 && m1.r1 < m2.r1) ||  (m1.t1 == m2.t1  && m1.t2 == m2.t2 && m1.r1 == m2.r1 && m1.r1 < m1.r2);
}
ostream& operator<<(ostream& os, const STT_ImprovedSwapHomes& m)
{
  return os << '<' << m.t1 << '-' << m.t2 << '%' << '[' << m.r1 << "," << m.r2 << "]";
}

istream& operator>>(istream& is, STT_ImprovedSwapHomes& m)
{
  char ch;
  is  >> ch >> m.t1 >> ch >> m.t2 >> ch >> ch >> m.r1 >> ch >> m.r2 >> ch;
  return is;
}

// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 2: swap teams              ********************
// ***************************************************************************************

bool operator==(const STT_SwapTeams &m1, const STT_SwapTeams &m2)
{
    return m1.t1 == m2.t1 && m1.t2 == m2.t2;
}

bool operator!=(const STT_SwapTeams &m1, const STT_SwapTeams &m2)
{
    return (m1.t1 != m2.t1 || m1.t2 != m2.t2);
}

bool operator<(const STT_SwapTeams &m1, const STT_SwapTeams &m2)
{
    return m1.t1 < m2.t1 || (m1.t1 == m2.t1  && m1.t2 < m2.t2);
}

ostream& operator<<(ostream& os, const STT_SwapTeams& m)
{
  return os << '<' << m.t1 << '-' << m.t2<< '>';
}
  
istream& operator>>(istream& is, STT_SwapTeams& m)
{
  char ch;
  is >> ch >> m.t1 >> ch >> m.t2 >> ch ;
  return is;
}

// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 3: swap rounds              *******************
// ***************************************************************************************

bool operator==(const STT_SwapRounds &m1, const STT_SwapRounds &m2)
{
  return m1.r1 == m2.r1 && m1.r2 == m2.r2;
}

bool operator!=(const STT_SwapRounds &m1, const STT_SwapRounds &m2)
{
  return m1.r1 != m2.r1 || m1.r2 != m2.r2;
}

bool operator<(const STT_SwapRounds &m1, const STT_SwapRounds &m2)
{
  return m1.r1 < m2.r1 || (m1.r1 == m2.r1  && m1.r2 < m2.r2);
}

ostream& operator<<(ostream& os, const STT_SwapRounds& m)
{
  return os << '<' << m.r1 << '-' << m.r2 << '>';
}

istream& operator>>(istream& is, STT_SwapRounds& m)
{
  char ch;
  is  >> ch>> m.r1 >> ch >> m.r2 >> ch;
  return is;
}

// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 4: swap matches NOT PHASED  *******************
// ***************************************************************************************

bool operator==(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2)
{
  return m1.t1 == m2.t1 && m1.t2 == m2.t2 && m1.rs[0] == m2.rs[0];
}

bool operator!=(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2)
{
  return m1.t1 != m2.t1 || m1.t2 != m2.t2 || m1.rs[0] != m2.rs[0];
}

bool operator<(const STT_SwapMatchesNotPhased &m1, const STT_SwapMatchesNotPhased &m2)
{
  return m1.t1 < m2.t1 || (m1.t1 == m2.t1 && m1.t2 < m2.t2) || (m1.t1 == m2.t1  && m1.t2 == m2.t2 && m1.rs[0] < m2.rs[0]);
}

ostream& operator<<(ostream& os, const STT_SwapMatchesNotPhased& m)
{
  unsigned i;
  os << '<' << m.t1 << '-' << m.t2 << "%[";
  for (i = 0; i < m.rs.size()-1; i++)
    os << m.rs[i] << ",";
  os << m.rs[i] << "]>";
  return os;
}

istream& operator>>(istream& is, STT_SwapMatchesNotPhased& m)
{
  char ch;
  unsigned r;
  is >> ch >> m.t1 >> ch >> m.t2 >> ch >> ch;
  m.rs.clear();
  do 
    {
      is >> r >> ch;
      m.rs.push_back(r);
    }
  while (ch == ',');
  is >> ch;
  return is;
}


// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 5: swap matches PHASED      *******************
// ***************************************************************************************

bool operator==(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2)
{
  return m1.t1 == m2.t1 && m1.t2 == m2.t2 && m1.rs[0] == m2.rs[0];
}

bool operator!=(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2)
{
  return m1.t1 != m2.t1 || m1.t2 != m2.t2 || m1.rs[0] != m2.rs[0];
}

bool operator<(const STT_SwapMatchesPhased &m1, const STT_SwapMatchesPhased &m2)
{
  return m1.t1 < m2.t1 || (m1.t1 == m2.t1 && m1.t2 < m2.t2) || (m1.t1 == m2.t1  && m1.t2 == m2.t2 && m1.rs[0] < m2.rs[0]);
}

ostream& operator<<(ostream& os, const STT_SwapMatchesPhased& m)
{
  unsigned i;
  os << '<' << m.t1 << '-' << m.t2 << "%[";
  for (i = 0; i < m.rs.size()-1; i++)
    os << m.rs[i] << ",";
  os << m.rs[i] << "]>";
  return os;
}

istream& operator>>(istream& is, STT_SwapMatchesPhased& m)
{
  char ch;
  unsigned r;
  is >> ch >> m.t1 >> ch >> m.t2 >> ch >> ch;
  m.rs.clear();
  do 
    {
      is >> r >> ch;
      m.rs.push_back(r);
    }
  while (ch == ',');
  is >> ch;
  return is;
}

// ***************************************************************************************
// ************** METHODS FOR NEIGHBORHOOD 6: swap rounds              *******************
// ***************************************************************************************

bool operator==(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2)
{
  return m1.ts[0] == m2.ts[0] && m1.r1 == m2.r1 && m1.r2 == m2.r2;
}

bool operator!=(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2)
{
  return m1.ts[0] != m2.ts[0] || m1.r1 != m2.r1 || m1.r2 != m2.r2;
}


bool operator<(const STT_SwapMatchRound &m1, const STT_SwapMatchRound &m2)
{
  return m1.r1 < m2.r1 || (m1.r1 == m2.r1 && m1.r2 < m2.r2) || (m1.r1 == m2.r1  && m1.r2 == m2.r2 && m1.ts[0] < m2.ts[0]);
}

ostream& operator<<(ostream& os, const STT_SwapMatchRound& m)
{
  unsigned i;
  os << "<[";
  for (i = 0; i < m.ts.size()-1; i++)
    os << m.ts[i] << ",";
  os << m.ts[i] << "]";
  return os << '^' << m.r1 << '-' << m.r2 << '>';
}

istream& operator>>(istream& is, STT_SwapMatchRound& m)
{
  char ch;
  unsigned t;
  m.ts.clear();
  is >> ch >> ch;
  do 
    {
      is >> t >> ch;
      m.ts.push_back(t);
    }
  while (ch == ',');
  is >> ch >> m.r1 >> ch >> m.r2 >> ch;
  return is;
}
