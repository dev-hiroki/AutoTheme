#include <nlohmann/json_fwd.hpp>

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

std::string ToStr(std::wstring Str)
{
    int Size = WideCharToMultiByte(CP_UTF8, 0, Str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (Size == 0)
    {
        return {};
    }
    std::string Out(Size - 1, '\0');

    WideCharToMultiByte(CP_UTF8, 0, Str.c_str(), -1, Out.data(), Size, nullptr, nullptr);

    return Out;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    std::string Path = "";
    {
        PWSTR PathW = nullptr;
        HRESULT Result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &PathW);
        if (SUCCEEDED(Result))
        {
            std::wstring  WPath(PathW);
            CoTaskMemFree(PathW);
            Path = ToStr(WPath) + "\\AutoTheme";
        }
    }

    int LightStart = 7;
    int LightEnd = 17;
    if (!Path.empty())
    {
        std::filesystem::create_directory(Path);

#ifdef DEBUG
        AllocConsole();

        auto ConsoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log.txt", true);

        std::vector<spdlog::sink_ptr> Sinks = { ConsoleSink, FileSink };
        auto Logger = std::make_shared<spdlog::logger>("", Sinks.begin(), Sinks.end());

        spdlog::set_default_logger(Logger);
#else
        auto FileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(Path + "\\Log.txt", true);

        std::vector<spdlog::sink_ptr> Sinks = { FileSink };
        auto Logger = std::make_shared<spdlog::logger>("", Sinks.begin(), Sinks.end());

        spdlog::set_default_logger(Logger);
#endif
        spdlog::flush_on(spdlog::level::info);

        if (std::filesystem::exists(Path + "\\Config.json"))
        {
            try
            {
                std::ifstream File(Path + "\\Config.json");
                nlohmann::json Config = nlohmann::json::parse(File);

                LightStart = Config["LightStart"];
                LightEnd = Config["LightEnd"];

                spdlog::info("Config loaded:");
                spdlog::info("Light Start: {}, Light End: {}", LightStart, LightEnd);
            }
            catch (...)
            {
                spdlog::error("Failed Parse Config");
            }
        }
        else
        {
            std::ofstream File(Path + "\\Config.json");

            if (File.is_open())
            {
                nlohmann::json Config;
                Config["LightStart"] = LightStart;
                Config["LightEnd"] = LightEnd;

                File << Config.dump(4);
            }
            else
            {
                spdlog::error("Failed Create Config");
            }
        }
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

            if (LocalTime.tm_hour > LightStart && LocalTime.tm_hour < LightEnd)
            {
                UseLight = true;
            }
            else
            {
                UseLight = false;
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