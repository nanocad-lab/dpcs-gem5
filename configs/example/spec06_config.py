# Copyright (c) 2012-2013 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2006-2008 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Steve Reinhardt

# Simple test script
#
# "m5 test.py"

# Author: Mark Gottscho
# mgottscho@ucla.edu

import optparse
import sys
import os

import m5
from m5.defines import buildEnv
from m5.objects import *
from m5.util import addToPath, fatal

addToPath('./configs')
addToPath('../common')
addToPath('../ruby')
addToPath('../topologies')

import Options
import Ruby
import Simulation
import CacheConfig
import MemConfig

import spec06_benchmarks 

from Caches import *
from cpu2000 import *

#def get_processes(options):
#    """Interprets provided options and returns a list of processes"""
#
#    multiprocesses = []
#    inputs = []
#    outputs = []
#    errouts = []
#    pargs = []
#
#    workloads = options.cmd.split(';')
#    if options.input != "":
#        inputs = options.input.split(';')
#    if options.output != "":
#        outputs = options.output.split(';')
#    if options.errout != "":
#        errouts = options.errout.split(';')
#    if options.options != "":
#        pargs = options.options.split(';')
#
#    idx = 0
#    for wrkld in workloads:
#        process = LiveProcess()
#        process.executable = wrkld
#
#        if len(pargs) > idx:
#            process.cmd = [wrkld] + pargs[idx].split()
#        else:
#            process.cmd = [wrkld]
#
#        if len(inputs) > idx:
#            process.input = inputs[idx]
#        if len(outputs) > idx:
#            process.output = outputs[idx]
#        if len(errouts) > idx:
#            process.errout = errouts[idx]
#
#        multiprocesses.append(process)
#        idx += 1
#
#    if options.smt:
#        assert(options.cpu_type == "detailed" or options.cpu_type == "inorder")
#        return multiprocesses, idx
#    else:
#        return multiprocesses, 1


# Begin options decoding


config_path = os.path.dirname(os.path.abspath(__file__))
#print "config_path: " + config_path
config_root = os.path.dirname(config_path)
#print "config_root: " + config_root
gem5_root = os.path.dirname(config_root)
#print "gem5_root: " + gem5_root

#print "Parsing options..."
parser = optparse.OptionParser()
Options.addCommonOptions(parser)
Options.addSEOptions(parser)

# DPCS: My SPEC2006 benchmark options
parser.add_option("-b", "--benchmark", type="string", default="", help="The SPEC benchmark to be loaded.")
parser.add_option("--benchmark_stdout", type="string", default="", help="Absolute path for stdout redirection for the benchmark.")
parser.add_option("--benchmark_stderr", type="string", default="", help="Absolute path for stderr redirection for the benchmark.")
parser.add_option("--l1_cache_mode", type="string", default="vanilla", help="vanilla, static, or dynamic")
parser.add_option("--l2_cache_mode", type="string", default="vanilla", help="vanilla, static, or dynamic")
parser.add_option("--l1_hit_latency", type="int", default=2, help="Hit latency for L1 (I and D)")
parser.add_option("--l2_hit_latency", type="int", default=20, help="Hit latency for L2")
parser.add_option("--l1_fault_map_file", type="string", default="", help="L1 cache path to file containing fault map file")
parser.add_option("--l2_fault_map_file", type="string", default="", help="L2 cache path to file containing fault map file")
parser.add_option("--l1_runtime_vdd_select_file", type="string", default="", help="L1 cache path to file containing selected runtime VDD levels")
parser.add_option("--l2_runtime_vdd_select_file", type="string", default="", help="L2 cache path to file containing selected runtime VDD levels")
parser.add_option("--l1i_cache_trace_file", type="string", default="", help="L1I cache path to file to dump cache trace info as CSV")
parser.add_option("--l1d_cache_trace_file", type="string", default="", help="L1D cache path to file to dump cache trace info as CSV")
parser.add_option("--l2_cache_trace_file", type="string", default="", help="L2 cache path to file to dump cache trace info as CSV")
parser.add_option("--dpcs_l1_vdd_switch_overhead", type="long", default=100, help="Overhead in cycles of changing DPCS L1 cache VDD value")
parser.add_option("--dpcs_l2_vdd_switch_overhead", type="long", default=400, help="Overhead in cycles of changing DPCS L2 cache VDD value")
parser.add_option("--dpcs_l1_sample_interval", type="long", default=50000, help="Interval in # of CPU cycles for measuring miss rate in DPCS caches")
parser.add_option("--dpcs_l2_sample_interval", type="long", default=500000, help="Interval in # of CPU cycles for measuring miss rate in DPCS caches")
parser.add_option("--dpcs_l1_threshold_low", type="float", default=0.15, help="Policy threshold low for DPCS L1")
parser.add_option("--dpcs_l1_threshold_high", type="float", default=0.25, help="Policy threshold high for DPCS L1")
parser.add_option("--dpcs_l2_threshold_low", type="float", default=0.15, help="Policy threshold low for DPCS L2")
parser.add_option("--dpcs_l2_threshold_high", type="float", default=0.25, help="Policy threshold high for DPCS L2")

