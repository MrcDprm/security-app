cmake_minimum_required(VERSION 3.21)
project(PasswordVault VERSION 0.1.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
qt_standard_project_setup()

# Kriptografi ve kasa mantığı: arayüzden bağımsız, testler de bunu kullanır
qt_add_library(vault_core STATIC
    src/core/Entry.h
    src/core/Crypto.cpp
    src/core/VaultFile.cpp
    src/core/VaultSession.cpp
    src/core/Generator.cpp
    src/core/Strength.cpp
    src/core/Totp.cpp
    src/core/Health.cpp
    src/core/CsvImport.cpp
    src/core/DemoVault.cpp
)
target_include_directories(vault_core PUBLIC src)
target_link_libraries(vault_core PUBLIC Qt6::Core PRIVATE PkgConfig::SODIUM)
# EFF kelime listesi programın içine gömülür
qt_add_resources(vault_core "wordlist" PREFIX "/" BASE resources FILES resources/eff_short_wordlist_1.txt)

# Sürüm numarası ve ikon .exe dosyasına buradan gömülür (Özellikler > Ayrıntılar)
configure_file(resources/app.rc.in ${CMAKE_BINARY_DIR}/app.rc @ONLY)

qt_add_executable(PasswordVault WIN32
    src/main.cpp
    src/app/I18n.cpp
    src/app/Settings.cpp
    src/app/Theme.cpp
    src/services/AutoLock.cpp
    src/services/BreachCheck.cpp
    src/services/SecureClipboard.cpp
    src/ui/PasswordField.cpp
    src/ui/WelcomePage.cpp
    src/ui/UnlockPage.cpp
    src/ui/EntryDialog.cpp
    src/ui/EntryPanel.cpp
    src/ui/EntriesView.cpp
    src/ui/TotpView.cpp
    src/ui/HealthView.cpp
    src/ui/GeneratorView.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AboutDialog.cpp
    src/ui/VaultPage.cpp
    src/ui/MainWindow.cpp
    ${CMAKE_BINARY_DIR}/app.rc
)
qt_add_resources(PasswordVault "app_resources" PREFIX "/" BASE resources FILES resources/icon.png)
target_link_libraries(PasswordVault PRIVATE vault_core Qt6::Widgets Qt6::Network)
target_compile_definitions(PasswordVault PRIVATE APP_VERSION="${PROJECT_VERSION}")

enable_testing()
qt_add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE vault_core Qt6::Test)
add_test(NAME test_core COMMAND test_core)
qt_add_executable(test_services tests/test_services.cpp src/services/BreachCheck.cpp src/app/Settings.cpp)
target_link_libraries(test_services PRIVATE vault_core Qt6::Network Qt6::Test)
add_test(NAME test_services COMMAND test_services)
