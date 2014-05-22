/*
 * Copyright (c) 2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Erik Hallnor
 *          Ron Dreslinski
 */

/**
 * @file
 * Definitions of BaseTags.
 */

#include "cpu/smt.hh" //maxThreadsPerCPU
#include "mem/cache/tags/base.hh"
#include "mem/cache/base.hh"
#include "sim/sim_exit.hh"

#include <string> //DPCS
#include <fstream> //DPCS
#include "mem/cache/tags/pcslevel.hh" //DPCS

using namespace std;

extern Stats::Value simFreq; //DPCS

BaseTags::BaseTags(const Params *p)
    : ClockedObject(p), blkSize(p->block_size), size(p->size),
      hitLatency(p->hit_latency)
{
	/* BEGIN DPCS PARAMS */
	mode = p->mode;
	nextVDD = 3;
	currVDD = 3;

	__readVoltageParameterFile(p->voltage_parameter_file); //DPCS: Read the voltage parameter file

	//Index 0 unused
	runtimePCSInfo[3] = inputPCSInfo[100];  //DPCS: defaults
	runtimePCSInfo[2] = inputPCSInfo[100]; 
	runtimePCSInfo[1] = inputPCSInfo[100]; 

	//inform("<DPCS> This cache is using VDD3 = %d mV (nominal), VDD2 = %d mV (>= %0.02f \% non-faulty blocks), VDD1 = %d mV (>= %0.02f \% non-faulty blocks ------- yield-limited? = %d\n", runtimePCSInfo[3].getVDD(), runtimePCSInfo[2].getVDD(), p->vdd2_capacity_level, runtimePCSInfo[1].getVDD(), vdd1_capacity_level, is_yield_limited);
	/* END DPCS PARAMS */
}

/**
 * DPCS
 */
void BaseTags::__readVoltageParameterFile(std::string filename) {
	//DPCS: Open voltage parameter file for this cache
	//I am too lazy to do error checking, so it's YOUR job to make sure
	//the file is correctly formatted! See the dpcs-gem5 README.
	inform("<DPCS> Reading this cache's voltage parameter file: %s\n", filename.c_str());
	ifstream voltageFile;
	voltageFile.open(filename.c_str());
	if (voltageFile.fail()) 
		fatal("<DPCS> Failed to open this cache's voltage parameter file: %s\n", filename.c_str());		

	
	//DPCS: Parse the input voltage parameter file, and store relevant data into our inputPCSInfo array.
	//We don't bother ourselves with fault map data here, so nfb will be left alone.
	int i = 100; //DPCS: Assume input file has no more than 100 possible voltage levels. We don't care what their increments are, as long as they are mV. Also, we assume that highest voltages are input first at high indices.
	std::string element;
	inform("<DPCS> VDD# | Voltage (mV) | BER | Block Error Rate | Cache Leakage Power (mW) | Cache Dynamic Energy/Access (nJ)\n");
	getline(voltageFile,element); //DPCS: throw out header row
	while (!voltageFile.eof() && i > 0) { //DPCS: Element 0 must be unused
		getline(voltageFile,element,',');
		inputPCSInfo[i].setVDD(atoi(element.c_str()));
		getline(voltageFile,element,',');
		inputPCSInfo[i].setBER(atof(element.c_str()));
		getline(voltageFile,element,',');
		inputPCSInfo[i].setBlockErrorRate(atof(element.c_str()));
		getline(voltageFile,element,',');
		inputPCSInfo[i].setStaticPower(atof(element.c_str()));
		getline(voltageFile,element);
		inputPCSInfo[i].setAccessEnergy(atof(element.c_str()));
		inputPCSInfo[i].setValid(true);
		inform("<DPCS> %d\t|\t%d\t|\t%4.3E\t|\t%4.3E\t|%0.3f\t|%0.3f\n", i, inputPCSInfo[i].getVDD(), inputPCSInfo[i].getBER(), inputPCSInfo[i].getBlockErrorRate(), inputPCSInfo[i].getStaticPower(), inputPCSInfo[i].getAccessEnergy());
		i--;
	}
	inform("<DPCS> Finished parsing this cache's voltage parameter file\n");
}

void
BaseTags::setCache(BaseCache *_cache)
{
    cache = _cache;
}

