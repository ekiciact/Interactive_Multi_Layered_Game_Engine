#ifndef GAMEVIEW_H
#define GAMEVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QTextEdit>
#include <QMap>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QProgressBar>
#include <memory>
#include "gamemodel.h"


class GameView : public QWidget
{
    Q_OBJECT

public:
    explicit GameView(GameModel *model, QWidget *parent = nullptr);
    ~GameView();
    void setOverlayVisible(bool visible); // Toggle visibility of overlay
    bool isOverlayVisible() const;        // Check if overlay is visible
    void setOverlayImage(const QString &path); // Set a new overlay image
    void setUniversalOverlayImage(const QString &path);

signals:
    void moveRequest(int dx, int dy);
    void autoPlayRequest();
    void tileSelected(int x, int y);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void updateView();
    void handleGameOver();
    void handleModelReset();

private:
    void setupScene();
    void drawTiles();
    void drawEntities();
    void updateOverlay(); // Update overlay size/position during zoom
    void animateProtagonist(const QString &action);
    void updateStatus();
    void setupLayout();

    void graphicsViewKeyPressEvent(QKeyEvent *event);
    void graphicsViewWheelEvent(QWheelEvent *event);

    GameModel *model;
    QGraphicsView *graphicsView;
    QGraphicsScene *scene;

    QMap<TileWrapper*, QGraphicsRectItem*> tileItems;
    QMap<EnemyWrapper*, QGraphicsPixmapItem*> enemyItems;
    QGraphicsPixmapItem *protagonistItem;
    QMap<HealthPack*, QGraphicsPixmapItem*> healthPackItems;
    QMap<Portal*, QGraphicsPixmapItem*> portalItems;

    QTextEdit *statusTextEdit;
    QProgressBar *healthBar;
    QProgressBar *energyBar;

    QGraphicsPixmapItem *overlayItem; // Overlay image item
    QString currentOverlayPath;      // Path to the overlay image
};

#endif // GAMEVIEW_H