#parser.add_option("-k", "--chkpt", default="", help="The checkpoint to load.")

if '--ruby' in sys.argv:
	Ruby.define_options(parser)

#execfile(os.path.join(config_root, "common", "Options.py"))
(options, args) = parser.parse_args()

if args:
	print "Error: script doesn't take any positional arguments"
	sys.exit(1)

#if options.bench:
#    apps = options.bench.split("-")
#    if len(apps) != options.num_cpus:
#        print "number of benchmarks not equal to set num_cpus!"
#        sys.exit(1)
#
#    for app in apps:
#        try:
#            if buildEnv['TARGET_ISA'] == 'alpha':
#                exec("workload = %s('alpha', 'tru64', 'ref')" % app)
#            else:
#                exec("workload = %s(buildEnv['TARGET_ISA'], 'linux', 'ref')" % app)
#            multiprocesses.append(workload.makeLiveProcess())
#        except:
#            print >>sys.stderr, "Unable to find workload for %s: %s" % (buildEnv['TARGET_ISA'], app)
#            sys.exit(1)
#elif options.cmd:
#    multiprocesses, numThreads = get_processes(options)
#elif options.benchmark:
if options.benchmark:
	print 'Selected SPEC_CPU2006 benchmark'
	if options.benchmark == 'perlbench':
		print '--> perlbench'
		process = spec06_benchmarks.perlbench
	elif options.benchmark == 'bzip2':
		print '--> bzip2'
		process = spec06_benchmarks.bzip2
	elif options.benchmark == 'gcc':
		print '--> gcc'
		process = spec06_benchmarks.gcc
	elif options.benchmark == 'bwaves':
		print '--> bwaves' 
		process = spec06_benchmarks.bwaves
	elif options.benchmark == 'gamess':
		print '--> gamess' 
		process = spec06_benchmarks.gamess
	elif options.benchmark == 'mcf':
		print '--> mcf' 
		process = spec06_benchmarks.mcf
	elif options.benchmark == 'milc':
		print '--> milc' 
		process = spec06_benchmarks.milc
	elif options.benchmark == 'zeusmp':
		print '--> zeusmp' 
		process = spec06_benchmarks.zeusmp
	elif options.benchmark == 'gromacs':
		print '--> gromacs' 
		process = spec06_benchmarks.gromacs
	elif options.benchmark == 'cactusADM':
		print '--> cactusADM' 
		process = spec06_benchmarks.cactusADM
	elif options.benchmark == 'leslie3d':
		print '--> leslie3d' 
		process = spec06_benchmarks.leslie3d
	elif options.benchmark == 'namd':
		print '--> namd' 
		process = spec06_benchmarks.namd
	elif options.benchmark == 'gobmk':
		print '--> gobmk' 
		process = spec06_benchmarks.gobmk;
	elif options.benchmark == 'dealII':
		print '--> dealII' 
		process = spec06_benchmarks.dealII
	elif options.benchmark == 'soplex':
		print '--> soplex' 
		process = spec06_benchmarks.soplex
	elif options.benchmark == 'povray':
		print '--> povray' 
		process = spec06_benchmarks.povray
	elif options.benchmark == 'calculix':
		print '--> calculix' 
		process = spec06_benchmarks.calculix
	elif options.benchmark == 'hmmer':
		print '--> hmmer' 
		process = spec06_benchmarks.hmmer
	elif options.benchmark == 'sjeng':
		print '--> sjeng' 
		process = spec06_benchmarks.sjeng
	elif options.benchmark == 'GemsFDTD':
		print '--> GemsFDTD' 
		process = spec06_benchmarks.GemsFDTD
	elif options.benchmark == 'libquantum':
		print '--> libquantum' 
		process = spec06_benchmarks.libquantum
	elif options.benchmark == 'h264ref':
		print '--> h264ref' 
		process = spec06_benchmarks.h264ref
	elif options.benchmark == 'tonto':
		print '--> tonto'
		process = spec06_benchmarks.tonto
	elif options.benchmark == 'lbm':
		print '--> lbm'
		process = spec06_benchmarks.lbm
	elif options.benchmark == 'omnetpp':
		print '--> omnetpp'
		process = spec06_benchmarks.omnetpp
	elif options.benchmark == 'astar':
		print '--> astar'
		process = spec06_benchmarks.astar
	elif options.benchmark == 'wrf':
		print '--> wrf'
		process = spec06_benchmarks.wrf
	elif options.benchmark == 'sphinx3':
		print '--> sphinx3'
		process = spec06_benchmarks.sphinx3
	elif options.benchmark == 'xalancbmk':
		print '--> xalancbmk'
		process = spec06_benchmarks.xalancbmk
	elif options.benchmark == 'specrand_i':
		print '--> specrand_i'
		process = spec06_benchmarks.specrand_i
	elif options.benchmark == 'specrand_f':
		print '--> specrand_f'
		process = spec06_benchmarks.specrand_f
	else:
		print "No recognized SPEC2006 benchmark selected! Exiting."
		sys.exit(1)
