#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTextEdit>
#include "gamemodel.h"

class GameView : public QWidget
{
    Q_OBJECT

public:
    explicit GameView(GameModel *model, QWidget *parent = nullptr);
    ~GameView();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateView();
    void handleGameOver();
    void handleModelReset(); // Slot to handle model reset

private:
    void setupScene();
    void drawTiles();
    void drawEntities();
    void animateProtagonist(const QString &action);

    void updateStatus();
    void setupLayout();

    void graphicsViewKeyPressEvent(QKeyEvent *event);
    void graphicsViewWheelEvent(QWheelEvent *event);

    GameModel *model;
    QGraphicsView *graphicsView;
    QGraphicsScene *scene;

    QMap<Tile*, QGraphicsRectItem*> tileItems;
    QMap<Enemy*, QGraphicsPixmapItem*> enemyItems;
    QGraphicsPixmapItem *protagonistItem;
    QMap<Tile*, QGraphicsPixmapItem*> healthPackItems;
    QMap<Tile*, QGraphicsPixmapItem*> portalItems; // Portal items

    QTextEdit *statusTextEdit;
};

#endif // GAMEVIEW_H
