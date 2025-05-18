#include <windows.h>
#include <XInput.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>

#pragma comment(lib, "XInput.lib")
#pragma comment(lib, "Comctl32.lib")

// Идентификаторы элементов управления
#define ID_TIMER 1
#define IDC_SPEED_SLIDER 101
#define IDC_SCROLL_SLIDER 102
#define IDC_DEADZONE_SLIDER 103
#define IDC_VIBRATION_SLIDER 104
#define IDC_ENABLE_CHECK 105
#define IDC_STATUS_TEXT 106

class GamepadControllerApp {
private:
    HWND hMainWnd;
    HWND hSpeedLabel, hScrollLabel, hDeadzoneLabel, hVibrationLabel, hStatusText;
    HWND hSpeedSlider, hScrollSlider, hDeadzoneSlider, hVibrationSlider;
    HWND hEnableCheck;

    // Настройки
    float cursorSpeed = 10.0f;
    float scrollStep = 20.0f;
    float deadZone = 0.15f;
    int vibrationPower = 50;
    bool isEnabled = true;

    // Состояние
    bool isWindowDragMode = false;
    HWND selectedWindow = nullptr;
    POINT windowDragStartPos;
    RECT windowDragStartRect;
    bool isConnected = false;

    // Состояние кнопок
    bool wasAPressed = false, wasBPressed = false;
    bool wasXPressed = false, wasYPressed = false;
    bool wasLeftShoulderPressed = false, wasRightShoulderPressed = false;
    bool isLeftButtonDown = false;
    bool isRightButtonDown = false;
    bool wasLeftTriggerPressed = false, wasRightTriggerPressed = false;

    // Инициализация элементов управления
    void InitControls() {
        // Метки
        hSpeedLabel = CreateWindowW(L"STATIC", L"Скорость курсора: 10",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 20, 200, 20, hMainWnd, NULL, NULL, NULL);

        hScrollLabel = CreateWindowW(L"STATIC", L"Шаг скролла: 20",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 60, 200, 20, hMainWnd, NULL, NULL, NULL);

        hDeadzoneLabel = CreateWindowW(L"STATIC", L"Мертвая зона: 15%",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 100, 200, 20, hMainWnd, NULL, NULL, NULL);

        hVibrationLabel = CreateWindowW(L"STATIC", L"Сила вибрации: 50%",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 140, 200, 20, hMainWnd, NULL, NULL, NULL);

        // Слайдеры
        hSpeedSlider = CreateWindowW(TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
            220, 20, 150, 30, hMainWnd, (HMENU)IDC_SPEED_SLIDER, NULL, NULL);
        SendMessageW(hSpeedSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 20));
        SendMessageW(hSpeedSlider, TBM_SETPOS, TRUE, (int)cursorSpeed);

        hScrollSlider = CreateWindowW(TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
            220, 60, 150, 30, hMainWnd, (HMENU)IDC_SCROLL_SLIDER, NULL, NULL);
        SendMessageW(hScrollSlider, TBM_SETRANGE, TRUE, MAKELONG(5, 50));
        SendMessageW(hScrollSlider, TBM_SETPOS, TRUE, (int)scrollStep);

