/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
Copyright (c) 2013 Jinglei Ren <jinglei.ren@gmail.com>
 
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

#include <stdio.h>
#include <atomic>
#include "pin.H"
#include "mem_addr_trace.h"

static MemAddrTrace * mem_trace;

static std::atomic_uint g_ins_count;
static TLS_KEY tls_ins_count;

#define TLS_INS_COUNT(tid) \
    (static_cast<UINT32*>(PIN_GetThreadData(tls_ins_count, tid)))

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    UINT32* tdata = new UINT32;
    PIN_SetThreadData(tls_ins_count, tdata, tid);
}

VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
    delete TLS_INS_COUNT(tid);
}

VOID InsCount(THREADID tid)
{
    *TLS_INS_COUNT(tid) = ++g_ins_count;
}

VOID RecordMemRead(THREADID tid, VOID * addr)
{
    mem_trace->Input(*TLS_INS_COUNT(tid), addr, 'R');
}

VOID RecordMemWrite(THREADID tid, VOID * addr)
{
    mem_trace->Input(*TLS_INS_COUNT(tid), addr, 'W');
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

VOID Fini(INT32 code, VOID *v)
{
    mem_trace->Flush();
    delete mem_trace;
}

/* Added command line option: buffer size */
KNOB<UINT32> KnobBufferSize(KNOB_MODE_WRITEONCE, "pintool",
    "buffer_size", "1048576", "specify the number of records to buffer");

KNOB<string> KnobTraceFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "mem_addr.trace", "specify output file name");

KNOB<UINT32> KnobFileSize(KNOB_MODE_WRITEONCE, "pintool",
    "file_size", "16384", "specify the max file size in MB");

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

    UINT32 limit = KnobFileSize.Value();
    mem_trace = new MemAddrTrace(KnobBufferSize.Value(),
        KnobTraceFile.Value().c_str(), limit);

    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

