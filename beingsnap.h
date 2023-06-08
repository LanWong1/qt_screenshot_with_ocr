#ifndef BEINGSNAP_H
#define BEINGSNAP_H

#include <QWidget>
#include "screen.h"
namespace Ui {
class beingSnap;
}

class beingSnap : public QWidget
{
    Q_OBJECT

public:
    explicit beingSnap(QWidget *parent = nullptr);
    ~beingSnap();


private slots:
    void on_startBtn_clicked();

private:
    Ui::beingSnap *ui;
};

#endif // BEINGSNAP_H
