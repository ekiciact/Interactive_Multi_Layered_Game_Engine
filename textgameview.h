#ifndef TEXTGAMEVIEW_H
#define TEXTGAMEVIEW_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QHBoxLayout>
#include "gamemodel.h"

class TextGameView : public QWidget
{
    Q_OBJECT

public:
    explicit TextGameView(GameModel *model, QWidget *parent = nullptr);
    ~TextGameView() override;

    void appendMessage(const QString &msg);

signals:
    void commandEntered(QString command);

private slots:
    void updateView();
    void handleGameOver();
    void handleModelReset();
    void onCommandReturnPressed();

private:
    void renderTextWorld();
    void updateStatus();
    void setupLayout();

    GameModel *model;
    QTextEdit *textEdit;
    QTextEdit *statusTextEdit;
    QLineEdit *commandLine;
};

#endif // TEXTGAMEVIEW_H
