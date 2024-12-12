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
#include "gamemodel.h"
#include "gameview.h"
#include "textgameview.h"
#include "pathfinder_class.h"
#include "node.h"
#include "commandparser.h"
#include "xenemy.h"
#include "penemy.h"

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

    // Pathfinding/auto-play
    void decideNextAction();
    EnemyWrapper* findNextTargetEnemy();
    void planPathToEnemy(EnemyWrapper* targetEnemy);
    void planPathToHealthPack();
    void planPathToPortal();
    void planPathToEnemyWithHealthPacks(EnemyWrapper* targetEnemy);
    void resetNodes();

    bool autoPlayActive = false;    // tracks if auto-play is currently active
    bool oneShotMovement = false;   // true if we are just moving to a clicked tile once

    bool saveGameToFile(const QString &fileName);
    bool loadGameFromFile(const QString &fileName);

    struct CachedLevel {
        std::vector<std::unique_ptr<TileWrapper>> tiles;
        std::unique_ptr<ProtagonistWrapper> protagonist;
        std::vector<std::unique_ptr<EnemyWrapper>> enemies;
        std::vector<std::unique_ptr<HealthPack>> healthPacks;
        std::vector<std::unique_ptr<Portal>> portals;
        int rows;
        int cols;

        // We'll store in QMap<int,std::shared_ptr<CachedLevel>>.

    };

    // We'll store cached levels as shared_ptr to avoid copying unique_ptr problems:
    QMap<int, std::shared_ptr<CachedLevel>> levelCache;

    void loadLevelFromCache(int level);
    void cacheCurrentLevel(int level);

    // clone functions - declare them now:
    std::vector<std::unique_ptr<TileWrapper>> cloneTiles(const std::vector<std::unique_ptr<TileWrapper>> &source);
    std::vector<std::unique_ptr<EnemyWrapper>> cloneEnemies(const std::vector<std::unique_ptr<EnemyWrapper>> &source);
    std::vector<std::unique_ptr<HealthPack>> cloneHealthPacks(const std::vector<std::unique_ptr<HealthPack>> &source);
    std::vector<std::unique_ptr<Portal>> clonePortals(const std::vector<std::unique_ptr<Portal>> &source);

    void convertRandomEnemiesToXEnemies(std::vector<std::unique_ptr<EnemyWrapper>> &enemies, int cols, int rows);

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

    std::vector<int> autoPath;
    int autoPathIndex;

    enum class TargetType { None, Enemy, HealthPack, Portal };
    TargetType currentTarget;

    std::vector<Node> nodes;
    Comparator<Node> nodeComparator;

    CommandParser commandParser;
};

#endif // GAMECONTROLLER_H
