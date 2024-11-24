#ifndef TEXTGAMEVIEW_H
#define TEXTGAMEVIEW_H

#include <QWidget>
#include <QTextEdit>
#include <QHBoxLayout>
#include "gamemodel.h"

class TextGameView : public QWidget
{
    Q_OBJECT

public:
    explicit TextGameView(GameModel *model, QWidget *parent = nullptr);
    ~TextGameView() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateView();
    void handleGameOver();
    void handleModelReset(); // Slot to handle model reset

private:
    void renderTextWorld();
    void updateStatus();
    void setupLayout();

    GameModel *model;
    QTextEdit *textEdit;
    QTextEdit *statusTextEdit; // Status panel
};

#endif // TEXTGAMEVIEW_H
