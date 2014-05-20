$GEM5_DIR/build/ALPHA/gem5.fast\
 \
 --outdir=$OUTPUT_DIR\
 \
 $GEM5_DIR/configs/example/spec06_config.py\
 \
 --cpu-type=detailed\
 --num-cpus=1\
 --sys-clock="2GHz"\
 --cpu-clock="2GHz"\
 --sys-voltage="1V"\
 \
 --caches\
 --cacheline_size="64"\
 \
 --l1d_size="64kB"\
 --l1i_size="64kB"\
 --l1d_assoc=4\
 --l1i_assoc=4\
 --l1_hit_latency=2\
 --l1_cache_mode=$L1_CACHE_MODE\
 --l1_voltage_parameter_file=$GEM5_L1_CONFIG\
 --l1_fault_map_file=$L1_FAULT_MAP_CSV\
 --l1_runtime_vdd_select_file=$L1_RUNTIME_VDD_CSV\
 --dpcs_l1_sample_interval=100000\
 --dpcs_l1_miss_threshold_low=0.05\
 --dpcs_l1_miss_threshold_high=0.10\
 \
 --l2cache\
 --num-l2caches=1\
 --l2_size="2MB"\
 --l2_assoc=8\
 --l2_hit_latency=4\
 --l2_miss_penalty=200\
 --l2_cache_mode=$L2_CACHE_MODE\
 --l2_voltage_parameter_file=$GEM5_L2_CONFIG\
 --l2_fault_map_file=$L2_FAULT_MAP_CSV\
 --l2_runtime_vdd_select_file=$L2_RUNTIME_VDD_CSV\
 --dpcs_l2_sample_interval=10000\
 --dpcs_l2_miss_threshold_low=0.05\
 --dpcs_l2_miss_threshold_high=0.10\
 \
 --num-l3caches=0\
 \
 --dpcs_super_sample_interval=20\
 --vdd_switch_overhead=20\
 \
 --mem-type=ddr3_1600_x64\
 --mem-channels=1\
 --mem-size="2048MB"\
 \
 --benchmark=$BENCHMARK\
 --benchmark_stdout=$OUTPUT_DIR/$BENCHMARK.out\
 --benchmark_stderr=$OUTPUT_DIR/$BENCHMARK.err\
 \
 --fast-forward=1000000000\
 --maxinsts=2000000000\
 --at-instruction