        hDeadzoneSlider = CreateWindowW(TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
            220, 100, 150, 30, hMainWnd, (HMENU)IDC_DEADZONE_SLIDER, NULL, NULL);
        SendMessageW(hDeadzoneSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 50));
        SendMessageW(hDeadzoneSlider, TBM_SETPOS, TRUE, (int)(deadZone * 100));

        hVibrationSlider = CreateWindowW(TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
            220, 140, 150, 30, hMainWnd, (HMENU)IDC_VIBRATION_SLIDER, NULL, NULL);
        SendMessageW(hVibrationSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessageW(hVibrationSlider, TBM_SETPOS, TRUE, vibrationPower);

        // Чекбокс и статус
        hEnableCheck = CreateWindowW(L"BUTTON", L"Включить управление",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            20, 180, 200, 30, hMainWnd, (HMENU)IDC_ENABLE_CHECK, NULL, NULL);
        SendMessageW(hEnableCheck, BM_SETCHECK, BST_CHECKED, 0);

        hStatusText = CreateWindowW(L"STATIC", L"Геймпад не подключен",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 220, 350, 30, hMainWnd, (HMENU)IDC_STATUS_TEXT, NULL, NULL);
    }

    // Обновление текста элементов управления
    void UpdateControls() {
        std::wstring speedText = L"Скорость курсора: " + std::to_wstring((int)cursorSpeed);
        SetWindowTextW(hSpeedLabel, speedText.c_str());

        std::wstring scrollText = L"Шаг скролла: " + std::to_wstring((int)scrollStep);
        SetWindowTextW(hScrollLabel, scrollText.c_str());

        std::wstring deadzoneText = L"Мертвая зона: " + std::to_wstring((int)(deadZone * 100)) + L"%";
        SetWindowTextW(hDeadzoneLabel, deadzoneText.c_str());

        std::wstring vibrationText = L"Сила вибрации: " + std::to_wstring(vibrationPower) + L"%";
        SetWindowTextW(hVibrationLabel, vibrationText.c_str());

        if (isConnected) {
            SetWindowTextW(hStatusText, L"Геймпад подключен");
        }
        else {
            SetWindowTextW(hStatusText, L"Геймпад не подключен");
        }
    }

    // Управление вибрацие
    void Vibrate(int leftMotor, int rightMotor) {
        XINPUT_VIBRATION vibration;
        ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
        vibration.wLeftMotorSpeed = leftMotor * vibrationPower / 100;
        vibration.wRightMotorSpeed = rightMotor * vibrationPower / 100;
        XInputSetState(0, &vibration);
    }

    void StopVibration() {
        Vibrate(0, 0);
    }

    // Обработка ввода с геймпада
    void ProcessGamepadInput() {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        DWORD result = XInputGetState(0, &state);
        bool currentlyConnected = (result == ERROR_SUCCESS);

        if (currentlyConnected != isConnected) {
            isConnected = currentlyConnected;
            UpdateControls();

            if (isConnected) {
                Vibrate(30000, 15000);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                StopVibration();
            }
        }

        if (!isEnabled || !isConnected) {
            // Отпускаем все кнопки при отключении
            if (isLeftButtonDown) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                isLeftButtonDown = false;
            }
            if (isRightButtonDown) {
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                isRightButtonDown = false;
            }
            return;
        }

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        // Обработка осей
        float LX = state.Gamepad.sThumbLX / 32767.0f;
        float LY = state.Gamepad.sThumbLY / 32767.0f;
        float RX = state.Gamepad.sThumbRX / 32767.0f;
        float RY = state.Gamepad.sThumbRY / 32767.0f;

        // Применение мертвой зоны
        if (fabs(LX) < deadZone) LX = 0;
        if (fabs(LY) < deadZone) LY = 0;
        if (fabs(RX) < deadZone) RX = 0;
        if (fabs(RY) < deadZone) RY = 0;

        // Квадратичная кривая для более точного управления
        LX = copysign(LX * LX, LX);
        LY = copysign(LY * LY, LY);
        RX = copysign(RX * RX, RX);
        RY = copysign(RY * RY, RY);

        // Управление курсором/окном
        if (!isWindowDragMode) {
            int newX = cursorPos.x + (int)(LX * cursorSpeed);
            int newY = cursorPos.y - (int)(LY * cursorSpeed);
            newX = max(0, min(GetSystemMetrics(SM_CXSCREEN) - 1, newX));
            newY = max(0, min(GetSystemMetrics(SM_CYSCREEN) - 1, newY));
            SetCursorPos(newX, newY);
        }
        else if (selectedWindow) {
            RECT rect;
            GetWindowRect(selectedWindow, &rect);
            int newX = rect.left + (int)(LX * 5);
            int newY = rect.top + (int)(LY * 5);
            MoveWindow(selectedWindow, newX, newY,
                rect.right - rect.left, rect.bottom - rect.top, TRUE);
        }

        // Скроллинг
        if (fabs(RX) > 0 || fabs(RY) > 0) {
            mouse_event(MOUSEEVENTF_HWHEEL, 0, 0, (int)(RX * scrollStep * 20), 0);
            mouse_event(MOUSEEVENTF_WHEEL, 0, 0, (int)(RY * scrollStep * 20), 0);
        }

        // Обработка кнопок
        bool isAPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_A;
        bool isBPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_B;
        bool isXPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_X;
        bool isYPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
        bool isLeftShoulderPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
        bool isRightShoulderPressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        bool isLeftTriggerPressed = (state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        bool isRightTriggerPressed = (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

        // Левый клик (X button)
        if (isXPressed && !wasXPressed) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            isLeftButtonDown = true;
            Vibrate(15000, 7500);
        }
        else if (!isXPressed && wasXPressed && isLeftButtonDown) {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            isLeftButtonDown = false;
            StopVibration();
        }

        // Правый клик (B button)
        if (isBPressed && !wasBPressed) {
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
            isRightButtonDown = true;
            Vibrate(20000, 10000);
        }
        else if (!isBPressed && wasBPressed && isRightButtonDown) {
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
            isRightButtonDown = false;
            StopVibration();
        }

        // Левый триггер - зажатие левой кнопки
        if (isLeftTriggerPressed && !wasLeftTriggerPressed) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            isLeftButtonDown = true;
            Vibrate(10000, 5000);
        }
        else if (!isLeftTriggerPressed && wasLeftTriggerPressed && isLeftButtonDown) {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            isLeftButtonDown = false;
            StopVibration();
        }

        // Правый триггер - зажатие правой кнопки
        if (isRightTriggerPressed && !wasRightTriggerPressed) {
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
            isRightButtonDown = true;
            Vibrate(15000, 7500);
        }
        else if (!isRightTriggerPressed && wasRightTriggerPressed && isRightButtonDown) {
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
            isRightButtonDown = false;
            StopVibration();
        }

        // Перетаскивание окна (A button)
        if (isAPressed && !wasAPressed) {
            if (isWindowDragMode) {
                Vibrate(30000, 15000);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                StopVibration();

                isWindowDragMode = false;
                if (selectedWindow) {
                    SetWindowLongPtrW(selectedWindow, GWL_EXSTYLE,
                        GetWindowLongPtrW(selectedWindow, GWL_EXSTYLE) & ~WS_EX_LAYERED);
                }
            }
            else {
                selectedWindow = WindowFromPoint(cursorPos);
                if (selectedWindow) {
                    Vibrate(40000, 20000);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    StopVibration();

                    isWindowDragMode = true;
                    GetWindowRect(selectedWindow, &windowDragStartRect);
                    GetCursorPos(&windowDragStartPos);
                    SetWindowLongPtrW(selectedWindow, GWL_EXSTYLE,
                        GetWindowLongPtrW(selectedWindow, GWL_EXSTYLE) | WS_EX_LAYERED);
                    SetLayeredWindowAttributes(selectedWindow, 0, 180, LWA_ALPHA);
                }
            }
        }

        // Обновление состояний кнопок
        wasAPressed = isAPressed;
        wasBPressed = isBPressed;
        wasXPressed = isXPressed;
        wasYPressed = isYPressed;
        wasLeftShoulderPressed = isLeftShoulderPressed;
        wasRightShoulderPressed = isRightShoulderPressed;
        wasLeftTriggerPressed = isLeftTriggerPressed;
        wasRightTriggerPressed = isRightTriggerPressed;
    }

