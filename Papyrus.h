#pragma once

extern bool sttBindedKey;

namespace Papyrus {

    int StartRecordVoice(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*,
                      int bindedKey);
    int StopRecordVoice(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag*,
                      int bindedKey);

    bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm);

}