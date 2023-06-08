/********************************************************************************
** Form generated from reading UI file 'beingsnap.ui'
**
** Created by: Qt User Interface Compiler version 6.4.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BEINGSNAP_H
#define UI_BEINGSNAP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_beingSnap
{
public:
    QPushButton *startBtn;
    QPushButton *pushButton_2;

    void setupUi(QWidget *beingSnap)
    {
        if (beingSnap->objectName().isEmpty())
            beingSnap->setObjectName("beingSnap");
        beingSnap->resize(400, 300);
        startBtn = new QPushButton(beingSnap);
        startBtn->setObjectName("startBtn");
        startBtn->setGeometry(QRect(70, 130, 80, 18));
        pushButton_2 = new QPushButton(beingSnap);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(200, 130, 80, 18));

        retranslateUi(beingSnap);

        QMetaObject::connectSlotsByName(beingSnap);
    } // setupUi

    void retranslateUi(QWidget *beingSnap)
    {
        beingSnap->setWindowTitle(QCoreApplication::translate("beingSnap", "Form", nullptr));
        startBtn->setText(QCoreApplication::translate("beingSnap", "Start", nullptr));
        pushButton_2->setText(QCoreApplication::translate("beingSnap", "Quit", nullptr));
    } // retranslateUi

};

namespace Ui {
    class beingSnap: public Ui_beingSnap {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BEINGSNAP_H
