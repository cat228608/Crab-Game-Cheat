#include <Windows.h>
#include <iostream>
#include <string>
#include <MinHook.h>

// Anti-Cheat 'StopDetection'
#define RVA_STOP_DETECTION 0x2257B0 

// PlayerMovement->Update (для B-hop)
#define RVA_PLAYER_UPDATE 0x17C67F0

// TrapController->OnTriggerEnter (для отключения ловушек)
#define RVA_TRAP_ON_TRIGGER_ENTER 0x1422B10

// PlayerMovement->GetFallSpeed (для No Fall Damage)
#define RVA_GET_FALL_SPEED 0x17BE4D0

// PlayerMovement->set_playerHeight (для режима крысы)
#define RVA_SET_PLAYER_HEIGHT 0x17C6C50

// PlayerMovement->get_Instance (чтобы получить указатель на игрока)
#define RVA_GET_PLAYERMOVEMENT_INSTANCE 0x17C6BA0

// Смещение поля 'grounded' в классе PlayerMovement
#define OFFSET_GROUNDED 0x104

HMODULE g_hModule = NULL;
DWORD64 g_baseAddress = 0;
bool g_bConsoleCreated = false;
FILE* g_pConsoleStream = nullptr;

bool bBhop = false;
bool bDisableTraps = false;
bool bNoFallDamage = false;
bool bTinyMode = false;

typedef void (*tStopDetection)();

typedef void(__fastcall* tUpdate)(void* pThis);
tUpdate pOriginalUpdate = nullptr;

typedef float(__fastcall* tGetFallSpeed)(void* pThis);
tGetFallSpeed pOriginalGetFallSpeed = nullptr;

typedef void(__fastcall* tSetPlayerHeight)(void* pThis, float height);
typedef void* (*tGetPlayerMovementInstance)();

typedef void(__fastcall* tOnTriggerEnter)(void* pThis, void* pCollider);
tOnTriggerEnter pOriginalOnTriggerEnter = nullptr;

// хук на PlayerMovement->Update
void __fastcall DetourUpdate(void* pThis)
{
    if (pThis != nullptr && bBhop)
    {
        bool* pGrounded = (bool*)((DWORD64)pThis + OFFSET_GROUNDED);
        *pGrounded = true;
    }
    pOriginalUpdate(pThis);
}

// хук на TrapController->OnTriggerEnter
void __fastcall DetourOnTriggerEnter(void* pThis, void* pCollider)
{
    if (bDisableTraps)
    {
        return; // Игнорируем срабатывание ловушки
    }
    pOriginalOnTriggerEnter(pThis, pCollider);
}

// хук на PlayerMovement->GetFallSpeed
float __fastcall DetourGetFallSpeed(void* pThis)
{
    if (bNoFallDamage)
    {
        return 0.0f;
    }
    return pOriginalGetFallSpeed(pThis);
}

void MainThread()
{
    AllocConsole();
    freopen_s(&g_pConsoleStream, "CONOUT$", "w", stdout);
    g_bConsoleCreated = true;
    std::cout << ">>> Crab Hack by Mafioznik <<<" << std::endl;

    g_baseAddress = (DWORD64)GetModuleHandleA("GameAssembly.dll");
    if (g_baseAddress == 0) {
        std::cout << "[FATAL ERROR] GameAssembly.dll not found!" << std::endl;
        Sleep(5000);
        if (g_pConsoleStream) fclose(g_pConsoleStream);
        FreeConsole();
        FreeLibraryAndExitThread(g_hModule, 0);
        return;
    }

    tStopDetection pStopDetectionFunc = (tStopDetection)(g_baseAddress + RVA_STOP_DETECTION);
    pStopDetectionFunc();
    std::cout << "[SUCCESS] Anti-Cheat disabled." << std::endl;

    if (MH_Initialize() == MH_OK) {
        MH_CreateHook((LPVOID)(g_baseAddress + RVA_PLAYER_UPDATE), &DetourUpdate, (LPVOID*)&pOriginalUpdate);
        MH_CreateHook((LPVOID)(g_baseAddress + RVA_TRAP_ON_TRIGGER_ENTER), &DetourOnTriggerEnter, (LPVOID*)&pOriginalOnTriggerEnter);
        MH_CreateHook((LPVOID)(g_baseAddress + RVA_GET_FALL_SPEED), &DetourGetFallSpeed, (LPVOID*)&pOriginalGetFallSpeed);
        MH_EnableHook(MH_ALL_HOOKS);
        std::cout << "[SUCCESS] All hooks installed." << std::endl;
    }

    std::cout << "\n------------------MENU------------------" << std::endl;
    std::cout << "F1 - endless jumps (Fly)" << std::endl;
    std::cout << "F2 - disable all traps" << std::endl;
    std::cout << "F3 - no fall damage" << std::endl;
    std::cout << "END - Unload Module" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    tGetPlayerMovementInstance GetPlayerMovementFunc = (tGetPlayerMovementInstance)(g_baseAddress + RVA_GET_PLAYERMOVEMENT_INSTANCE);
    tSetPlayerHeight SetPlayerHeightFunc = (tSetPlayerHeight)(g_baseAddress + RVA_SET_PLAYER_HEIGHT);

    while (!(GetAsyncKeyState(VK_END) & 1))
    {
        if (GetAsyncKeyState(VK_F1) & 1) {
            bBhop = !bBhop;
            std::cout << "endless jumps (Fly): " << (bBhop ? "ON" : "OFF") << std::endl;
        }
        if (GetAsyncKeyState(VK_F2) & 1) {
            bDisableTraps = !bDisableTraps;
            std::cout << "disable all traps: " << (bDisableTraps ? "ON" : "OFF") << std::endl;
        }
        if (GetAsyncKeyState(VK_F3) & 1) {
            bNoFallDamage = !bNoFallDamage;
            std::cout << "no fall damage: " << (bNoFallDamage ? "ON" : "OFF") << std::endl;
        }
        Sleep(100);
    }

    if (g_pConsoleStream) fclose(g_pConsoleStream);
    FreeConsole();
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    FreeLibraryAndExitThread(g_hModule, 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, NULL, 0, NULL);
    }
    return TRUE;
}