public:
    GamepadControllerApp() {
        // Регистрация класса окна
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"GamepadControllerClass";
        RegisterClassExW(&wc);

        // Создание главного окна
        hMainWnd = CreateWindowExW(0, L"GamepadControllerClass", L"Gamepad Controller",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            400, 300, NULL, NULL, GetModuleHandleW(NULL), this);

        // Инициализация элементов управления
        InitControls();
        UpdateControls();

        ShowWindow(hMainWnd, SW_SHOW);
        UpdateWindow(hMainWnd);

        // Таймер для обработки ввода (60 FPS)
        SetTimer(hMainWnd, ID_TIMER, 16, NULL);
    }

    // Обработчик сообщений окна
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        GamepadControllerApp* pThis = nullptr;

        if (msg == WM_NCCREATE) {
            CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
            pThis = (GamepadControllerApp*)pCreate->lpCreateParams;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else {
            pThis = (GamepadControllerApp*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        }

        if (pThis) {
            return pThis->HandleMessage(hWnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_HSCROLL: {
            HWND hSlider = (HWND)lParam;
            if (hSlider == hSpeedSlider) {
                cursorSpeed = (float)SendMessageW(hSpeedSlider, TBM_GETPOS, 0, 0);
            }
            else if (hSlider == hScrollSlider) {
                scrollStep = (float)SendMessageW(hScrollSlider, TBM_GETPOS, 0, 0);
            }
            else if (hSlider == hDeadzoneSlider) {
                deadZone = (float)SendMessageW(hDeadzoneSlider, TBM_GETPOS, 0, 0) / 100.0f;
            }
            else if (hSlider == hVibrationSlider) {
                vibrationPower = (int)SendMessageW(hVibrationSlider, TBM_GETPOS, 0, 0);
            }
            UpdateControls();
            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_ENABLE_CHECK) {
                isEnabled = SendMessageW(hEnableCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            }
            return 0;
        }
        case WM_TIMER: {
            ProcessGamepadInput();
            return 0;
        }
        case WM_CLOSE: {
            KillTimer(hWnd, ID_TIMER);
            DestroyWindow(hWnd);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    void Run() {
        MSG msg = { 0 };
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    // Запуск приложения
    GamepadControllerApp app;
    app.Run();

    return 0;
}