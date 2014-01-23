/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
Copyright (c) 2013 Jinglei Ren <jinglei.ren@stanzax.org>
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 *  Create Date: 12/20/2013
 */

#include <cstdio>
#include <string>
#include <atomic>
#include "pin.H"
#include "pinplay.H"
#include "mem_addr_trace.h"

#define CACHE_LINE_SIZE 64 // bytes
#define MEGA 1000000

static PIN_LOCK g_lock;
static MemAddrTrace * g_mem_trace;
static std::atomic_uint_fast64_t g_ins_count;
static UINT64 g_ins_skip;
static UINT64 g_ins_max;
static volatile bool g_switch;

/* Added command line option: buffer size */
KNOB<UINT32> KnobBufferLength(KNOB_MODE_WRITEONCE, "pintool",
    "buffer_length", "1048576", "specify the number of records to buffer");

KNOB<string> KnobFilePrefix(KNOB_MODE_WRITEONCE, "pintool",
    "file_prefix", "mem_addr", "specify prefix of output file name");

KNOB<UINT32> KnobFileSize(KNOB_MODE_WRITEONCE, "pintool",
    "file_size", "8192", "specify the max file size in MiB");

KNOB<UINT64> KnobInsSkip(KNOB_MODE_WRITEONCE, "pintool",
    "ins_skip", "0", "specify the number of mega-instructions to skip");

KNOB<UINT64> KnobInsMax(KNOB_MODE_WRITEONCE, "pintool",
    "ins_max", "1000000", "specify the max number of mega-instructions");

PINPLAY_ENGINE pinplay_engine;
KNOB<BOOL> KnobPinPlayLogger(KNOB_MODE_WRITEONCE, "pintool",
    "log", "0", "Activate the pinplay logger");

KNOB<BOOL> KnobPinPlayReplayer(KNOB_MODE_WRITEONCE, "pintool",
    "replay", "0", "Activate the pinplay replayer");

VOID InitGlobal()
{
    PIN_InitLock(&g_lock);

    std::string file_name(KnobFilePrefix.Value());
    file_name.append("_").append(std::to_string(PIN_GetPid())).append(".trace");
    g_mem_trace = new MemAddrTrace(KnobBufferLength.Value(),
        file_name.c_str(), KnobFileSize.Value());

    g_ins_count = 0;
    g_ins_skip = KnobInsSkip.Value() * MEGA;
    g_ins_max = KnobInsMax.Value() * MEGA + (MEGA / 10);
    g_switch = false;
}

VOID InsCount(THREADID tid)
{
    UINT64 ins_count = ++g_ins_count; // starts from 1
    g_switch = (g_ins_skip < ins_count) && (ins_count < g_ins_max);
}

VOID RecordMemRead(THREADID tid, VOID * addr)
{
    if (!g_switch) return;
    PIN_GetLock(&g_lock, tid);
    if (!g_mem_trace->Input(g_ins_count, addr, 'R')) {
        PIN_Detach();
    }
#ifdef TEST
    std::cout << g_ins_count << '\t' << addr << "\tR" << std::endl;
#endif
    PIN_ReleaseLock(&g_lock);
}

VOID RecordMemWrite(THREADID tid, VOID * addr)
{
    if (!g_switch) return;
    PIN_GetLock(&g_lock, tid);
    if (!g_mem_trace->Input(g_ins_count, addr, 'W')) {
        PIN_Detach();
    }
#ifdef TEST
    std::cout << g_ins_count << '\t' << addr << "\tW" << std::endl;
#endif
    PIN_ReleaseLock(&g_lock);
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    INS_InsertCall(
        ins, IPOINT_BEFORE, (AFUNPTR)InsCount, IARG_THREAD_ID,
        IARG_CALL_ORDER, CALL_ORDER_FIRST,
        IARG_END);

    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}

VOID Detach(VOID *v)
{
    PIN_GetLock(&g_lock, 0);
    g_mem_trace->Flush();
    PIN_ReleaseLock(&g_lock); 
    delete g_mem_trace;
}

VOID Fini(INT32 code, VOID *v)
{
    Detach(v);
}

VOID BeforeFork(THREADID tid, const CONTEXT* ctxt, VOID * arg)
{
    PIN_GetLock(&g_lock, tid);
    g_mem_trace->Flush();
    PIN_ReleaseLock(&g_lock);
}

VOID AfterForkInChild(THREADID threadid, const CONTEXT* ctxt, VOID * arg)
{
    delete g_mem_trace;
    InitGlobal();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();
    pinplay_engine.Activate(argc, argv, KnobPinPlayLogger, KnobPinPlayReplayer);

    InitGlobal();

    PIN_AddForkFunction(FPOINT_BEFORE, BeforeFork, 0);
    PIN_AddForkFunction(FPOINT_AFTER_IN_CHILD, AfterForkInChild, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_AddDetachFunction(Detach, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

