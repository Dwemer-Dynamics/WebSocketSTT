
#include <Windows.h>


#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "Papyrus.h"
#include "VoiceRecordControl.h"


namespace logger = SKSE::log;

extern void StartRecording();

int Papyrus::StartRecordVoice(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*,
                          int bindedKey) {

     std::thread sendThread([bindedKey]() {
        StartRecording();
        logger::info("Voice thread ended");
    });
    sendThread.detach();

    return 0;
    
}

int Papyrus::StopRecordVoice(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID,
                           RE::StaticFunctionTag*, int bindedKey) {

    VoiceRecordControl::getInstance().setRecording(false);

    return 0;
}

bool Papyrus::RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm) {
    a_vm->RegisterFunction("StartRecordVoice", "WebSocketSTT", StartRecordVoice, false);
    a_vm->RegisterFunction("StopRecordVoice", "WebSocketSTT", StopRecordVoice, false);
        return true;
}

