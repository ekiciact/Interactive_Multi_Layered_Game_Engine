#include <QApplication>
#include "gamecontroller.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GameController controller;
    controller.show();

    return app.exec();
}
