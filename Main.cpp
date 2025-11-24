#include "Resources/Resource.h"


bool SetColor(bool Light)
{
    DWORD Value = Light ? 1 : 0;

    HKEY hKey;
    LSTATUS Open = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_SET_VALUE, &hKey);
    if (Open != ERROR_SUCCESS)
    {
        return false;
    }

    LSTATUS SetAppsUseLightTheme = RegSetValueExW(hKey, L"AppsUseLightTheme", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&Value), sizeof(Value));
    if (SetAppsUseLightTheme != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    LSTATUS SystemUsesLightTheme = RegSetValueExW(hKey, L"SystemUsesLightTheme", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&Value), sizeof(Value));
    if (SystemUsesLightTheme != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ImmersiveColorSet", SMTO_ABORTIFHUNG, 100, nullptr);

    return true;
}

bool GetColor(bool& Color)
{
    DWORD Value;

    HKEY hKey;
    LSTATUS Open = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey);
    if (Open != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD Size = sizeof(Value);
    LSTATUS GetValue = RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<BYTE*>(&Value), &Size);
    if (GetValue != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);

    Color = Value != 0;

    return true;
}


void SendToastNotification(std::wstring App, std::wstring Title, std::wstring Message)
{
    winrt::Windows::Data::Xml::Dom::XmlDocument ToastXML =
        winrt::Windows::UI::Notifications::ToastNotificationManager::GetTemplateContent(
            winrt::Windows::UI::Notifications::ToastTemplateType::ToastText02
            );

    auto Strings = ToastXML.GetElementsByTagName(L"text");
    Strings.Item(0).AppendChild(ToastXML.CreateTextNode(Title.c_str()));
    Strings.Item(1).AppendChild(ToastXML.CreateTextNode(Message.c_str()));

    winrt::Windows::UI::Notifications::ToastNotification Toast{ToastXML};

    auto Notifi = winrt::Windows::UI::Notifications::ToastNotificationManager::CreateToastNotifier(App.c_str());
    Notifi.Show(Toast);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    {
#ifdef DEBUG
        AllocConsole();

        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log.txt", true);

        std::vector<spdlog::sink_ptr> Sinks = { ConsoleSink, FileSink };
        auto Logger = std::make_shared<spdlog::logger>("", Sinks.begin(), Sinks.end());

        spdlog::set_default_logger(Logger);
#else
        auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log.txt", true);

        std::vector<spdlog::sink_ptr> Sinks = { FileSink };
        auto Logger = std::make_shared<spdlog::logger>("", Sinks.begin(), Sinks.end());

        spdlog::set_default_logger(Logger);
#endif
        spdlog::flush_on(spdlog::level::info);
    }

    winrt::init_apartment();
    spdlog::info("Started!");
    SendToastNotification(L"Auto Theme", L"Information", L"Started");

    bool CurrentUseLight = false;
    if (!GetColor(CurrentUseLight))
    {
        spdlog::info("Failed get current theme. so we use 'light' mode");
    }
    else
    {
        spdlog::info("Current Theme is {}", CurrentUseLight ? "Light" : "Dark");
    }

    while (true)
    {
        bool UseLight = false;

        {
            auto Now = std::chrono::system_clock::now();
            time_t T = std::chrono::system_clock::to_time_t(Now);
            tm LocalTime;

            localtime_s(&LocalTime, &T);

            if (LocalTime.tm_hour > 22 || LocalTime.tm_hour < 7)
            {
                UseLight = false;
            }
            else if (LocalTime.tm_hour > 7 && LocalTime.tm_hour < 22)
            {
                UseLight = true;
            }
        }

        if (CurrentUseLight != UseLight)
        {
            spdlog::info("Switched {}", UseLight ? "Light" : "Dark");
            SendToastNotification(L"Auto Theme", L"Information", UseLight ? L"Switched To Light Mode!" : L"Switched To Dark Mode!");
            SetColor(UseLight);

            CurrentUseLight = UseLight;
        }

        Sleep(1000);
    }

    return 0;
}