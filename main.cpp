#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow main_window;
    main_window.center_and_resize();
    main_window.show();

    thread save_thread{&MainWindow::save_worker, &main_window};

    return a.exec();
}
