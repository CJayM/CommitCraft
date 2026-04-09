#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    QCoreApplication::setApplicationName("Commit Craft");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("CommitCraft");
    
    // Показываем splash screen
    QPixmap splashPixmap(":/icons/icons/splash.png");
    QSplashScreen splash(splashPixmap);
    splash.show();
    a.processEvents();
    
    MainWindow w;
    w.setWindowIcon(QIcon(":/icons/icons/icon.png"));
    w.show();
    
    // Скрываем splash через 3 секунды
    QTimer::singleShot(3000, &splash, &QSplashScreen::close);
    
    return a.exec();
}
