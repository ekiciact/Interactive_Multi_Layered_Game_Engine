#include <QApplication>
#include "gamecontroller.h"

/**

 * Entry point. We create a QApplication and then a GameController which acts as our main window.
 */

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    GameController controller;
    controller.show();

    return app.exec();
}
