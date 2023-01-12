#include "stt_data.hh"
#include "stt_helpers.hh"
#include <easylocal.hh>
#include <array>

//2-stages SA:

//1st stage (in1) : I read only hard constraints

//2n stage (in2) : I read all constraints

using namespace EasyLocal::Core;
using namespace EasyLocal::Debug;

int main(int argc, const char* argv[]) {

    ParameterBox main_parameters("main", "Main Program options");
    ParameterBox SA_parameters("SA", "Simulated Annealing, full run with HARD and SOFT costs");
    ParameterBox STAGE1_parameters("STAGE1", "Simulated Annealing options - Stage 1");
    ParameterBox STAGE1_2_parameters("STAGE1_2", "Simulated Annealing options - Stage 1_2");
    ParameterBox STAGE2_parameters("STAGE2", "Simulated Annealing options - Stage 2");
    ParameterBox NH_parameters("NH", "Neighborhoods options");
    ParameterBox HW_parameters("HW", "Hard Components Weights");

    Parameter<string> instance("instance", "Input instance", main_parameters);
    Parameter<long int> seed("seed", "Random seed", main_parameters);
    Parameter<string> method("method", "Solution method (empty for tester)", main_parameters);
    Parameter<string> init_state("init_state", "Initial state (to be read from file)", main_parameters);
    Parameter<string> output_file("output_file", "Write the output to a file (filename required)", main_parameters);
    Parameter<int> hard_weight("hard_weight", "Global Hard weight", main_parameters);
    Parameter<int> phased_weight("phased_weight", "Phased Weight, if not set the same of hard_weight will be applied", main_parameters);
    Parameter<bool> mix_initial_phase("mix_initial_phase", "Mix Initial Phase for Random State, default=true", main_parameters);
    Parameter<bool> mix_phase_during_search("mix_phase_during_search", "Mix Phase during search, default=true", main_parameters);
    Parameter<bool> display_OF("DisplayObjFunc", "display obj function decrease", main_parameters);
    Parameter<int> test_stage("test_stage", "activate tester for stage 1 or 2", main_parameters);
    Parameter<bool> use_hard_coded_parameters("use_hcp", "Use Hard Coded Parameters, NOTE: will ignore any other command line parameter, default: false", main_parameters);
    Parameter<bool> verbose_mode("verbose_mode", "Activate Verbose Mode, NOTE: not compatible with Json2Run, default: false", main_parameters);
    Parameter<double> correlation_factor("correlation_factor", "In ESA-3S Multilplies obtains hard weight multipling this factor per the number of hard constraints", main_parameters);
    Parameter<bool> correlate_with_n_hard_constraints("correlate_with_n_hard_constraints", "In ESA-3S Multilplies obtains hard weight multipling correlation factor per the number of hard constraints, default = FALSE. Works only with use_hard_coded_parameters", main_parameters);
    Parameter<bool> disable_pswtp("disable_pswtp", "disable move partial_swap_teams_phased (used just for tuning purposes)", main_parameters);
    Parameter<string> start_type("start_type", "possible valuses: random, greedy or vizing. default: random (ignored if use_hard_coded_parameters is active)", main_parameters);
    Parameter<bool> j2rmode("j2rmode", "if true, prints the output on a single line. Default: false", main_parameters);
    Parameter<bool> print_full_solution("print_full_solution", "if true, prints the full solutions at the end. Ignored if verbose_mode is active. Default: true", main_parameters);


    Parameter<double> swap_teams_rate("swap_teams_rate", "Probability of move swap_teams", NH_parameters);
    Parameter<double> swap_rounds_rate("swap_rounds_rate", "Probability of move swap_rounds", NH_parameters);
    Parameter<double> swap_matches_notphased_rate("swap_matches_notphased_rate", "Probability of move swap_matches_notphased_rate", NH_parameters);
    Parameter<double> swap_matches_phased_rate("swap_matches_phased_rate", "Probability of move swap_matches_phased_rate", NH_parameters);
    Parameter<double> swap_matchround_rate("swap_matchround_rate", "Probability of move swap_matchround", NH_parameters);
    Parameter<double> swap_double_swap_homes_rate("swap_double_swap_homes_rate", "Probability of move swap_double_swap_homes_rate", NH_parameters);
    Parameter<double> swap_triple_swap_homes_rate("swap_triple_swap_homes_rate", "Probability of move swap_triple_swap_homes_rate", NH_parameters);
    Parameter<double> swap_double_swap_matchrounds_rate("swap_double_swap_rounds_rate", "Probability of move swap_double_swap_rounds_rate", NH_parameters);
    Parameter<double> swap_round_swap_homes_rate("swap_round_swap_homes_rate", "Probability of move swap_round_swap_homes_rate", NH_parameters);
    Parameter<double> swap_matches_notphased_rate_swap_matchround_rate("swap_matches_notphased_rate_swap_matchround_rate", "Probability of move swap_matches_notphased_rate_swap_matchround", NH_parameters);
    Parameter<double> swap_matches_phased_rate_swap_matchround_rate("swap_matches_phased_rate_swap_matchround_rate", "Probability of move swap_matches_phased_rate_swap_matchround", NH_parameters);

    Parameter<double> start_temperature_0("start_temperature", "Full Simulated Annealing start_temperature", SA_parameters);
    Parameter<double> cooling_rate_0("cooling_rate", "Full Simulated Annealing cooling_rate", SA_parameters);
    Parameter<unsigned long int> max_evaluations_0("max_evaluations", "Full Simulated Annealing max_evaluations", SA_parameters);
    Parameter<double> expected_min_temperature_0("expected_min_temperature", "Full Simulated Annealing expected_min_temperature", SA_parameters);
    Parameter<double> neighbors_accepted_ratio_0("neighbors_accepted_ratio", "Full Simulated Annealing neighbors_accepted_ratio", SA_parameters);

    Parameter<double> start_temperature_1("start_temperature", "Stage 1 start_temperature", STAGE1_parameters);
    Parameter<double> cooling_rate_1("cooling_rate", "Stage 1 :cooling_rate", STAGE1_parameters);
    Parameter<unsigned long int> max_evaluations_1("max_evaluations", "Stage 1 max_evaluations", STAGE1_parameters);
    Parameter<double> expected_min_temperature_1("expected_min_temperature", "Stage 1 expected_min_temperature", STAGE1_parameters);
    Parameter<double> neighbors_accepted_ratio_1("neighbors_accepted_ratio", "Stage 1 neighbors_accepted_ratio", STAGE1_parameters);

    Parameter<double> start_temperature_1_2("start_temperature", "Stage 1_2 start_temperature", STAGE1_2_parameters);
    Parameter<double> cooling_rate_1_2("cooling_rate", "Stage 1_2 :cooling_rate", STAGE1_2_parameters);
    Parameter<unsigned long int> max_evaluations_1_2("max_evaluations", "Stage 1_2 max_evaluations", STAGE1_2_parameters);
    Parameter<double> expected_min_temperature_1_2("expected_min_temperature", "Stage 1_2 expected_min_temperature", STAGE1_2_parameters);
    Parameter<double> neighbors_accepted_ratio_1_2("neighbors_accepted_ratio", "Stage 1_2 neighbors_accepted_ratio", STAGE1_2_parameters);

    Parameter<double> start_temperature_2("start_temperature", "Stage 2 start_temperature", STAGE2_parameters);
    Parameter<double> cooling_rate_2("cooling_rate", "Stage 2 cooling_rate", STAGE2_parameters);
    Parameter<unsigned long int> max_evaluations_2("max_evaluations", "Stage 2 max_evaluations", STAGE2_parameters);
    Parameter<double> expected_min_temperature_2("expected_min_temperature", "Stage 2 expected_min_temperature", STAGE2_parameters);
    Parameter<double> neighbors_accepted_ratio_2("neighbors_accepted_ratio", "Stage 2 neighbors_accepted_ratio", STAGE2_parameters);

    //pesi dei vincoli hard
    Parameter<int> hw_ca1("CA1", "CA1 Weight", HW_parameters);
    Parameter<int> hw_ca2("CA2", "CA2 Weight", HW_parameters);
    Parameter<int> hw_ca3("CA3", "CA3 Weight", HW_parameters);
    Parameter<int> hw_ca4("CA4", "CA4 Weight", HW_parameters);
    Parameter<int> hw_ga1("GA1", "GA1 Weight", HW_parameters);
    Parameter<int> hw_br1("BR1", "BR2 Weight", HW_parameters);
    Parameter<int> hw_br2("BR2", "BR2 Weight", HW_parameters);
    Parameter<int> hw_fa2("FA2", "FA2 Weight", HW_parameters);
    Parameter<int> hw_se1("SE1", "SE1 Weight", HW_parameters);

    swap_teams_rate = 0.1;
    swap_rounds_rate = 0.1;
    swap_matches_notphased_rate = 0.2;
    swap_matches_phased_rate = 0.2;
    swap_matchround_rate = 0.2;
    swap_double_swap_homes_rate = 0.0;
    swap_triple_swap_homes_rate = 0.0;
    swap_double_swap_matchrounds_rate = 0.0;
    swap_round_swap_homes_rate = 0.0;
    swap_matches_phased_rate_swap_matchround_rate = 0.0;
    swap_matches_notphased_rate_swap_matchround_rate = 0.0;
    mix_initial_phase = true; //default value
    mix_phase_during_search = true; //default value
    use_hard_coded_parameters = false; //default value
    display_OF = false;
    verbose_mode = false;
    correlate_with_n_hard_constraints = false;
    disable_pswtp = false;
    start_type = "random";
    j2rmode = false;
    print_full_solution = true;

    //HARD WEIGHTS
    hw_ca1 = 1;
    hw_ca2 = 1;
    hw_ca3 = 1;
    hw_ca4 = 1;
    hw_ga1 = 1;
    hw_br1 = 1;
    hw_br2 = 1;
    hw_fa2 = 1;
    hw_se1 = 1;
    
    //double swap_matches_notphased_rate, swap_matches_phased_rate;

    const int DEFAULT_HARD_WEIGHT = 1000000; 
    hard_weight = DEFAULT_HARD_WEIGHT;
    phased_weight = DEFAULT_HARD_WEIGHT;

    CommandLineParameters::Parse(argc, argv, false, true);

    bool vizing_greedy = false;
    int hard_weight_1 = hard_weight;
    int phased_weight_1 = phased_weight;
    int hard_weight_1_2 = hard_weight;
    int phased_weight_1_2 = phased_weight;

    if(start_type.IsSet())
    {
      if(static_cast<string>(start_type) != "vizing" && static_cast<string>(start_type) != "greedy" && static_cast <string>(start_type) != "random")
      {
        cout << "Incorrect value for --main::start_type, please select one of the following (with no capital letters): [random, greedy, vizing]" << endl;
        exit(1);
      }
      if(static_cast<string>(start_type) == "vizing")
        vizing_greedy = true;
    }

    if(use_hard_coded_parameters)
    {

      vizing_greedy = true;
      hw_ca1 = 7;
      hw_ca2 = 8;
      hw_ca3 = 2;
      hw_ca4 = 8;
      hw_ga1 = 10;
      hw_br1 = 1;
      hw_br2 = 6;
      hw_fa2 = 1;
      hw_se1 = 1;

      if(!method.IsSet())
      {
        cout << "use_hard_coded_parameters enabled and method is not set. Method must be set" << endl;
        return 0;
      }

      //SA PARAMETERS
      start_temperature_0 = 600;
      expected_min_temperature_0 = 3.52;
      cooling_rate_0 = 0.99;
      neighbors_accepted_ratio_0 = 0.1;


      start_temperature_1 = 179; //*100
      expected_min_temperature_1 = 2.1; //*100
      cooling_rate_1 = 0.99;
      neighbors_accepted_ratio_1 = 0.1;
      hard_weight_1 = 10;
      phased_weight_1 = 117;

      // start_temperature_1 = 600;
      // expected_min_temperature_1 = 3.52;
      // cooling_rate_1 = 0.99;
      // neighbors_accepted_ratio_1 = 0.1;

      start_temperature_1_2 = 600;
      expected_min_temperature_1_2 = 3.52;
      cooling_rate_1_2 = 0.99;
      neighbors_accepted_ratio_1_2 = 0.1;

      // SECOND STAGE (I DON'T ALLOW HARD WORSENING MOVES)
      start_temperature_2 = 17.9;
      expected_min_temperature_2 = 0.21;
      cooling_rate_2 = 0.99;
      neighbors_accepted_ratio_2 = 0.1;

      // HARD AND PHASED WEIGHTS
      if(method == "ESA-2S" || method == "ESA-2S-OH" || method == "ESA-3S")
      {
        hard_weight = 40;
        phased_weight = 400;
        hard_weight_1_2 = 40;
        phased_weight_1_2 = 400;
      }
      else if(method == "ESA-0")
      {
        hard_weight = 40;
        phased_weight = 400;
      }

      correlation_factor = 0.5;
    }

    if (!instance.IsSet())
    {
        cout << "Error: --main::instance filename option must always be set" << endl;
        return 1;
    }  
    if (seed.IsSet())
        Random::SetSeed(seed);
    
    array<int, N_CONSTRAINTS> hw_array = {hw_ca1, hw_ca2, hw_ca3, hw_ca4, hw_ga1, hw_br1, hw_br2, hw_fa2, hw_se1};
    
    bool only_hard_in_stage1 = true; //ESA-2S-OH e ESA-3S
    bool exit_at_zero_hard_stage1 = true; //facciamo che questo sia sempre falso
    
    if(method.IsSet() && method == "ESA-2S")
    {
      only_hard_in_stage1 = false;
      exit_at_zero_hard_stage1 = false;
    }

    STT_Input in0(instance, hard_weight, phased_weight, false, mix_phase_during_search, false, false, hw_array);  //all constraints, I DON'T forbid hard worsening moves (for SA unique stage)
    
    //unsigned n_constraints = in0.constraints_CA1.size() + in0.constraints_CA2.size() + in0.constraints_CA3.size() + in0.constraints_CA4.size() + in0.constraints_GA1.size() + in0.constraints_BR1.size() + in0.constraints_BR2.size() + in0.constraints_FA2.size() + in0.constraints_SE1.size();
    unsigned n_hard_n_constraints = in0.constraints_hard_indexes[CA1].size() + in0.constraints_hard_indexes[CA2].size() + in0.constraints_hard_indexes[CA3].size() + in0.constraints_hard_indexes[CA4].size() + in0.constraints_hard_indexes[GA1].size() + in0.constraints_hard_indexes[BR1].size() + in0.constraints_hard_indexes[BR2].size() + in0.constraints_hard_indexes[FA2].size() + in0.constraints_hard_indexes[SE1].size();
    
    STT_Input in1(instance, hard_weight_1, phased_weight_1, only_hard_in_stage1, mix_phase_during_search, false, exit_at_zero_hard_stage1, hw_array);  //only hard, I don't forbid hard worsening moves
    
    if(use_hard_coded_parameters && correlate_with_n_hard_constraints)
    {
      hard_weight_1_2 = static_cast<int>(n_hard_n_constraints*correlation_factor);
      phased_weight_1_2 = 10*hard_weight_1_2;
    }
    
    STT_Input in1_2(instance, hard_weight_1_2, phased_weight_1_2, false, mix_phase_during_search, false, false, hw_array);  //only hard, I don't forbid hard worsening moves
    STT_Input in2(instance, hard_weight, phased_weight, false, in1.phased ? false : mix_phase_during_search, true, false, hw_array);  //all constraints, I forbid hard worsening moves
    
    if(use_hard_coded_parameters && !max_evaluations_1.IsSet() && !max_evaluations_2.IsSet() && !max_evaluations_1_2.IsSet())
    {
      //per farla durare circa un'ora applichiamo la formula sottostante,
      // pilotando il primo numero cambia il tempo concesso 
      //double expected_duration = 0.05; //expected duration in hours
      //max_evaluations_0 = static_cast<long unsigned int>(expected_duration*20000000000/n_constraints);
      //la lunghezza dello stage 1 dipenderà dalla complessità dell'istanza
      if(in1.phased && n_hard_n_constraints >= 200)  //in questo caso 500 mln
      {
        max_evaluations_1 = 500000000;  //5000000 for races, 500000000 for batches
        max_evaluations_1_2 = 50000000; //500000 for races, 50000000 for batches
      }
      else
      {
        max_evaluations_1 = 20000000;   //200000 for races, 20000000 for batches
        max_evaluations_1_2 = 250000000; //2500000 for races, 250000000 for batches 
      }     
      max_evaluations_2 = 40000;   //0 for races, 40000 for batches
    }

    if(!in1.phased && mix_phase_during_search == false)
    {
      cout << "Error: instance is not phased and mix_phase_during_search set as false" << endl;
      return 0;
    }

    if(mix_initial_phase == true && mix_phase_during_search == false)
    {
      cout << "Error: mix_initial_phase set as true and mix_phase_during_search set as false: unacceptable combinations" << endl;
      return 0;
    }

    //should start now implementation of costs: hard and soft
    STT_SolutionManager sm0(in0, "STT_SolutionManager_0", in0.phased ? false : mix_initial_phase, display_OF, vizing_greedy);
    STT_SolutionManager sm1(in1, "STT_SolutionManager_1", in1.phased ? false : mix_initial_phase, display_OF, vizing_greedy);
    STT_SolutionManager sm1_2(in1_2, "STT_SolutionManager_1_2", in1_2.phased ? false : mix_initial_phase, display_OF, vizing_greedy);
    STT_SolutionManager sm2(in2, "STT_SolutionManager_2", in2.phased ? false : mix_initial_phase, display_OF, vizing_greedy);    

    //aggiungo i costi a sm0
    CA1CostComponent ca1_0(in0, 1, false, "CA1_0");
    sm0.AddCostComponent(ca1_0);
    CA2CostComponent ca2_0(in0, 1, false, "CA2_0");
    sm0.AddCostComponent(ca2_0);
    CA3CostComponent ca3_0(in0, 1, false, "CA3_0");
    sm0.AddCostComponent(ca3_0);
    CA4CostComponent ca4_0(in0, 1, false, "CA4_0");
    sm0.AddCostComponent(ca4_0);
    GA1CostComponent ga1_0(in0, 1, false, "GA1_0");
    sm0.AddCostComponent(ga1_0);
    BR1CostComponent br1_0(in0, 1, false, "BR1_0");
    sm0.AddCostComponent(br1_0);
    BR2CostComponent br2_0(in0, 1, false, "BR2_0");
    sm0.AddCostComponent(br2_0);
    FA2CostComponent fa2_0(in0, 1, false, "FA2_0");
    sm0.AddCostComponent(fa2_0);
    SE1CostComponent se1_0(in0, 1, false, "SE1_0");
    sm0.AddCostComponent(se1_0);
    PhasedCostComponent phs_0(in0, 1, false, "phased_0"); 
    if(in0.phased)
    {
      sm0.AddCostComponent(phs_0);
    }


    //aggiungo i costi a sm1
    CA1CostComponent ca1_1(in1, 1, false, "CA1_1");
    sm1.AddCostComponent(ca1_1);
    CA2CostComponent ca2_1(in1, 1, false, "CA2_1");
    sm1.AddCostComponent(ca2_1);
    CA3CostComponent ca3_1(in1, 1, false, "CA3_1");
    sm1.AddCostComponent(ca3_1);
    CA4CostComponent ca4_1(in1, 1, false, "CA4_1");
    sm1.AddCostComponent(ca4_1);
    GA1CostComponent ga1_1(in1, 1, false, "GA1_1");
    sm1.AddCostComponent(ga1_1);
    BR1CostComponent br1_1(in1, 1, false, "BR1_1");
    sm1.AddCostComponent(br1_1);
    BR2CostComponent br2_1(in1, 1, false, "BR2_1");
    sm1.AddCostComponent(br2_1);
    FA2CostComponent fa2_1(in1, 1, false, "FA2_1");
    sm1.AddCostComponent(fa2_1);
    SE1CostComponent se1_1(in1, 1, false, "SE1_1");
    sm1.AddCostComponent(se1_1);
    PhasedCostComponent phs_1(in1, 1, false, "phased_1"); 
    if(in1.phased)
    {
      sm1.AddCostComponent(phs_1);
    }

    CA1CostComponent ca1_1_2(in1_2, 1, false, "CA1_1_2");
    sm1_2.AddCostComponent(ca1_1_2);
    CA2CostComponent ca2_1_2(in1_2, 1, false, "CA2_1_2");
    sm1_2.AddCostComponent(ca2_1_2);
    CA3CostComponent ca3_1_2(in1_2, 1, false, "CA3_1_2");
    sm1_2.AddCostComponent(ca3_1_2);
    CA4CostComponent ca4_1_2(in1_2, 1, false, "CA4_1_2");
    sm1_2.AddCostComponent(ca4_1_2);
    GA1CostComponent ga1_1_2(in1_2, 1, false, "GA1_1_2");
    sm1_2.AddCostComponent(ga1_1_2);
    BR1CostComponent br1_1_2(in1_2, 1, false, "BR1_1_2");
    sm1_2.AddCostComponent(br1_1_2);
    BR2CostComponent br2_1_2(in1_2, 1, false, "BR2_1_2");
    sm1_2.AddCostComponent(br2_1_2);
    FA2CostComponent fa2_1_2(in1_2, 1, false, "FA2_1_2");
    sm1_2.AddCostComponent(fa2_1_2);
    SE1CostComponent se1_1_2(in1_2, 1, false, "SE1_1_2");
    sm1_2.AddCostComponent(se1_1_2);
    PhasedCostComponent phs_1_2(in1_2, 1, false, "phased_1_2"); 
    if(in1.phased)
    {
      sm1_2.AddCostComponent(phs_1_2);
    }

    //aggiungo i costi a sm2
    //qui tutti i costi assumono il costo di default
    CA1CostComponent ca1_2(in2, 1, false, "CA1_2");
    sm2.AddCostComponent(ca1_2);
    CA2CostComponent ca2_2(in2, 1, false, "CA2_2");
    sm2.AddCostComponent(ca2_2);
    CA3CostComponent ca3_2(in2, 1, false, "CA3_2");
    sm2.AddCostComponent(ca3_2);
    CA4CostComponent ca4_2(in2, 1, false, "CA4_2");
    sm2.AddCostComponent(ca4_2);
    GA1CostComponent ga1_2(in2, 1, false, "GA1_2");
    sm2.AddCostComponent(ga1_2);
    BR1CostComponent br1_2(in2, 1, false, "BR1_2");
    sm2.AddCostComponent(br1_2);
    BR2CostComponent br2_2(in2, 1, false, "BR2_2");
    sm2.AddCostComponent(br2_2);
    FA2CostComponent fa2_2(in2, 1, false, "FA2_2");
    sm2.AddCostComponent(fa2_2);
    SE1CostComponent se1_2(in2, 1, false, "SE1_2");
    sm2.AddCostComponent(se1_2);
    PhasedCostComponent phs_2(in2, 1, false, "phased_2"); //passo costo "1" perché ho già incorporato il peso in in1.stt_phased_weight
    if(in2.phased)
    {
      sm2.AddCostComponent(phs_2);
    }

    //Creo i NH per il SA a unico stage
    STT_SwapHomesNeighborhoodExplorer STT_swap_homes_nh0(in0, sm0);
    STT_swap_homes_nh0.AddCostComponent(ca1_0);
    STT_swap_homes_nh0.AddCostComponent(ca2_0);
    STT_swap_homes_nh0.AddCostComponent(ca3_0);
    STT_swap_homes_nh0.AddCostComponent(ca4_0);
    STT_swap_homes_nh0.AddCostComponent(ga1_0);
    STT_swap_homes_nh0.AddCostComponent(br1_0);
    STT_swap_homes_nh0.AddCostComponent(br2_0);
    STT_swap_homes_nh0.AddCostComponent(fa2_0);
    STT_swap_homes_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_homes_nh0.AddCostComponent(phs_0);
    }

    STT_SwapTeamsNeighborhoodExplorer STT_swap_teams_nh0(in0, sm0);
    STT_swap_teams_nh0.AddCostComponent(ca1_0);
    STT_swap_teams_nh0.AddCostComponent(ca2_0);
    STT_swap_teams_nh0.AddCostComponent(ca3_0);
    STT_swap_teams_nh0.AddCostComponent(ca4_0);
    STT_swap_teams_nh0.AddCostComponent(ga1_0);
    STT_swap_teams_nh0.AddCostComponent(br1_0);
    STT_swap_teams_nh0.AddCostComponent(br2_0);
    STT_swap_teams_nh0.AddCostComponent(fa2_0);
    STT_swap_teams_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_teams_nh0.AddCostComponent(phs_0);
    }

    STT_SwapRoundsNeighborhoodExplorer STT_swap_rounds_nh0(in0, sm0);
    STT_swap_rounds_nh0.AddCostComponent(ca1_0);
    STT_swap_rounds_nh0.AddCostComponent(ca2_0);
    STT_swap_rounds_nh0.AddCostComponent(ca3_0);
    STT_swap_rounds_nh0.AddCostComponent(ca4_0);
    STT_swap_rounds_nh0.AddCostComponent(ga1_0);
    STT_swap_rounds_nh0.AddCostComponent(br1_0);
    STT_swap_rounds_nh0.AddCostComponent(br2_0);
    STT_swap_rounds_nh0.AddCostComponent(fa2_0);
    STT_swap_rounds_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_rounds_nh0.AddCostComponent(phs_0);
    }


    STT_SwapMatchesNotPhasedNeighborhoodExplorer STT_swap_matches_notphased_nh0(in0, sm0);
    STT_swap_matches_notphased_nh0.AddCostComponent(ca1_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(ca2_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(ca3_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(ca4_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(ga1_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(br1_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(br2_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(fa2_0);
    STT_swap_matches_notphased_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_matches_notphased_nh0.AddCostComponent(phs_0);
    }

    STT_SwapMatchesPhasedNeighborhoodExplorer STT_swap_matches_phased_nh0(in0, sm0);
    STT_swap_matches_phased_nh0.AddCostComponent(ca1_0);
    STT_swap_matches_phased_nh0.AddCostComponent(ca2_0);
    STT_swap_matches_phased_nh0.AddCostComponent(ca3_0);
    STT_swap_matches_phased_nh0.AddCostComponent(ca4_0);
    STT_swap_matches_phased_nh0.AddCostComponent(ga1_0);
    STT_swap_matches_phased_nh0.AddCostComponent(br1_0);
    STT_swap_matches_phased_nh0.AddCostComponent(br2_0);
    STT_swap_matches_phased_nh0.AddCostComponent(fa2_0);
    STT_swap_matches_phased_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_matches_phased_nh0.AddCostComponent(phs_0);
    }

    STT_SwapMatchRoundNeighborhoodExplorer STT_swap_match_round_nh0(in0, sm0);
    STT_swap_match_round_nh0.AddCostComponent(ca1_0);
    STT_swap_match_round_nh0.AddCostComponent(ca2_0);
    STT_swap_match_round_nh0.AddCostComponent(ca3_0);
    STT_swap_match_round_nh0.AddCostComponent(ca4_0);
    STT_swap_match_round_nh0.AddCostComponent(ga1_0);
    STT_swap_match_round_nh0.AddCostComponent(br1_0);
    STT_swap_match_round_nh0.AddCostComponent(br2_0);
    STT_swap_match_round_nh0.AddCostComponent(fa2_0);
    STT_swap_match_round_nh0.AddCostComponent(se1_0);
    if(in0.phased)
    {
      STT_swap_match_round_nh0.AddCostComponent(phs_0);
    }
    
    //Creo i NH per il il primo stage
    STT_SwapHomesNeighborhoodExplorer STT_swap_homes_nh1(in1, sm1);
    STT_swap_homes_nh1.AddCostComponent(ca1_1);
    STT_swap_homes_nh1.AddCostComponent(ca2_1);
    STT_swap_homes_nh1.AddCostComponent(ca3_1);
    STT_swap_homes_nh1.AddCostComponent(ca4_1);
    STT_swap_homes_nh1.AddCostComponent(ga1_1);
    STT_swap_homes_nh1.AddCostComponent(br1_1);
    STT_swap_homes_nh1.AddCostComponent(br2_1);
    STT_swap_homes_nh1.AddCostComponent(fa2_1);
    STT_swap_homes_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_homes_nh1.AddCostComponent(phs_1);
    }

    STT_SwapTeamsNeighborhoodExplorer STT_swap_teams_nh1(in1, sm1);
    STT_swap_teams_nh1.AddCostComponent(ca1_1);
    STT_swap_teams_nh1.AddCostComponent(ca2_1);
    STT_swap_teams_nh1.AddCostComponent(ca3_1);
    STT_swap_teams_nh1.AddCostComponent(ca4_1);
    STT_swap_teams_nh1.AddCostComponent(ga1_1);
    STT_swap_teams_nh1.AddCostComponent(br1_1);
    STT_swap_teams_nh1.AddCostComponent(br2_1);
    STT_swap_teams_nh1.AddCostComponent(fa2_1);
    STT_swap_teams_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_teams_nh1.AddCostComponent(phs_1);
    }

    STT_SwapRoundsNeighborhoodExplorer STT_swap_rounds_nh1(in1, sm1);
    STT_swap_rounds_nh1.AddCostComponent(ca1_1);
    STT_swap_rounds_nh1.AddCostComponent(ca2_1);
    STT_swap_rounds_nh1.AddCostComponent(ca3_1);
    STT_swap_rounds_nh1.AddCostComponent(ca4_1);
    STT_swap_rounds_nh1.AddCostComponent(ga1_1);
    STT_swap_rounds_nh1.AddCostComponent(br1_1);
    STT_swap_rounds_nh1.AddCostComponent(br2_1);
    STT_swap_rounds_nh1.AddCostComponent(fa2_1);
    STT_swap_rounds_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_rounds_nh1.AddCostComponent(phs_1);
    }


    STT_SwapMatchesNotPhasedNeighborhoodExplorer STT_swap_matches_notphased_nh1(in1, sm1);
    STT_swap_matches_notphased_nh1.AddCostComponent(ca1_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(ca2_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(ca3_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(ca4_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(ga1_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(br1_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(br2_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(fa2_1);
    STT_swap_matches_notphased_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_matches_notphased_nh1.AddCostComponent(phs_1);
    }

    STT_SwapMatchesPhasedNeighborhoodExplorer STT_swap_matches_phased_nh1(in1, sm1);
    STT_swap_matches_phased_nh1.AddCostComponent(ca1_1);
    STT_swap_matches_phased_nh1.AddCostComponent(ca2_1);
    STT_swap_matches_phased_nh1.AddCostComponent(ca3_1);
    STT_swap_matches_phased_nh1.AddCostComponent(ca4_1);
    STT_swap_matches_phased_nh1.AddCostComponent(ga1_1);
    STT_swap_matches_phased_nh1.AddCostComponent(br1_1);
    STT_swap_matches_phased_nh1.AddCostComponent(br2_1);
    STT_swap_matches_phased_nh1.AddCostComponent(fa2_1);
    STT_swap_matches_phased_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_matches_phased_nh1.AddCostComponent(phs_1);
    }

    STT_SwapMatchRoundNeighborhoodExplorer STT_swap_match_round_nh1(in1, sm1);
    STT_swap_match_round_nh1.AddCostComponent(ca1_1);
    STT_swap_match_round_nh1.AddCostComponent(ca2_1);
    STT_swap_match_round_nh1.AddCostComponent(ca3_1);
    STT_swap_match_round_nh1.AddCostComponent(ca4_1);
    STT_swap_match_round_nh1.AddCostComponent(ga1_1);
    STT_swap_match_round_nh1.AddCostComponent(br1_1);
    STT_swap_match_round_nh1.AddCostComponent(br2_1);
    STT_swap_match_round_nh1.AddCostComponent(fa2_1);
    STT_swap_match_round_nh1.AddCostComponent(se1_1);
    if(in1.phased)
    {
      STT_swap_match_round_nh1.AddCostComponent(phs_1);
    }

    //Stage 1_2
    //Creo i NH per il il primo stage
    STT_SwapHomesNeighborhoodExplorer STT_swap_homes_nh1_2(in1_2, sm1_2);
    STT_swap_homes_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_homes_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_homes_nh1_2.AddCostComponent(phs_1_2);
    }

    STT_SwapTeamsNeighborhoodExplorer STT_swap_teams_nh1_2(in1_2, sm1_2);
    STT_swap_teams_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_teams_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_teams_nh1_2.AddCostComponent(phs_1_2);
    }

    STT_SwapRoundsNeighborhoodExplorer STT_swap_rounds_nh1_2(in1_2, sm1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_rounds_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_rounds_nh1_2.AddCostComponent(phs_1_2);
    }


    STT_SwapMatchesNotPhasedNeighborhoodExplorer STT_swap_matches_notphased_nh1_2(in1_2, sm1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_matches_notphased_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_matches_notphased_nh1_2.AddCostComponent(phs_1_2);
    }

    STT_SwapMatchesPhasedNeighborhoodExplorer STT_swap_matches_phased_nh1_2(in1_2, sm1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_matches_phased_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_matches_phased_nh1_2.AddCostComponent(phs_1_2);
    }

    STT_SwapMatchRoundNeighborhoodExplorer STT_swap_match_round_nh1_2(in1_2, sm1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(ca1_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(ca2_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(ca3_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(ca4_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(ga1_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(br1_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(br2_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(fa2_1_2);
    STT_swap_match_round_nh1_2.AddCostComponent(se1_1_2);
    if(in1_2.phased)
    {
      STT_swap_match_round_nh1_2.AddCostComponent(phs_1_2);
    }

    //Creo i NH per il secondo stage
    STT_SwapHomesNeighborhoodExplorer STT_swap_homes_nh2(in2, sm2);
    STT_swap_homes_nh2.AddCostComponent(ca1_2);
    STT_swap_homes_nh2.AddCostComponent(ca2_2);
    STT_swap_homes_nh2.AddCostComponent(ca3_2);
    STT_swap_homes_nh2.AddCostComponent(ca4_2);
    STT_swap_homes_nh2.AddCostComponent(ga1_2);
    STT_swap_homes_nh2.AddCostComponent(br1_2);
    STT_swap_homes_nh2.AddCostComponent(br2_2);
    STT_swap_homes_nh2.AddCostComponent(fa2_2);
    STT_swap_homes_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_homes_nh2.AddCostComponent(phs_2);
    }

    STT_SwapTeamsNeighborhoodExplorer STT_swap_teams_nh2(in2, sm2);
    STT_swap_teams_nh2.AddCostComponent(ca1_2);
    STT_swap_teams_nh2.AddCostComponent(ca2_2);
    STT_swap_teams_nh2.AddCostComponent(ca3_2);
    STT_swap_teams_nh2.AddCostComponent(ca4_2);
    STT_swap_teams_nh2.AddCostComponent(ga1_2);
    STT_swap_teams_nh2.AddCostComponent(br1_2);
    STT_swap_teams_nh2.AddCostComponent(br2_2);
    STT_swap_teams_nh2.AddCostComponent(fa2_2);
    STT_swap_teams_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_teams_nh2.AddCostComponent(phs_2);
    }

    STT_SwapRoundsNeighborhoodExplorer STT_swap_rounds_nh2(in2, sm2);
    STT_swap_rounds_nh2.AddCostComponent(ca1_2);
    STT_swap_rounds_nh2.AddCostComponent(ca2_2);
    STT_swap_rounds_nh2.AddCostComponent(ca3_2);
    STT_swap_rounds_nh2.AddCostComponent(ca4_2);
    STT_swap_rounds_nh2.AddCostComponent(ga1_2);
    STT_swap_rounds_nh2.AddCostComponent(br1_2);
    STT_swap_rounds_nh2.AddCostComponent(br2_2);
    STT_swap_rounds_nh2.AddCostComponent(fa2_2);
    STT_swap_rounds_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_rounds_nh2.AddCostComponent(phs_2);
    }


    STT_SwapMatchesNotPhasedNeighborhoodExplorer STT_swap_matches_notphased_nh2(in2, sm2);
    STT_swap_matches_notphased_nh2.AddCostComponent(ca1_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(ca2_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(ca3_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(ca4_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(ga1_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(br1_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(br2_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(fa2_2);
    STT_swap_matches_notphased_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_matches_notphased_nh2.AddCostComponent(phs_2);
    }

    STT_SwapMatchesPhasedNeighborhoodExplorer STT_swap_matches_phased_nh2(in2, sm2);
    STT_swap_matches_phased_nh2.AddCostComponent(ca1_2);
    STT_swap_matches_phased_nh2.AddCostComponent(ca2_2);
    STT_swap_matches_phased_nh2.AddCostComponent(ca3_2);
    STT_swap_matches_phased_nh2.AddCostComponent(ca4_2);
    STT_swap_matches_phased_nh2.AddCostComponent(ga1_2);
    STT_swap_matches_phased_nh2.AddCostComponent(br1_2);
    STT_swap_matches_phased_nh2.AddCostComponent(br2_2);
    STT_swap_matches_phased_nh2.AddCostComponent(fa2_2);
    STT_swap_matches_phased_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_matches_phased_nh2.AddCostComponent(phs_2);
    }

    STT_SwapMatchRoundNeighborhoodExplorer STT_swap_match_round_nh2(in2, sm2);
    STT_swap_match_round_nh2.AddCostComponent(ca1_2);
    STT_swap_match_round_nh2.AddCostComponent(ca2_2);
    STT_swap_match_round_nh2.AddCostComponent(ca3_2);
    STT_swap_match_round_nh2.AddCostComponent(ca4_2);
    STT_swap_match_round_nh2.AddCostComponent(ga1_2);
    STT_swap_match_round_nh2.AddCostComponent(br1_2);
    STT_swap_match_round_nh2.AddCostComponent(br2_2);
    STT_swap_match_round_nh2.AddCostComponent(fa2_2);
    STT_swap_match_round_nh2.AddCostComponent(se1_2);
    if(in2.phased)
    {
      STT_swap_match_round_nh2.AddCostComponent(phs_2);
    }

    //PROBABILITIES
    if (use_hard_coded_parameters || !swap_teams_rate.IsSet() || !swap_rounds_rate.IsSet() || !swap_matches_phased_rate.IsSet() || !swap_matches_notphased_rate.IsSet() || !swap_matchround_rate.IsSet())
    { // insert rates based on the feature "phased"
      if(!in1.phased)
      {
         if(disable_pswtp)
         {
            swap_teams_rate = 0.08;
            swap_rounds_rate = 0.03;
            swap_matches_notphased_rate = 0.34;
            swap_matches_phased_rate = 0.00;
            swap_matchround_rate = 0.38;
         }
         else
         {
            swap_teams_rate = 0.07;
            swap_rounds_rate = 0.025;
            swap_matches_notphased_rate = 0.319;
            swap_matches_phased_rate = 0.07;
            swap_matchround_rate = 0.35;
         }
          
      }
      else
      {      
          if(disable_pswtp)
          {
            swap_teams_rate = 0.02;
            swap_rounds_rate = 0.09;
            swap_matches_notphased_rate = 0.14;
            swap_matches_phased_rate = 0.00;
            swap_matchround_rate = 0.60;
          }
          else
          {
            swap_teams_rate = 0.02;
            swap_rounds_rate = 0.08;
            swap_matches_notphased_rate = 0.12; 
            swap_matches_phased_rate = 0.13;
            swap_matchround_rate = 0.52;
          }
          
      }
    }      
    array<double, 6> rates_6{max(0.0, 1.0 - swap_teams_rate - swap_rounds_rate - swap_matches_notphased_rate - swap_matches_phased_rate - swap_matchround_rate), swap_teams_rate, swap_rounds_rate, swap_matches_notphased_rate, swap_matches_phased_rate, swap_matchround_rate};

    //ESAMODAL Unique Stage
    SetUnionNeighborhoodExplorer<STT_Input, STT_Solution, DefaultCostStructure<int>, 
    decltype(STT_swap_homes_nh0), decltype(STT_swap_teams_nh0), 
    decltype(STT_swap_rounds_nh0), decltype(STT_swap_matches_notphased_nh0),
    decltype(STT_swap_matches_phased_nh0), decltype(STT_swap_match_round_nh0)> 
    STT_esamodal_nh0(in0, sm0, "Esamodal_Stage0", STT_swap_homes_nh0, STT_swap_teams_nh0, 
                        STT_swap_rounds_nh0, STT_swap_matches_notphased_nh0, STT_swap_matches_phased_nh0, 
                        STT_swap_match_round_nh0, rates_6);

    //ESAMODAL Stage 1
    SetUnionNeighborhoodExplorer<STT_Input, STT_Solution, DefaultCostStructure<int>, 
    decltype(STT_swap_homes_nh1), decltype(STT_swap_teams_nh1), 
    decltype(STT_swap_rounds_nh1), decltype(STT_swap_matches_notphased_nh1),
    decltype(STT_swap_matches_phased_nh1), decltype(STT_swap_match_round_nh1)> 
    STT_esamodal_nh1(in1, sm1, "Esamodal_Stage1", STT_swap_homes_nh1, STT_swap_teams_nh1, 
                        STT_swap_rounds_nh1, STT_swap_matches_notphased_nh1, STT_swap_matches_phased_nh1, 
                        STT_swap_match_round_nh1, rates_6);

    //ESAMODAL Stage 1_2
    SetUnionNeighborhoodExplorer<STT_Input, STT_Solution, DefaultCostStructure<int>, 
    decltype(STT_swap_homes_nh1_2), decltype(STT_swap_teams_nh1_2), 
    decltype(STT_swap_rounds_nh1_2), decltype(STT_swap_matches_notphased_nh1_2),
    decltype(STT_swap_matches_phased_nh1_2), decltype(STT_swap_match_round_nh1_2)> 
    STT_esamodal_nh1_2(in1_2, sm1_2, "Esamodal_Stage1_2", STT_swap_homes_nh1_2, STT_swap_teams_nh1_2, 
                        STT_swap_rounds_nh1_2, STT_swap_matches_notphased_nh1_2, STT_swap_matches_phased_nh1_2, 
                        STT_swap_match_round_nh1_2, rates_6);
    
    //ESAMODAL Stage 2
    SetUnionNeighborhoodExplorer<STT_Input, STT_Solution, DefaultCostStructure<int>, 
    decltype(STT_swap_homes_nh2), decltype(STT_swap_teams_nh2), 
    decltype(STT_swap_rounds_nh2), decltype(STT_swap_matches_notphased_nh2),
    decltype(STT_swap_matches_phased_nh2), decltype(STT_swap_match_round_nh2)> 
    STT_esamodal_nh2(in2, sm2, "Esamodal_Stage2", STT_swap_homes_nh2, STT_swap_teams_nh2, 
                        STT_swap_rounds_nh2, STT_swap_matches_notphased_nh2, STT_swap_matches_phased_nh2, 
                        STT_swap_match_round_nh2, rates_6);
 

    //Solver Unique Stage
    SimulatedAnnealingEvaluationBased<STT_Input, STT_Solution, decltype(STT_esamodal_nh0)::MoveType> STT_ESA_0(in0, sm0, STT_esamodal_nh0, "ESA_0");// "STT_EsamodalSimulatedAnnealing");
    SimpleLocalSearch<STT_Input, STT_Solution> STT_solver_0(in0, sm0, "STT_solver_0");
    if(use_hard_coded_parameters || static_cast<string>(start_type) != "random")
      STT_solver_0.SetParameter("random_state", false);
    else
      STT_solver_0.SetParameter("random_state", true);

    //Solver Stage 1
    SimulatedAnnealingEvaluationBased<STT_Input, STT_Solution, decltype(STT_esamodal_nh1)::MoveType> STT_ESA_1(in1, sm1, STT_esamodal_nh1, "ESA_1");// "STT_EsamodalSimulatedAnnealing");
    SimpleLocalSearch<STT_Input, STT_Solution> STT_solver_1(in1, sm1, "STT_solver_1");
    if(use_hard_coded_parameters || static_cast<string>(start_type) != "random")
      STT_solver_1.SetParameter("random_state", false);
    else
      STT_solver_1.SetParameter("random_state", true);

    //Solver Stage 1_2
    SimulatedAnnealingEvaluationBased<STT_Input, STT_Solution, decltype(STT_esamodal_nh1_2)::MoveType> STT_ESA_1_2(in1_2, sm1_2, STT_esamodal_nh1_2, "ESA_1_2");// "STT_EsamodalSimulatedAnnealing");
    SimpleLocalSearch<STT_Input, STT_Solution> STT_solver_1_2(in1_2, sm1_2, "STT_solver_1_2");

    //Solver Stage 2
    SimulatedAnnealingEvaluationBased<STT_Input, STT_Solution, decltype(STT_esamodal_nh2)::MoveType> STT_ESA_2(in2, sm2, STT_esamodal_nh2, "ESA_2");// "STT_EsamodalSimulatedAnnealing");
    SimpleLocalSearch<STT_Input, STT_Solution> STT_solver_2(in2, sm2, "STT_solver_2");

    //Tester Unique Stage
    Tester<STT_Input, STT_Solution> tester0(in0, sm0);
    MoveTester<STT_Input, STT_Solution, STT_SwapHomes> swap_homes_tester0(in0, sm0, STT_swap_homes_nh0, "STT_SwapHomes0", tester0);
    MoveTester<STT_Input, STT_Solution, STT_SwapTeams> swap_teams_tester0(in0, sm0, STT_swap_teams_nh0, "STT_SwapTeams0", tester0);
    MoveTester<STT_Input, STT_Solution, STT_SwapRounds> swap_rounds_tester0(in0, sm0, STT_swap_rounds_nh0, "STT_SwapRounds0", tester0);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesNotPhased> swap_matches_tester0(in0, sm0, STT_swap_matches_notphased_nh0, "STT_SwapMatchesNotPhased0", tester0);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesPhased> swap_matches_phased_tester0(in0, sm0, STT_swap_matches_phased_nh0, "STT_SwapMatchesPhased0", tester0);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchRound> swap_match_round_tester0(in0, sm0, STT_swap_match_round_nh0, "STT_SwapMatchRound0", tester0); 
    MoveTester<STT_Input, STT_Solution, decltype(STT_esamodal_nh0)::MoveType> esamodal_tester0(in0, sm0, STT_esamodal_nh0, "STT_Esamodal0", tester0); 
    
    //Tester Stage 1
    Tester<STT_Input, STT_Solution> tester1(in1, sm1);
    MoveTester<STT_Input, STT_Solution, STT_SwapHomes> swap_homes_tester1(in1, sm1, STT_swap_homes_nh1, "STT_SwapHomes1", tester1);
    MoveTester<STT_Input, STT_Solution, STT_SwapTeams> swap_teams_tester1(in1, sm1, STT_swap_teams_nh1, "STT_SwapTeams1", tester1);
    MoveTester<STT_Input, STT_Solution, STT_SwapRounds> swap_rounds_tester1(in1, sm1, STT_swap_rounds_nh1, "STT_SwapRounds1", tester1);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesNotPhased> swap_matches_tester1(in1, sm1, STT_swap_matches_notphased_nh1, "STT_SwapMatchesNotPhased1", tester1);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesPhased> swap_matches_phased_tester1(in1, sm1, STT_swap_matches_phased_nh1, "STT_SwapMatchesPhased1", tester1);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchRound> swap_match_round_tester1(in1, sm1, STT_swap_match_round_nh1, "STT_SwapMatchRound1", tester1); 
    MoveTester<STT_Input, STT_Solution, decltype(STT_esamodal_nh1)::MoveType> esamodal_tester1(in1, sm1, STT_esamodal_nh1, "STT_Esamodal1", tester1); 
    
    //Tester Stage 1_2
    Tester<STT_Input, STT_Solution> tester1_2(in1_2, sm1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapHomes> swap_homes_tester1_2(in1_2, sm1_2, STT_swap_homes_nh1_2, "STT_SwapHomes1_2", tester1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapTeams> swap_teams_tester1_2(in1_2, sm1_2, STT_swap_teams_nh1_2, "STT_SwapTeams1_2", tester1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapRounds> swap_rounds_tester1_2(in1_2, sm1_2, STT_swap_rounds_nh1_2, "STT_SwapRounds1_2", tester1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesNotPhased> swap_matches_tester1_2(in1_2, sm1_2, STT_swap_matches_notphased_nh1_2, "STT_SwapMatchesNotPhased1_2", tester1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesPhased> swap_matches_phased_tester1_2(in1_2, sm1_2, STT_swap_matches_phased_nh1_2, "STT_SwapMatchesPhased1_2", tester1_2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchRound> swap_match_round_tester1_2(in1_2, sm1_2, STT_swap_match_round_nh1_2, "STT_SwapMatchRound1_2", tester1_2); 
    MoveTester<STT_Input, STT_Solution, decltype(STT_esamodal_nh1_2)::MoveType> esamodal_tester1_2(in1_2, sm1_2, STT_esamodal_nh1_2, "STT_Esamodal1_2", tester1_2); 
    
    //Tester Stage 2
    Tester<STT_Input, STT_Solution> tester2(in2, sm2);
    MoveTester<STT_Input, STT_Solution, STT_SwapHomes> swap_homes_tester2(in2, sm2, STT_swap_homes_nh2, "STT_SwapHomes2", tester2);
    MoveTester<STT_Input, STT_Solution, STT_SwapTeams> swap_teams_tester2(in2, sm2, STT_swap_teams_nh2, "STT_SwapTeams2", tester2);
    MoveTester<STT_Input, STT_Solution, STT_SwapRounds> swap_rounds_tester2(in2, sm2, STT_swap_rounds_nh2, "STT_SwapRounds2", tester2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesNotPhased> swap_matches_tester2(in2, sm2, STT_swap_matches_notphased_nh2, "STT_SwapMatchesNotPhased2", tester2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchesPhased> swap_matches_phased_tester2(in2, sm2, STT_swap_matches_phased_nh2, "STT_SwapMatchesPhased2", tester2);
    MoveTester<STT_Input, STT_Solution, STT_SwapMatchRound> swap_match_round_tester2(in2, sm2, STT_swap_match_round_nh2, "STT_SwapMatchRound2", tester2); 
    MoveTester<STT_Input, STT_Solution, decltype(STT_esamodal_nh2)::MoveType> esamodal_tester2(in2, sm2, STT_esamodal_nh2, "STT_Esamodal2", tester2); 
    
    if (!CommandLineParameters::Parse(argc, argv, true, false))
      return 1;

  //for testing purposes
  if (test_stage.IsSet())
  { 
    if (static_cast<int>(test_stage) == 0)
    {
      if(init_state.IsSet())
        tester0.RunMainMenu(init_state);
      else
        tester0.RunMainMenu();
    }
    else if (static_cast<int>(test_stage) == 1)
    {
      if(init_state.IsSet())
        tester1.RunMainMenu(init_state);
      else
        tester1.RunMainMenu();
    }
    else if (static_cast<int>(test_stage) == 2)
    {
      if(init_state.IsSet())
        tester2.RunMainMenu(init_state);
      else
      {
        cout << "tester stage 2 requires --main::init_state" << endl;
        return 0;
      }
    }
    else
    {
      cout << "test_stage must be 0 (for unique SA), 1 or 2" << endl;
      return 0;
    }
  }   //Simulated Annealing algorithm for optimization
  else 
  {
    int cost;
    double time;

    STT_Solution out0(in0, display_OF);
    out0.SetPrintSolutionOneLine(j2rmode);
    SolverResult<STT_Input, STT_Solution> result0(out0);

    if (method.IsSet() && method == string("ESA-0")) //ESA-0 = Esamodal Simulated Annealing - Single Stage
    {
      STT_ESA_0.SetParameter("start_temperature", static_cast<double>(start_temperature_0));
      STT_ESA_0.SetParameter("expected_min_temperature", static_cast<double>(expected_min_temperature_0));
      STT_ESA_0.SetParameter("cooling_rate", static_cast<double>(cooling_rate_0));
      STT_ESA_0.SetParameter("max_evaluations", static_cast<unsigned long int>(max_evaluations_0));
      STT_ESA_0.SetParameter("neighbors_accepted_ratio", static_cast<double>(neighbors_accepted_ratio_0));
      STT_solver_0.SetRunner(STT_ESA_0);

      if(init_state.IsSet())
      {
        ifstream is(static_cast<string>(init_state));
        STT_Solution out_warmstart(in0, display_OF);
        //out_warmstart.SetPrintSolutionOneLine(j2rmode);
        is >> out_warmstart;
        result0 = STT_solver_0.Resolve(out_warmstart); 
      }
      else
      {
        result0 = STT_solver_0.Solve();
      }

      out0 = result0.output;
      time = result0.running_time;
      // per poter confrontare i differenti costi ottenuti da differenti valori di stt_hard_weight e stt_phased_weight,
      // li reimposto entrambi a "DEFAULT_HARD_WEIGHT" prima di restituire a schermo il costo
      // della soluzione trovata
      out0.stt_hard_weight = DEFAULT_HARD_WEIGHT;
      out0.stt_phased_weight = DEFAULT_HARD_WEIGHT;

      cost = out0.CalculateFullCost();

      if (output_file.IsSet())
      {
        string output_file_name = output_file;          
        
        cout << "Cost: " << cost << endl;
        if (output_file_name.find(".xml") != string::npos)
          {
            sm0.PrettyPrintOutput(out0, output_file_name);
          }
        else // file txt
          {
            ofstream os(output_file_name);
            os << out0 << endl;
            os << "Cost: " << cost << endl;
            os << "Time: " << time << "s " << endl;
            os << "Seed: " << Random::GetSeed() << endl;
          }
      }
      else
      {
        cout << "{\"cost\": " << cost
        << ", \"phase_cost\":" << out0.cost_phased
        << ", \"hard_cost\":" << out0.total_cost_components_hard
        << ", \"stage_1 attempts\":" << "0"
        << ", \"CA1\":" << out0.cost_components[CA1]  
        << ", \"CA2\":" << out0.cost_components[CA2]  
        << ", \"CA3\":" << out0.cost_components[CA3]  
        << ", \"CA4\":" << out0.cost_components[CA4]  
        << ", \"GA1\":" << out0.cost_components[GA1]  
        << ", \"BR1\":" << out0.cost_components[BR1]  
        << ", \"BR2\":" << out0.cost_components[BR2]  
        << ", \"FA2\":" << out0.cost_components[FA2]  
        << ", \"SE1\":" << out0.cost_components[SE1]  
        << ", \"time\":" << time
        << ", \"time_stage_1\":" << time
        << ", \"out\": \"\'" << out0 << "\'\""
        << ", \"seed\":" << Random::GetSeed()
        << "}" << endl;
      } 
    }
    else if (method.IsSet() && (method == string("ESA-3S") || method == string("ESA-2S") || method == string("ESA-2S-OH"))) //ESA-2S = 2 Stages with all constraints in phase1, ESA-2S-OH = 2 Stages with only hard constraints in phase 1
    {
      int stage1_attempts = 1;
      double time_stage_1;
      STT_Solution out1(in1, display_OF);
      //out1.SetPrintSolutionOneLine(j2rmode);
      SolverResult<STT_Input, STT_Solution> result1(out1);
      
      unsigned long int evaluations_first_stage = 0;


      if(method == string("ESA-2S")) //in questo caso applichiamo lo stesso calcolo dell'ESA-0
        evaluations_first_stage = max_evaluations_0; 
      if(method == string("ESA-2S-OH") || method == string("ESA-3S"))
        evaluations_first_stage = max_evaluations_1;

    
      STT_ESA_1.SetParameter("start_temperature", static_cast<double>(start_temperature_1));
      STT_ESA_1.SetParameter("expected_min_temperature", static_cast<double>(expected_min_temperature_1));
      STT_ESA_1.SetParameter("cooling_rate", static_cast<double>(cooling_rate_1));
      STT_ESA_1.SetParameter("max_evaluations", static_cast<unsigned long int>(evaluations_first_stage));
      STT_ESA_1.SetParameter("neighbors_accepted_ratio", static_cast<double>(neighbors_accepted_ratio_1));
      STT_solver_1.SetRunner(STT_ESA_1);
      //ESECUZIONE STAGE 1
      if(init_state.IsSet())
      {
        ifstream is(static_cast<string>(init_state));
        STT_Solution out_warmstart(in1, display_OF);
        //out_warmstart.SetPrintSolutionOneLine(j2rmode);
        is >> out_warmstart;
        result1 = STT_solver_1.Resolve(out_warmstart);
      }
      else
      {
        result1 = STT_solver_1.Solve();
      }

      out1 = result1.output;
      time = result1.running_time;
      time_stage_1 = time;

      // visto che in1 non aveva i costi hard, per calcolare lo stage 1 con costo + f.ob.
      // costruisco un input di appoggio

      STT_Input in1_bis(instance, DEFAULT_HARD_WEIGHT, DEFAULT_HARD_WEIGHT, false, mix_phase_during_search, false, exit_at_zero_hard_stage1, {1, 1, 1, 1, 1, 1, 1, 1, 1});
      STT_Solution out1_bis(in1_bis, display_OF);
      //out1_bis.SetPrintSolutionOneLine(j2rmode);
      
      

      ostringstream ostreammio_1_bis;
      ostreammio_1_bis << out1;
      istringstream istreammio_1_bis(ostreammio_1_bis.str());
      istreammio_1_bis >> out1_bis;
      out1_bis.CalculateFullCost();

      
      if(verbose_mode)
      {
        cout << "Found feasible solution at Stage 1" << endl;
        cout << "{\"cost\": " << out1_bis.ReturnTotalCost()
            << ", \"phase_cost\":" << out1_bis.cost_phased
            << ", \"hard_cost\":" << out1_bis.total_cost_components_hard
            << ", \"stage_1 attempts\":" << stage1_attempts
            << ", \"CA1\":" << out1_bis.cost_components[CA1]  
            << ", \"CA2\":" << out1_bis.cost_components[CA2]  
            << ", \"CA3\":" << out1_bis.cost_components[CA3]  
            << ", \"CA4\":" << out1_bis.cost_components[CA4]  
            << ", \"GA1\":" << out1_bis.cost_components[GA1]  
            << ", \"BR1\":" << out1_bis.cost_components[BR1]  
            << ", \"BR2\":" << out1_bis.cost_components[BR2]  
            << ", \"FA2\":" << out1_bis.cost_components[FA2]  
            << ", \"SE1\":" << out1_bis.cost_components[SE1]  
            << ", \"time\":" << time
            << ", \"out\": \"\'" << out1_bis << "\'\""
            << ", \"seed\":" << Random::GetSeed()
            << "}" << endl;

        cout << "#################################################" << endl
            << "I start Phase 2 (or Phase1_2 if method is ESA-3S)" << endl 
            << "##################################################" << endl;
      }
      
      STT_Solution out1_2(in1_2, display_OF);
      //out1_2.SetPrintSolutionOneLine(j2rmode);
      ostringstream ostreammio_1_2;
      ostreammio_1_2 << out1;
      istringstream istreammio_1_2(ostreammio_1_2.str());
      STT_Solution out_warmstart_1_2(in1_2, display_OF);
      //out_warmstart_1_2.SetPrintSolutionOneLine(j2rmode);
      istreammio_1_2 >> out_warmstart_1_2;
      SolverResult<STT_Input, STT_Solution> result1_2(out1_2);
      double time_stage_1_2 = 0;
      if(method == string("ESA-3S"))
      {
        STT_ESA_1_2.SetParameter("start_temperature", static_cast<double>(start_temperature_1_2));
        STT_ESA_1_2.SetParameter("expected_min_temperature", static_cast<double>(expected_min_temperature_1_2));
        STT_ESA_1_2.SetParameter("cooling_rate", static_cast<double>(cooling_rate_1_2));
        STT_ESA_1_2.SetParameter("max_evaluations", static_cast<unsigned long int>(max_evaluations_1_2));
        STT_ESA_1_2.SetParameter("neighbors_accepted_ratio", static_cast<double>(neighbors_accepted_ratio_1_2));
        STT_solver_1_2.SetRunner(STT_ESA_1_2);
        result1_2 = STT_solver_1_2.Resolve(out_warmstart_1_2);
        
        out1_2 = result1_2.output;
        time = time + result1_2.running_time;
        time_stage_1_2 = result1_2.running_time;

        out1_2.stt_hard_weight = DEFAULT_HARD_WEIGHT;
        out1_2.stt_phased_weight = DEFAULT_HARD_WEIGHT;
        
        for(unsigned h = 0; h < in1_2.hard_weights.size(); h++)
        {
          in1_2.hard_weights[h] = 1;
        }

        out1_2.CalculateFullCost();
        

        if(verbose_mode)
        {
          cerr << "Found solution at Stage 1_2" << endl;
          cerr << "{\"cost\": " << out1_2.ReturnTotalCost()
              << ", \"phase_cost\":" << out1_2.cost_phased
              << ", \"hard_cost\":" << out1_2.total_cost_components_hard
              << ", \"stage_1 attempts\":" << stage1_attempts
              << ", \"CA1\":" << out1_2.cost_components[CA1]  
              << ", \"CA2\":" << out1_2.cost_components[CA2]  
              << ", \"CA3\":" << out1_2.cost_components[CA3]  
              << ", \"CA4\":" << out1_2.cost_components[CA4]  
              << ", \"GA1\":" << out1_2.cost_components[GA1]  
              << ", \"BR1\":" << out1_2.cost_components[BR1]  
              << ", \"BR2\":" << out1_2.cost_components[BR2]  
              << ", \"FA2\":" << out1_2.cost_components[FA2]  
              << ", \"SE1\":" << out1_2.cost_components[SE1]  
              << ", \"time\":" << time
              << ", \"time_stage_1_2\":" << time_stage_1_2
              << ", \"out\": \"\'" << out1_2 << "\'\""
              << ", \"seed\":" << Random::GetSeed()
              << "}" << endl;

          cerr << "###################" << endl
              << "I start Phase 2" << endl 
              << "###################" << endl;
        }
      }

        
      STT_Solution out2(in2, display_OF);
      //out2.SetPrintSolutionOneLine(j2rmode);
      ostringstream ostreammio;
      if(method == string("ESA-3S")) //se ho usato il 3-stage, per l'ultimo stage leggo out1_2
      {
        if(out1_bis.ReturnTotalCost() < out1_2.ReturnTotalCost())
          ostreammio << out1_bis;
        else
          ostreammio << out1_2;
      }
      else
      {
        ostreammio << out1; //altrimenti leggo out1
      }

      istringstream istreammio(ostreammio.str());
      STT_Solution out_warmstart_2(in2, display_OF);
      //out_warmstart_2.SetPrintSolutionOneLine(j2rmode);
      istreammio >> out_warmstart_2;

      STT_ESA_2.SetParameter("start_temperature", static_cast<double>(start_temperature_2));
      STT_ESA_2.SetParameter("expected_min_temperature", static_cast<double>(expected_min_temperature_2));
      STT_ESA_2.SetParameter("cooling_rate", static_cast<double>(cooling_rate_2));
      STT_ESA_2.SetParameter("max_evaluations", static_cast<unsigned long int>(max_evaluations_2));
      STT_ESA_2.SetParameter("neighbors_accepted_ratio", static_cast<double>(neighbors_accepted_ratio_2));
      STT_solver_2.SetRunner(STT_ESA_2);

      SolverResult<STT_Input, STT_Solution> result2(out2);

      result2 = STT_solver_2.Resolve(out_warmstart_2);
      out2 = result2.output;

      // per poter confrontare i differenti costi ottenuti da differenti valori di stt_hard_weight e stt_phased_weight,
      // li reimposto entrambi a "DEFAULT_HARD_WEIGHT" prima di restituire a schermo il costo
      // della soluzione trovata
      out2.stt_hard_weight = DEFAULT_HARD_WEIGHT;
      out2.stt_phased_weight = DEFAULT_HARD_WEIGHT;
      
      // ripristino anche i costi a 1 per calcolare il costo finale

      for(unsigned h = 0; h < in2.hard_weights.size(); h++)
      {
        in2.hard_weights[h] = 1;
      }
      //rimettere i pesi dei costi hard a 1

      cost = out2.CalculateFullCost();
      time = time + result2.running_time;

      if (output_file.IsSet())
        {
          string output_file_name = output_file;          
          cout << "Cost: " << cost << endl;
          if (output_file_name.find(".xml") != string::npos)
            {
              sm2.PrettyPrintOutput(out2, output_file_name);
            }
          else // file txt
            {
              ofstream os(output_file_name);
              os << out2 << endl;
              os << "Cost: " << cost << endl;
              os << "Time: " << time << "s " << endl;
              os << "Seed: " << Random::GetSeed() << endl;
            }
        }
      else
        {
          out2.SetPrintSolutionOneLine(j2rmode);
          cout << "{\"cost\": " << cost
          << ", \"phase_cost\":" << out2.cost_phased
          << ", \"hard_cost\":" << out2.total_cost_components_hard  
          << ", \"CA1\":" << out2.cost_components[CA1]  
          << ", \"CA2\":" << out2.cost_components[CA2]  
          << ", \"CA3\":" << out2.cost_components[CA3]  
          << ", \"CA4\":" << out2.cost_components[CA4]  
          << ", \"GA1\":" << out2.cost_components[GA1]  
          << ", \"BR1\":" << out2.cost_components[BR1]  
          << ", \"BR2\":" << out2.cost_components[BR2]  
          << ", \"FA2\":" << out2.cost_components[FA2]  
          << ", \"SE1\":" << out2.cost_components[SE1]  
          << ", \"time\":" << time
          << ", \"time_stage_1\":" << time_stage_1
          << ", \"time_stage_1_2\":" << time_stage_1_2
          << ", \"time_stage_2\":" << time - time_stage_1 -time_stage_1_2
          << ", \"cost_stage_1\":" << out1_bis.ReturnTotalCost()
          << ", \"cost_stage_1_2\":" << out1_2.ReturnTotalCost();
          if(print_full_solution || verbose_mode)
            cout << ", \"out\": \"\'" << out2 << "\'\"";
          cout << ", \"seed\":" << Random::GetSeed()
          << "}" << endl;
        } 
    }
    else
    {
      throw invalid_argument(string("Unknown method ") + string(method));
    }
  } 
    return 0;
}

