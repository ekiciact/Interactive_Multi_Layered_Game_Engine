#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>
#include <memory>
#include <vector>
#include <QAction>
#include <QMenu>
#include <QMap>
#include <memory> // for shared_ptr
#include <functional>

#include "gamemodel.h"
#include "gameview.h"
#include "textgameview.h"
#include "pathfinder_class.h"
#include "node.h"
#include "commandparser.h"
#include "autoplaystrategy.h"
#include "defaultautoplaystrategy.h"
#include "gamestatemanager.h"

class GameController : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameController(QWidget *parent = nullptr);
    ~GameController();

    void show();

public slots:
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void gotoXY(int x, int y);
    void attackNearestEnemy();
    void takeNearestHealthPack();
    void printHelp();

private slots:
    void switchView();
    void startAutoPlay();
    void stopAutoPlay();
    void handleAutoPlayStep();
    void saveGame();
    void loadGame();
    void newGame();
    void restartGame();
    void onTileSelected(int x, int y);
    void handleTextCommand(QString command);

    // New slot for command-based movement steps
    void handleCommandMoveStep();

private:
    void setupModel();
    void setupViews();
    void setupConnections();
    void createActions();
    void createMenus();
    void setupCommands();

    void moveProtagonist(int dx, int dy);
    void checkForEncounters();
    void checkForHealthPacks();
    void checkForPortal();
    void handlePEnemyPoison(PEnemy *pEnemy);

    std::vector<int> computeDirectPath(int startX, int startY, int endX, int endY, bool avoidPortalIfEnemies = false);

    // Updated: Instead of instantly moving along the path, we store it and animate.
    void startCommandPathMovement(const std::vector<int> &path);

    // For mouse click movement (direct path movement)
    void moveProtagonistDirectlyToTile(int x, int y);

    EnemyWrapper* findNearestUndefeatedEnemy();
    HealthPack* findNearestHealthPack();

    bool autoPlayActive = false;
    bool oneShotMovement = false;

    QMap<int, std::shared_ptr<GameStateManager::CachedLevel>> levelCache;

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

    QTimer *autoPlayTimer;

    CommandParser commandParser;

    std::unique_ptr<AutoPlayStrategy> autoPlayStrategy;
    GameStateManager gameStateManager;

    // New fields for command-based movement animation
    QTimer *commandMoveTimer;
    std::vector<int> commandPath;
    int commandPathIndex;
};

#endif // GAMECONTROLLER_H
