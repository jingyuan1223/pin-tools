#include <stdio.h>
#include "pin.H"
#include <iostream>
#include <fstream>
#include<string>
#include<sstream>
#include<set>
using namespace std;
using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
	"o", "controflow.dot", "specify output file name");


FILE* out;
PIN_LOCK pinLock;

ofstream OutFile;
stringstream ss;
set<string> flow;
string last;


VOID ThreadStart(THREADID threadid, CONTEXT* ctxt, INT32 flags, VOID* v)
{
	PIN_GetLock(&pinLock, threadid + 1);
	
	PIN_ReleaseLock(&pinLock);
}


VOID ThreadFini(THREADID threadid, const CONTEXT* ctxt, INT32 code, VOID* v)
{
	PIN_GetLock(&pinLock, threadid + 1);
	

	string output = "digraph controlflow{\n";
	set<string>::const_iterator pos;
	for (pos = flow.begin(); pos != flow.end(); pos++) {
		output += (*pos + "\n");
	}
	output += "}";
	
	fprintf(out, "%s", output.c_str());
	fflush(out);
	

	PIN_ReleaseLock(&pinLock);
}

VOID drawflow(void* ip) {
	if (!last.empty()) {
		stringstream ss;
		ss << "\"" << last << "\"" << " -> " << "\"" << ip << "\"" << ";";
		string str = ss.str();
		flow.insert(str);
	}
	stringstream ss;
	ss << ip; last = ss.str();
}

VOID Instruction(INS ins, VOID* v)
{
	PIN_LockClient(); IMG image = IMG_FindByAddress(INS_Address(ins)); PIN_UnlockClient();
	if (IMG_Valid(image) && IMG_IsMainExecutable(image)) {
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)drawflow, IARG_INST_PTR, IARG_END);
	}
}


// This routine is executed once at the end.
VOID Fini(INT32 code, VOID* v)
{
	fclose(out);
}


INT32 Usage()
{
	PIN_ERROR("This Pintool prints control flow.\n"
		+ KNOB_BASE::StringKnobSummary() + "\n");
	return -1;
}


int main(INT32 argc, CHAR** argv)
{
	// Initialize the pin lock
	PIN_InitLock(&pinLock);

	// Initialize pin
	if (PIN_Init(argc, argv)) return Usage();
	PIN_InitSymbols();


	out = fopen(KnobOutputFile.Value().c_str(), "w");

	
	INS_AddInstrumentFunction(Instruction, 0);

	// Register Analysis routines to be called when a thread begins/ends
	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	// Never returns
	PIN_StartProgram();

	return 0;
}