void
BaseTags::regStats()
{
    using namespace Stats;
    replacements
        .init(maxThreadsPerCPU)
        .name(name() + ".replacements")
        .desc("number of replacements")
        .flags(total)
        ;

    tagsInUse
        .name(name() + ".tagsinuse")
        .desc("Cycle average of tags in use")
        ;

    totalRefs
        .name(name() + ".total_refs")
        .desc("Total number of references to valid blocks.")
        ;

    sampledRefs
        .name(name() + ".sampled_refs")
        .desc("Sample count of references to valid blocks.")
        ;

    avgRefs
        .name(name() + ".avg_refs")
        .desc("Average number of references to valid blocks.")
        ;

    avgRefs = totalRefs/sampledRefs;

    warmupCycle
        .name(name() + ".warmup_cycle")
        .desc("Cycle when the warmup percentage was hit.")
        ;

    occupancies
        .init(cache->system->maxMasters())
        .name(name() + ".occ_blocks")
        .desc("Average occupied blocks per requestor")
        .flags(nozero | nonan)
        ;
    for (int i = 0; i < cache->system->maxMasters(); i++) {
        occupancies.subname(i, cache->system->getMasterName(i));
    }

    avgOccs
        .name(name() + ".occ_percent")
        .desc("Average percentage of cache occupancy")
        .flags(nozero | total)
        ;
    for (int i = 0; i < cache->system->maxMasters(); i++) {
        avgOccs.subname(i, cache->system->getMasterName(i));
    }

    avgOccs = occupancies / Stats::constant(numBlocks);

    registerExitCallback(new BaseTagsCallback(this));
	
	numFaultyBlocks_VDD1 //DPCS
        .name(name() + ".numFaultyBlocks_VDD1")
        .desc("Total number of faulty blocks at VDD1 (lowest)")
        ;

	numFaultyBlocks_VDD2 //DPCS
        .name(name() + ".numFaultyBlocks_VDD2")
        .desc("Total number of faulty blocks at VDD2 (mid)")
        ;
	
	numFaultyBlocks_VDD3 //DPCS
        .name(name() + ".numFaultyBlocks_VDD3")
        .desc("Total number of faulty blocks at VDD3 (highest)")
        ;

	blockFaultRate_VDD1 //DPCS
        .name(name() + ".blockFaultRate_VDD1")
        .desc("Proportion of all blocks that are faulty at VDD1")
        ;
	blockFaultRate_VDD1 = numFaultyBlocks_VDD1 / numBlocks;
	
	blockFaultRate_VDD2 //DPCS
        .name(name() + ".blockFaultRate_VDD2")
        .desc("Proportion of all blocks that are faulty at VDD2")
        ;
	blockFaultRate_VDD2 = numFaultyBlocks_VDD2 / numBlocks;
	
	blockFaultRate_VDD3 //DPCS
        .name(name() + ".blockFaultRate_VDD3")
        .desc("Proportion of all blocks that are faulty at VDD3")
        ;
	blockFaultRate_VDD3 = numFaultyBlocks_VDD3 / numBlocks;

	cycles_VDD1 //DPCS
        .name(name() + ".cycles_VDD1")
        .desc("Total number of cycles spent at VDD1")
        ;

	cycles_VDD2 //DPCS
        .name(name() + ".cycles_VDD2")
        .desc("Total number of cycles spent at VDD2")
        ;

	cycles_VDD3 //DPCS
        .name(name() + ".cycles_VDD3")
        .desc("Total number of cycles spent at VDD3")
        ;
	
	transitionsTo_VDD1 //DPCS
        .name(name() + ".transitionsTo_VDD1")
        .desc("Total number of transitions to VDD1")
        ;

	transitionsTo_VDD2 //DPCS
        .name(name() + ".transitionsTo_VDD2")
        .desc("Total number of transitions to VDD2")
        ;

	transitionsTo_VDD3 //DPCS
        .name(name() + ".transitionsTo_VDD3")
        .desc("Total number of transitions to VDD3")
        ;

	avgConsecutiveCycles_VDD1 //DPCS
        .name(name() + ".avgConsecutiveCycles_VDD1")
        .desc("Average number of consecutive cycles at VDD1 after transitioning to it")
        ;
	avgConsecutiveCycles_VDD1 = cycles_VDD1 / transitionsTo_VDD1;

	avgConsecutiveCycles_VDD2 //DPCS
        .name(name() + ".avgConsecutiveCycles_VDD2")
        .desc("Average number of consecutive cycles at VDD2 after transitioning to it")
        ;
	avgConsecutiveCycles_VDD2 = cycles_VDD2 / transitionsTo_VDD2;

	avgConsecutiveCycles_VDD3 //DPCS
        .name(name() + ".avgConsecutiveCycles_VDD3")
        .desc("Average number of consecutive cycles at VDD3 after transitioning to it")
        ;
	avgConsecutiveCycles_VDD3 = cycles_VDD3 / transitionsTo_VDD3;
	
	proportionExecTime_VDD1 //DPCS
        .name(name() + ".proportionExecTime_VDD1")
        .desc("Proportion of total execution time that was spent at VDD1 for this cache")
        ;
	proportionExecTime_VDD1 = cycles_VDD1 / (cycles_VDD1 + cycles_VDD2 + cycles_VDD3);
	
	proportionExecTime_VDD2 //DPCS
        .name(name() + ".proportionExecTime_VDD2")
        .desc("Proportion of total execution time that was spent at VDD2 for this cache")
        ;
	proportionExecTime_VDD2 = cycles_VDD2 / (cycles_VDD1 + cycles_VDD2 + cycles_VDD3);

	proportionExecTime_VDD3 //DPCS
        .name(name() + ".proportionExecTime_VDD3")
        .desc("Proportion of total execution time that was spent at VDD3 for this cache")
        ;
	proportionExecTime_VDD3 = cycles_VDD3 / (cycles_VDD1 + cycles_VDD2 + cycles_VDD3);
	
	numUnchangedNotFaultyTo_VDD1 //DPCS
        .name(name() + ".numUnchangedNotFaultyTo_VDD1")
        .desc("Total number of non-faulty blocks unchanged during DPCS transitions to VDD1")
        ;
	
	numUnchangedNotFaultyTo_VDD2 //DPCS
        .name(name() + ".numUnchangedNotFaultyTo_VDD2")
        .desc("Total number of non-faulty blocks unchanged during DPCS transitions to VDD2")
        ;
	
	numUnchangedNotFaultyTo_VDD3 //DPCS
        .name(name() + ".numUnchangedNotFaultyTo_VDD3")
        .desc("Total number of non-faulty blocks unchanged during DPCS transitions to VDD3")
        ;

	numUnchangedFaultyTo_VDD1 //DPCS
        .name(name() + ".numUnchangedFaultyTo_VDD1")
        .desc("Total number of faulty blocks unchanged during DPCS transitions to VDD1")
        ;
	
	numUnchangedFaultyTo_VDD2 //DPCS
        .name(name() + ".numUnchangedFaultyTo_VDD2")
        .desc("Total number of faulty blocks unchanged during DPCS transitions to VDD2")
        ;
	
	numUnchangedFaultyTo_VDD3 //DPCS
        .name(name() + ".numUnchangedFaultyTo_VDD3")
        .desc("Total number of faulty blocks unchanged during DPCS transitions to VDD3")
        ;
	
	numInvalidateOnlyTo_VDD1 //DPCS
        .name(name() + ".numInvalidateOnlyTo_VDD1")
        .desc("Total number of newly faulty blocks only invalidated during DPCS transitions to VDD1")
        ;
	
	numInvalidateOnlyTo_VDD2 //DPCS
        .name(name() + ".numInvalidateOnlyTo_VDD2")
        .desc("Total number of newly faulty blocks only invalidated during DPCS transitions to VDD2")
        ;
	
	numInvalidateOnlyTo_VDD3 //DPCS
        .name(name() + ".numInvalidateOnlyTo_VDD3")
        .desc("Total number of newly faulty blocks only invalidated during DPCS transitions to VDD3")
        ;

	numFaultyWriteBacksTo_VDD1 //DPCS
        .name(name() + ".numFaultyWriteBacksTo_VDD1")
        .desc("Total number of newly faulty blocks invalidated and written back during DPCS transitions to VDD1")
        ;
	
	numFaultyWriteBacksTo_VDD2 //DPCS
        .name(name() + ".numFaultyWriteBacksTo_VDD2")
        .desc("Total number of newly faulty blocks invalidated and written back during DPCS transitions to VDD2")
        ;
	
	numFaultyWriteBacksTo_VDD3 //DPCS
        .name(name() + ".numFaultyWriteBacksTo_VDD3")
        .desc("Total number of newly faulty blocks invalidated and written back during DPCS transitions to VDD3")
        ;

	numMadeAvailableTo_VDD1 //DPCS
        .name(name() + ".numMadeAvailableTo_VDD1")
        .desc("Total number of newly non-faulty blocks made available during DPCS transitions to VDD1")
        ;
	
	numMadeAvailableTo_VDD2 //DPCS
        .name(name() + ".numMadeAvailableTo_VDD2")
        .desc("Total number of newly non-faulty blocks made available during DPCS transitions to VDD2")
        ;
	
	numMadeAvailableTo_VDD3 //DPCS
        .name(name() + ".numMadeAvailableTo_VDD3")
        .desc("Total number of newly non-faulty blocks made available during DPCS transitions to VDD3")
        ;

	faultyWriteBackRateTo_VDD1 //DPCS
        .name(name() + ".faultyWriteBackRateTo_VDD1")
        .desc("Average number of blocks written back during DPCS transitions to VDD1")
        ;
	faultyWriteBackRateTo_VDD1 = numFaultyWriteBacksTo_VDD1 / transitionsTo_VDD1;

	faultyWriteBackRateTo_VDD2 //DPCS
        .name(name() + ".faultyWriteBackRateTo_VDD2")
        .desc("Average number of blocks written back during DPCS transitions to VDD2")
        ;
	faultyWriteBackRateTo_VDD2 = numFaultyWriteBacksTo_VDD2 / transitionsTo_VDD2;
	
	faultyWriteBackRateTo_VDD3 //DPCS
        .name(name() + ".faultyWriteBackRateTo_VDD3")
        .desc("Average number of blocks written back during DPCS transitions to VDD3")
        ;
	faultyWriteBackRateTo_VDD3 = numFaultyWriteBacksTo_VDD3 / transitionsTo_VDD3;
	
	accessEnergy_VDD3 //DPCS
        .name(name() + ".accessEnergy_VDD3")
        .desc("Total dynamic energy dissipated at VDD3 in nJ")
        ;
	
	accessEnergy_VDD2 //DPCS
        .name(name() + ".accessEnergy_VDD2")
        .desc("Total dynamic energy dissipated at VDD2 in nJ")
        ;
	
	accessEnergy_VDD1 //DPCS
        .name(name() + ".accessEnergy_VDD1")
        .desc("Total dynamic energy dissipated at VDD1 in nJ")
        ;

	accessEnergy_tot //DPCS
		.name(name() + ".accessEnergy_tot")
		.desc("Total dynamic energy dissipated in nJ")
		;
	accessEnergy_tot = accessEnergy_VDD3 + accessEnergy_VDD2 + accessEnergy_VDD1;

	staticEnergy_VDD3 //DPCS
        .name(name() + ".staticEnergy_VDD3")
        .desc("Total static energy dissipated at VDD3 in nJ")
        ;
	staticEnergy_VDD3 = cycles_VDD3 * clockPeriod() / simFreq * runtimePCSInfo[3].getStaticPower(); //DPCS
	
	staticEnergy_VDD2 //DPCS
        .name(name() + ".staticEnergy_VDD2")
        .desc("Total static energy dissipated at VDD2 in nJ")
        ;
	staticEnergy_VDD2 = cycles_VDD2 * clockPeriod() / simFreq * runtimePCSInfo[2].getStaticPower(); //DPCS
	
	staticEnergy_VDD1 //DPCS
        .name(name() + ".staticEnergy_VDD1")
        .desc("Total static energy dissipated at VDD1 in nJ")
        ;
	staticEnergy_VDD1 = cycles_VDD1 * clockPeriod() / simFreq * runtimePCSInfo[1].getStaticPower(); //DPCS

	staticEnergy_tot //DPCS
		.name(name() + ".staticEnergy_tot")
		.desc("Total static energy dissipated in nJ")
		;
	staticEnergy_tot = staticEnergy_VDD3 + staticEnergy_VDD2 + staticEnergy_VDD1;

	staticPower_avg
		.name(name() + ".staticPower_avg")
		.desc("Average static power of this cache over the entire execution")
		;
	staticPower_avg = proportionExecTime_VDD1 * runtimePCSInfo[1].getStaticPower() + proportionExecTime_VDD2 * runtimePCSInfo[2].getStaticPower() + proportionExecTime_VDD3 * runtimePCSInfo[3].getStaticPower(); //DPCS
}
