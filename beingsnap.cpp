#include "beingsnap.h"
#include "ui_beingsnap.h"

beingSnap::beingSnap(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::beingSnap)
{
    ui->setupUi(this);
//this->screen = new Screen(this);
}

beingSnap::~beingSnap()
{
    delete ui;
}

void beingSnap::on_startBtn_clicked()
{
    this->close();
    Screen *s = new Screen();
    s->show();
   //s.update();
    //hide();
   // this->screen->show();
}

