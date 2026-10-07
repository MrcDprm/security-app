#include "app/I18n.h"
#include "app/Settings.h"
#include "app/Theme.h"
#include "core/Crypto.h"
#include "ui/MainWindow.h"
#include "ui/UiHelpers.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PasswordVault");
    app.setOrganizationName("MrcDprm");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/icon.png"));

    // Kasa ve ayarlar program klasörüne değil kullanıcı klasörüne yazılır: %APPDATA%\MrcDprm\PasswordVault
    const QString dataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataFolder);

    Settings settings(dataFolder);
    I18n::setLanguage(settings.language());
    Theme::apply(app, settings.darkTheme());

    if (!Crypto::init()) {
        Ui::message(nullptr, QMessageBox::Critical, I18n::error("crypto_failed"));
        return 1;
    }

    MainWindow window(settings, dataFolder);
    window.show();
    return app.exec();
}
