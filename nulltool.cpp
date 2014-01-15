// This null tool is for PinPlay logging

#include "pin.H"
#include "pinplay.H"

PINPLAY_ENGINE pinplay_engine;
KNOB<BOOL> KnobPinPlayLogger(KNOB_MODE_WRITEONCE, "pintool",
        "log", "0", "Activate the pinplay logger");

INT32 Usage()
{
    cerr << "This tool works as a PinPlay logger." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    pinplay_engine.Activate(argc, argv, KnobPinPlayLogger, NULL);

    PIN_StartProgram();
    return 0;
}

