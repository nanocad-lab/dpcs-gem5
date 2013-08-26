#build/ARM/gem5.opt configs/example/se.py -c tests/test-progs/hello/bin/arm/linux/hello
#build/X86/gem5.opt configs/example/se.py -c tests/test-progs/hello/bin/x86/linux/hello
#build/X86/gem5.opt configs/example/se.py -c ~/Dropbox/research/projects/sram-fault-mapping/eval-tools/fft-cache/fftcache -o "-i /home/mark/Dropbox/research/projects/sram-fault-mapping/eval-tools/fft-cache/config/L2_cache.cfg -fr /home/mark/Dropbox/research/projects/sram-fault-mapping/eval-tools/fft-cache/config/faultrate.cfg -p /home/mark/Dropbox/research/projects/sram-fault-mapping/eval-tools/fft-cache/config/power.cfg -o . --simulation"

build/X86/gem5.opt configs/example/myconfig.py --cpu-type=AtomicSimpleCPU --num-cpus=1 --sys-clock="3GHz" --cpu-clock="3GHz" --mem-type=SimpleMemory --mem-channels=1 --mem-size="4096MB" --caches --l2cache --num-l2caches=1 --num-l3caches=0 --l1d_size="64kB" --l1i_size="64kB" --l2_size="2MB" --l1d_assoc=4 --l1i_assoc=4 --l2_assoc=8 --cacheline_size="64" --benchmark=perlbench
#build/X86/gem5.opt configs/example/myconfig.py --benchmark=perlbench


#build/X86/gem5.opt configs/example/se.py -c /home/mark/spec_cpu2006_install/benchspec/CPU2006/400.perlbench/run/run_base_test_amd64-m64-gcc42-nn.0000/perlbench_base.amd64-m64-gcc42-nn
