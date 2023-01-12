# sa4stt
### Simulated Annealing for Sports Timetabling ITC2021 formulation

Rosati, R.M., Petris, M., Di Gaspero, L., and Schaerf, A. Multi-neighborhood simulated annealing for the sports timetabling competition ITC2021. J Sched 25, 301–319 (2022). https://doi.org/10.1007/s10951-022-00740-y

-------------

Our code is based in C++. It requires library libboost, minimum version=1.71. On Linux you can install the package libboost1.71-dev installed. It also requires cmake for compiling.

#### How to compile sa4stt
To compile it, from the main folder:

```bash
cmake -DCMAKE_BUILD_TYPE=Release .
make
```
#### How to run sa4stt
The executable will be in the bin folder. To call it, from the main folder itc2021, you can run:

```bash
./bin/stt [parameters...]
```
The algorithm is a three-stages Simulated Annealing (SA) that takes several parameters in input (details on the tuning procedure are provided in our paper). To make things easy for the user, the best parameters resulting from the tuning procedure are encoded as default ones. For example you can call the solver on instance Late 15 in this way:

```bash
./bin/stt --main::instance instances/all/all/ITC2021_Late_15.xml --main::method ESA-3S --main::use_hcp-enable
```

This will, however, encode also the number of evaluations. If you want to define them by yourself, you'll need to define them for each of the three stages (named as STAGE1, STAGE1_2, STAGE2). You'll have to define the evaluations for all the three stages, otherwise the solver will consider the default values. You can transform the 3-stages algorithm into a 2-stages or 1-stage SA by setting to 0 the corresponding number of evaluations of the stage(s) you want to skip. Please avoid using very low values for the number of evaluations (<10000), as this can create inconsistencies in the cooling scheme. 

```bash
./bin/stt --main::instance instances/all/all/ITC2021_Late_15.xml --main::method ESA-3S --main::use_hcp-enable --STAGE1::max_evaluations 100000 --STAGE1_2::max_evaluations 100000 --STAGE2::max_evaluations 10000
```

The output will be json that will be printed on the standard output. If you don't want the solution to be printed, you can add `--main::print_full_solution-disable`. Example:

```bash
./bin/stt --main::instance instances/all/all/ITC2021_Late_15.xml --main::method ESA-3S --main::use_hcp-enable --STAGE1::max_evaluations 100000 --STAGE1_2::max_evaluations 100000 --STAGE2::max_evaluations 10000 --main::print_full_solution-disable
```

If you want to see the solution, but printed on one line, you can use `--main::j2rmode-enable`:

```bash
./bin/stt --main::instance instances/all/all/ITC2021_Late_15.xml --main::method ESA-3S --main::use_hcp-enable --STAGE1::max_evaluations 100000 --STAGE1_2::max_evaluations 100000 --STAGE2::max_evaluations 10000 --main::j2rmode-enable
```

You can of course also pass all the parameters for each of the three stages of the Simulated Annealing by command line, to do so you will not have to use `--main::use_hcp-enable`. Example:

```bash
./bin/stt --main::instance instances/all/all/ITC2021_Late_15.xml --main::method ESA-3S  --HW::CA1 7 --HW::CA2 8 --HW::CA3 2 --HW::CA4 8 --HW::GA1 10 --HW::BR1 1 --HW::BR2 6 --HW::FA2 1 --HW::SE1 1  --STAGE1_2::start_temperature 100  --STAGE1_2::cooling_rate 0.99 --STAGE1_2::expected_min_temperature 1 --STAGE1_2::neighbors_accepted_ratio 0.1  --STAGE2::start_temperature 100 --STAGE2::cooling_rate 0.99 --STAGE2::expected_min_temperature 1 --STAGE2::neighbors_accepted_ratio 0.1  --main::hard_weight 10 --main::phased_weight 117 --STAGE1::start_temperature 179  --STAGE1::cooling_rate 0.99 --STAGE1::expected_min_temperature 2.1 --STAGE1::neighbors_accepted_ratio 0.1 --main::j2rmode-enable --main::start_type random --STAGE1::max_evaluations 100000 --STAGE1_2::max_evaluations 100000 --STAGE2::max_evaluations 10000
```
--------------
How to cite:

```latex
@article{sa4stt,
  title={Multi-neighborhood simulated annealing for the sports timetabling competition ITC2021},
  author={Rosati, Roberto Maria and Petris, Matteo and Di Gaspero, Luca and Schaerf, Andrea},
  journal={Journal of Scheduling},
  volume={25},
  number={3},
  pages={301--319},
  year={2022},
  publisher={Springer}
}
```
