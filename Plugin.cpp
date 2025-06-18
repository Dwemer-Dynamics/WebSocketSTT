#include <SkyrimScripting/Plugin.h>
#include "Papyrus.h"


extern void InitializeWebSocket();

OnInit { 
    logger::info("OnInit: WebSocketSTT "); 
}

OnDataLoaded { 
    logger::info("OnDataLoaded: WebSocketSTT ");

    const auto papyrus = SKSE::GetPapyrusInterface();
    papyrus->Register(Papyrus::RegisterFunctions);


    logger::info("OnDataLoaded End");

    InitializeWebSocket();
}

