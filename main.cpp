#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow main_window;
    main_window.showMaximized();

    thread save_thread{&MainWindow::save_worker, &main_window};

    return a.exec();
}