else:
    print >> sys.stderr, "Need --benchmark switch to specify SPEC CPU2006 workload. Exiting!\n"
    sys.exit(1)

#if options.chkpt != "":
#	process.chkpt = options.chkpt

# Set process stdout/stderr
if options.benchmark_stdout:
	process.output = options.benchmark_stdout
	print "Process stdout file: " + process.output
if options.benchmark_stderr:
	process.errout = options.benchmark_stderr
	print "Process stderr file: " + process.errout

#multiprocesses = []
numThreads = 1
(CPUClass, test_mem_mode, FutureClass) = Simulation.setCPUClass(options)
CPUClass.numThreads = numThreads
MemClass = Simulation.setMemClass(options)

# Check -- do not allow SMT with multiple CPUs
if options.smt and options.num_cpus > 1:
    fatal("You cannot use SMT with multiple CPUs!")

np = options.num_cpus
system = System(cpu = [CPUClass(cpu_id=i) for i in xrange(np)],
                mem_mode = test_mem_mode,
                mem_ranges = [AddrRange(options.mem_size)],
                cache_line_size = options.cacheline_size)

# Create a top-level voltage domain
system.voltage_domain = VoltageDomain(voltage = options.sys_voltage)

# Create a source clock for the system and set the clock period
system.clk_domain = SrcClockDomain(clock =  options.sys_clock,
                                   voltage_domain = system.voltage_domain)

# Create a CPU voltage domain (is this derived from top-level voltage?)
system.cpu_voltage_domain = VoltageDomain()

# Create a separate clock domain for the CPUs
system.cpu_clk_domain = SrcClockDomain(clock = options.cpu_clock,
                                       voltage_domain =
                                       system.cpu_voltage_domain)

# All cpus belong to a common cpu_clk_domain, therefore running at a common
# frequency.
for cpu in system.cpu:
    cpu.clk_domain = system.cpu_clk_domain

# Sanity check
if options.fastmem:
    if CPUClass != AtomicSimpleCPU:
        fatal("Fastmem can only be used with atomic CPU!")
    if (options.caches or options.l2cache):
        fatal("You cannot use fastmem in combination with caches!")

if options.simpoint_profile:
    if not options.fastmem:
        # Atomic CPU checked with fastmem option already
        fatal("SimPoint generation should be done with atomic cpu and fastmem")
    if np > 1:
        fatal("SimPoint generation not supported with more than one CPUs")

#for i in xrange(np):
#	if options.benchmark:
#		system.cpu[i].workload = process
#		print process.cmd
#
#	elif options.smt:
#		system.cpu[i].workload = multiprocesses
#	elif len(multiprocesses) == 1:
#		system.cpu[i].workload = multiprocesses[0]
#	else:
#		system.cpu[i].workload = multiprocesses[i]
#
#	if options.fastmem:
#		system.cpu[i].fastmem = True
#
#	if options.simpoint_profile:
#		system.cpu[i].simpoint_profile = True
#		system.cpu[i].simpoint_interval = options.simpoint_interval
#
#	if options.checker:
#		system.cpu[i].addCheckerCpu()
#
#	system.cpu[i].createThreads()


# Assign workloads to processors
for i in xrange(np):
	system.cpu[i].workload = process
	print process.cmd

# Ruby option handling
if options.ruby:
	if not (options.cpu_type == "detailed" or options.cpu_type == "timing"):
		print >> sys.stderr, "Ruby requires TimingSimpleCPU or O3CPU!"
		sys.exit(1)

   # Set the option for physmem so that it is not allocated any space
	system.physmem = MemClass(range=AddrRange(options.mem_size), null = True)
	options.use_map = True
	Ruby.create_system(options, system)
	assert(options.num_cpus == len(system.ruby._cpu_ruby_ports)) # Sanity check

	# Map ports for each CPU
	for i in xrange(np):
		ruby_port = system.ruby._cpu_ruby_ports[i]

	# Create the interrupt controller and connect its ports to Ruby
	# Note that the interrupt controller is always present but only
	# in x86 does it have message ports that need to be connected
		system.cpu[i].createInterruptController()

	# Connect the cpu's cache ports to Ruby
		system.cpu[i].icache_port = ruby_port.slave
		system.cpu[i].dcache_port = ruby_port.slave
	# if buildEnv['TARGET_ISA'] == 'x86': #N/A
	#     system.cpu[i].interrupts.pio = ruby_port.master
	#     system.cpu[i].interrupts.int_master = ruby_port.slave
	#     system.cpu[i].interrupts.int_slave = ruby_port.master
	#     system.cpu[i].itb.walker.port = ruby_port.slave
	#     system.cpu[i].dtb.walker.port = ruby_port.slave
else: # Regular gem5 memory (not Ruby)
    system.membus = CoherentBus()
    system.system_port = system.membus.slave
    CacheConfig.config_cache(options, system)
    MemConfig.config_mem(options, system)

# Create the root system hierarchy
root = Root(full_system = False, system = system)

# Run the simulation!
Simulation.run(options, root, system, FutureClass)
