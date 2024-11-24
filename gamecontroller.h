#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QMainWindow>
#include <QStackedWidget>
#include "gamemodel.h"
#include "gameview.h"
#include "textgameview.h"

class GameController : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameController(QWidget *parent = nullptr);
    ~GameController();

    void show();

private:
    void setupModel();
    void setupViews();
    void setupConnections();
    void createActions();
    void createMenus();

    GameModel *model;
    GameView *graphicView;
    TextGameView *textView;
    QStackedWidget *stackedWidget;

    QAction *switchViewAction;
    QAction *autoPlayAction;
    QAction *saveGameAction;
    QAction *loadGameAction;
    QAction *newGameAction;
    QAction *restartGameAction;
    QMenu *gameMenu;
    QMenu *viewMenu;

private slots:
    void switchView();
    void startAutoPlay();
    void saveGame();
    void loadGame();
    void newGame();
    void restartGame();
};

#endif // GAMECONTROLLER_H
