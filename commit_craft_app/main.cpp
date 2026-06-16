#include "mainwindow.h"
#include "appstyle.h"

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>
#include <QIcon>
#include <QFont>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("Commit Craft");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("CommitCraft");

    // Глобальный шрифт приложения
    QFont defaultFont = a.font();
    defaultFont.setFamily("Segoe UI");
    defaultFont.setPointSize(10);
    a.setFont(defaultFont);

    // Глобальная таблица стилей
    a.setStyleSheet(globalStylesheet());

    // Показываем splash screen (минимум 3 секунды)
    QPixmap splashPixmap(":/icons/icons/splash.png");
    // Указываем DPR экрана — иначе Qt считает pixmap логическим и создаёт окно
    // размером 800*DPI x 446*DPI физических пикселей, растягивая картинку
    splashPixmap.setDevicePixelRatio(a.primaryScreen()->devicePixelRatio());
    QSplashScreen splash(splashPixmap, Qt::WindowFlags(Qt::WindowType::WindowStaysOnTopHint));
    splash.show();
    a.processEvents();

    MainWindow w;
    w.setWindowIcon(QIcon(":/icons/icons/icon.png"));
    w.show();
    a.processEvents();

    // Скрываем splash через 3 секунды после появления главного окна
    QTimer::singleShot(3000, &splash, &QSplashScreen::close);    

    return a.exec();
}
