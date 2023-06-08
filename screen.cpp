
#include "screen.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPoint>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QFileDialog>
#include <QRectF>
#include "controlwidget.h"
#include <QHBoxLayout>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QPushButton>
#include <QTime>
#include "myrect.h"
#include "qapplication.h"
#include "beingsnap.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//#include <onnxruntime_cxx_api.h>

using namespace cv;
//using namespace Ort;
Screen::Screen(QWidget *parent):
    QWidget(parent),
    widthInfoRect(QRectF(0, 0, 0, 0)),
    control(NULL)
{
    setWindowFlag(Qt::FramelessWindowHint);
    move(0,0);
    //showFullScreen();//将窗口部件全屏显示
    //setWindowOpacity(0.4);
    //截取全屏
    QScreen *scrPix = QGuiApplication::primaryScreen();
    QSize size =  scrPix->size();
    qreal ratio = scrPix->devicePixelRatio();
    setFixedSize(size.width()*ratio,size.height()*ratio);


    oncePress = false;
    draw_rect = false;

    //画框的那个区域
    labelimage = new QSLabel(this);
    labelimage->installEventFilter(this);
    Qt::WindowFlags nType = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
    labelimage->setWindowFlags(nType);
    labelimage->hide();


    ocrTextEdit = new QTextEdit(this);
    ocrTextEdit->setWindowFlags(nType);
    ocrTextEdit->hide();
    setMouseTracking(true);
    //setFocusPolicy(Qt::StrongFocus);
    type = NO;
}



//将上一个函数得到的全屏灰色图绘制在painter上，并添加宽高信息矩形和边角拖曳小正方形
void Screen::paintEvent(QPaintEvent *e)
{
    if(!pixmap){
        QScreen *scrPix = QGuiApplication::primaryScreen();
        pixmap = scrPix->grabWindow(0);
        globalPath.lineTo(pixmap.width(), 0);
        globalPath.lineTo(pixmap.width(), pixmap.height());
        globalPath.lineTo(0, pixmap.height());
        globalPath.lineTo(0, 0);
    }


    QPainter paint(this);
    paint.drawPixmap(0, 0, pixmap);
    //初始化画笔操作
    paint.setPen(Qt::NoPen);
    paint.setBrush(QColor(0, 0, 0, 180));
    //设置路径
    QPainterPath path = getPath();
    paint.drawPath(path);
    //绘制选取左上角显示宽高的区域
    widthInfoRect.drawMe(paint);
    //绘制四个小正方形
    if(draw_rect)
        drawControlArea(paint);
}
//绘制正方形
void Screen::drawControlArea(QPainter &painter)//已看懂
{
    //计算四个小正方形
    rect1.setX(movePoint.x() - 3);
    rect1.setY(pressedPoint.y() - 3);
    rect1.setWidth(6);
    rect1.setHeight(6);
    rect2.setX(pressedPoint.x() - 3);
    rect2.setY(pressedPoint.y() - 3);
    rect2.setWidth(6);
    rect2.setHeight(6);
    rect3.setX(pressedPoint.x() - 3);
    rect3.setY(movePoint.y() - 3);
    rect3.setWidth(6);
    rect3.setHeight(6);
    rect4.setX(movePoint.x() - 3);
    rect4.setY(movePoint.y() - 3);
    rect4.setWidth(6);
    rect4.setHeight(6);
    painter.save();
    painter.setBrush(Qt::blue);
    painter.drawRect(rect1);
    painter.drawRect(rect2);
    painter.drawRect(rect3);
    painter.drawRect(rect4);
    painter.restore();
}


//得到选区之外的路径
//这个函数得到的是开始截图时候得到的白色选区
QPainterPath Screen::getPath()
{
    //选取路径
    QPainterPath path;
    path.moveTo(pressedPoint.x(), pressedPoint.y());
    path.lineTo(movePoint.x(), pressedPoint.y());
    path.lineTo(movePoint.x(), movePoint.y());
    path.lineTo(pressedPoint.x(), movePoint.y());
    path.lineTo(pressedPoint.x(), pressedPoint.y());
    return globalPath.subtracted(path);    //从灰色全屏路径中扣掉这个白色路径区域，然后作为截取的图片？
}


void Screen::mousePressEvent(QMouseEvent *e)
{
    //    if( e->button() & Qt::LeftButton && oncePress)   //如果鼠标左键第一次按下
    //    {
    //       pressedPoint = e->pos();
    //    } else
    //    {
    //        oldPoint = e->pos();
    //    }

    if( e->button() & Qt::LeftButton)   //如果鼠标左键第一次按下
    {
        draw_rect = true;
        Type r =  pointInWhere(e->pos());
        if(r == NO){
            pressedPoint = e->pos();
            movePoint = e->pos();
            oncePress = true;
        }
        else
        {
            oldPoint = e->pos();
        }
    }
}

void Screen::mouseReleaseEvent(QMouseEvent *e)    //只有已经按下鼠标按键，才会存在鼠标释放行为，释放鼠标后会立刻出现控制面板
{


    oncePress = false;
    if( !control )        //如果未出现截图操作控件
    {
        control = new QWidget(this);         //新建一个窗口控件
        controlUi = new ControlWidget(control);  //新建控制窗口
        QHBoxLayout *layout = new QHBoxLayout(control); //在control上建立水平布局
        layout->addWidget(controlUi);         //将控制窗口应用于水平布局
        layout->setContentsMargins(0, 0, 0, 0);
        control->setObjectName("control");
        control->setStyleSheet("QWidget#control{\
                               background-color: #eaecf0;}");
        controlUi->setScreenQuote(this);
    }
    //设置控制面板的位置
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height =  rect.height();

    //control->setGeometry(movePoint.x() - 543, movePoint.y() + 6, 543, 25);
    control->setGeometry(x+width - 543 < 0 ? 0:x+width - 543, y+height + 6, 543, 25);
    control->show();
    //labelimage->show();


//OCR();
}


void Screen::mouseMoveEvent(QMouseEvent *e)
{

    //当前鼠标位置
    QPoint tempPoint =  e->globalPosition().toPoint();
    Type r = pointInWhere( e->globalPosition().toPoint());
    if(e->buttons() & Qt::LeftButton)   //如果左键按下
    {
        //选取左上角的信息更改
        if(oncePress){
            movePoint = e->pos();
            int x = qAbs(movePoint.x() - pressedPoint.x());//movepoint即终点坐标
            int y = qAbs(movePoint.y() - pressedPoint.y());
            widthInfoRect.setText(tr("%1 * %2").arg(x).arg(y));   //将截图选区的长宽信息显示在widthinforect中
            if( comparePoint(pressedPoint, movePoint)) {
                widthInfoRect.setLocation(pressedPoint.x(), pressedPoint.y());
                //计算选区矩形
                rect.setX(pressedPoint.x());
                rect.setY(pressedPoint.y());
                rect.setWidth(movePoint.x() - pressedPoint.x());
                rect.setHeight(movePoint.y() - pressedPoint.y());

            } else {
                widthInfoRect.setLocation(std::max(movePoint.x(),0), movePoint.y());
                rect.setX(movePoint.x());
                rect.setY(movePoint.y());
                rect.setWidth(pressedPoint.x() - movePoint.x());
                rect.setHeight(pressedPoint.y() - movePoint.y());
            }
        }
        else
        {

            if( control ){
                control->hide();
            }//如果控制面板已经出现，则第二次以后的每一次鼠标按键都会使其暂时隐藏

            int moveX = tempPoint.x() - oldPoint.x();  //鼠标移动的x距离
            int moveY = tempPoint.y() - oldPoint.y();   //鼠标移动的y距离
            switch( type )         //选区角落四个小矩形的位置
            {
                case RECT1:               //意思是第一次选区之后，分别拖动四个角落的小矩形时候，截图选区的变化
                    pressedPoint.setY(pressedPoint.y() + moveY);  //右上角的矩形
                    movePoint.setX(movePoint.x() + moveX);
                    break;
                case RECT2:
                    pressedPoint.setX(pressedPoint.x() + moveX);    //左上角的矩形
                    pressedPoint.setY(pressedPoint.y() + moveY);
                    break;
                case RECT3:
                    pressedPoint.setX(pressedPoint.x() + moveX);  //左下角的矩形
                    movePoint.setY(movePoint.y() + moveY);
                    break;
                case RECT4:
                    movePoint.setX(movePoint.x() + moveX);     //右下角的矩形
                    movePoint.setY(movePoint.y() + moveY);
                    break;
                case RECT0:                  //指的是当鼠标在截图选区中按下左键然后拖动时候，截图选区的整体位置变化
                {
                    int tempPressX = pressedPoint.x() + moveX;
                    int tempPressY = pressedPoint.y() + moveY;
                    int tempMoveX = movePoint.x() + moveX;
                    int tempMoveY = movePoint.y() + moveY;
                    int deskWidth = pixmap.width();
                    int deskHeight = pixmap.height();
                    if( tempPressX < 0 )
                    {
                        tempPressX = 0;
                        tempMoveX = movePoint.x();
                    }
                    if( tempPressX > deskWidth)
                    {
                        tempPressX = deskHeight;
                    }
                    if(tempPressY < 0)
                    {
                        tempPressY = 0;
                        tempMoveY = movePoint.y();
                    }
                    if( tempPressY > deskHeight)
                    {

                        tempPressY = deskHeight;
                    }
                    if( tempMoveX < 0)
                    {
                        tempMoveX = 0;
                    }
                    if( tempMoveX > deskWidth)
                    {
                        tempMoveX = deskWidth;
                        tempPressX = pressedPoint.x();
                    }
                    if( tempMoveY < 0)
                    {
                        tempMoveY = 0;
                    }
                    if( tempMoveY > deskHeight)
                    {
                        tempMoveY = deskHeight;
                        tempPressY = pressedPoint.y();

                    }
                    pressedPoint.setX(tempPressX);    //鼠标在截图区域中拖动结束后，选区的位置
                    pressedPoint.setY(tempPressY);
                    movePoint.setX(tempMoveX);
                    movePoint.setY(tempMoveY);

                    int x = qAbs(movePoint.x() - pressedPoint.x());//movepoint即终点坐标
                    int y = qAbs(movePoint.y() - pressedPoint.y());
                    widthInfoRect.setText(tr("%1 * %2").arg(x).arg(y));   //将截图选区的长宽信息显示在widthinforect中
                    break;
                }
                case NO:
                    break;
                default:
                    break;
            }
            oldPoint = tempPoint;    //更新位置坐标信息
        }

        if( comparePoint(pressedPoint, movePoint)) {
            widthInfoRect.setLocation(pressedPoint.x(), pressedPoint.y());
            int x = qAbs(movePoint.x() - pressedPoint.x());//movepoint即终点坐标
            int y = qAbs(movePoint.y() - pressedPoint.y());
            widthInfoRect.setText(tr("%1 * %2").arg(x).arg(y));
            //计算选区矩形
            rect.setX(pressedPoint.x());
            rect.setY(pressedPoint.y());
            rect.setWidth(movePoint.x() - pressedPoint.x());
            rect.setHeight(movePoint.y() - pressedPoint.y());

        } else {
            widthInfoRect.setLocation(std::max(movePoint.x(),0), movePoint.y());
            int x = qAbs(movePoint.x() - pressedPoint.x());//movepoint即终点坐标
            int y = qAbs(movePoint.y() - pressedPoint.y());
            widthInfoRect.setText(tr("%1 * %2").arg(x).arg(y));
            rect.setX(movePoint.x());
            rect.setY(movePoint.y());
            rect.setWidth(pressedPoint.x() - movePoint.x());
            rect.setHeight(pressedPoint.y() - movePoint.y());
        }
        setselectimagelabel(rect);

    }
    else                          //指的是当鼠标位于四个角落小矩形中和选区中时候鼠标的图标变化
    {
        if( r == RECT1)
        {
            type = RECT1;
            setCursor(Qt::SizeBDiagCursor);
        } else if( r == RECT2)
        {
            type = RECT2;
            setCursor(Qt::SizeFDiagCursor);
        } else if( r == RECT3)
        {
            type = RECT3;
            setCursor(Qt::SizeBDiagCursor);
        } else if( r == RECT4)
        {
            type = RECT4;
            setCursor(Qt::SizeFDiagCursor);
        } else if( r == RECT0)
        {
            type = RECT0;
            setCursor(Qt::SizeAllCursor);
        } else
        {
            type = NO;
            setCursor(Qt::ArrowCursor);
        }

    }
    repaint();
}



bool Screen::compareRect(QRectF &r1, QRectF &r2)
{
    return false;
}

//计算此时鼠标移动在哪个选区上面
Type Screen::pointInWhere(QPoint p)
{

    if( pointInRect(p, rect1))
        return RECT1;
    else if( pointInRect(p, rect2))
        return RECT2;
    else if( pointInRect(p, rect3))
        return RECT3;
    else if( pointInRect(p, rect4))
        return RECT4;
    else if( pointInRect(p, rect)){

        return RECT0;}
    else
        return NO;

}

//判断点是否在矩形之内   已看懂
bool Screen::pointInRect(QPoint &p, QRectF &r)
{
    if( p.x() > r.x() && p.x() < r.x() + r.width() &&
            p.y() > r.y() && p.y() < r.y() + r.height())
        return true;
    return false;
}

void Screen::keyPressEvent(QKeyEvent *e)    //键盘事件，包括esc退出截图，回车键完成截图，返回键完成截图
{


    int key = e->key();
    switch(key)
    {
        case Qt::Key_Escape :
            //close();
            Exit();
            break;
        case Qt::Key_Enter:
            if( controlUi )
            {
                controlUi->finishBtn_slot();
            }
            break;
        case Qt::Key_Return :
            if( controlUi )
            {
                controlUi->finishBtn_slot();
            }
            break;
        default:
            break;
    }
}

//如果p1的x和y坐标都小于p2的x和y坐标返回true
bool Screen::comparePoint(QPoint &p1, QPoint &p2)      //点p1的位置坐标是否小于p2点
{
    if(p1.x() < p2.x() && p1.y() < p2.y())
        return true;
    return false;
}

void Screen::setselectimagelabel(QRectF rect)
{

    float scale = getSacle();
    int wid = scale *rect.width() + 5 ;
    int hei = scale * rect.height()+5;
    int x = scale * rect.x();
    int y = scale * rect.y();
    QImage selectimage = pixmap.copy(x,y,wid,hei).toImage();
    labelimage->setimagetolabel(selectimage);
    labelimage->setFixedSize(rect.width(),rect.height());
    labelimage->move(QPoint(rect.x(), rect.y()));
}

float Screen::getSacle()
{

    float rate = 0;
    QList<QScreen*> screens = QApplication::screens();
    if(screens.size() > 0){
        int desktopWidth = screens[0]->availableGeometry().width();
        rate = float(pixmap.width()) /  desktopWidth;
        return rate;
    }
    return 1.0;
}

//保存截取出来的图片
void Screen::savePixmap()
{

    //生成图片名称
    QString picName;
    QTime time;
    //获取当前系统时间，用做伪随机数的种子
    time = QTime::currentTime();
    srand(time.msec() + time.second() * 1000);
    //随机字符串
    QString randStr;
    randStr.setNum(rand());
    picName.append(randStr);
    QString filename=QFileDialog::getSaveFileName(this,QStringLiteral("保存截图"),picName,"JPEG Files(*.jpg)");
    if(filename.length()>0){
        QImage pimage=labelimage->resultimage();
        pimage.save(filename, "jpg");
    }
}

QPixmap Screen::getGrabPixmap()   //返回截到的图片
{
    return pixmap.copy(pressedPoint.x(), pressedPoint.y(), movePoint.x() - pressedPoint.x(),
                       movePoint.y() - pressedPoint.y());   //这个地方是关键，可以从这里入手了，得到了pixmap之后，进行toimage，然后进行编辑操作
}

void Screen::drawline()
{
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->setdrawlineenable();
}

void Screen::textedit()
{
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->settexteditenable();
}

void Screen::drawarrow()
{
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->setdrawarrowenable();
}

void Screen::returnEdit() {
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->setReturnEditEnable();
}

void Screen::drawround()
{
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->setroundenable();
}

void Screen::drawrectang()
{
    labelimage->show();
    ocrTextEdit->hide();
    labelimage->setrectangleenable();
}

void Screen::OCR()
{
    ocrTextEdit->clear();
    if(!labelimage->getOCRStatus()){
        QImage res = labelimage->resultimage();
        labelimage->ocr(labelimage->qim2mat(res));
    }

    for(auto text:labelimage->getOcrTexts()){

        ocrTextEdit->insertPlainText(text);
        ocrTextEdit->moveCursor(QTextCursor::Down);
    }
    //设置控制面板的位置
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height =  rect.height();
    ocrTextEdit->setGeometry(x+width, y, 200, height);
    ocrTextEdit->show();
    //labelimage->show();
    labelimage->setocrenable();

}

void Screen::Exit()
{

//    hide();
    close();
    if(labelimage)
    {
        labelimage->close();
    }
    if(control){
        control->close();
    }
    beingSnap *b = new beingSnap();
    b->show();
}

Screen::~Screen()
{
    // delete control;
}


bool Screen::eventFilter(QObject *obj, QEvent *event)
{

    if (obj == labelimage)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            keyPressEvent(keyEvent);

            qDebug() << "you press" << keyEvent->key();
            //事件不再进行传播，拦截
            return true;
        }
//        else if (event->type() == QEvent::MouseMove)
//        {

//            if(!labelimage->getDrawStatus()){

//                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
//                mouseMoveEvent(mouseEvent);
//                return true;
//            }

//        }
        else
        {
            return false;//继续传播
        }
    }
    else
    {
        //当不确定是否继续传播时，按照父类的方法来处理
        //即调用父类的evenFilter函数
        return QWidget::eventFilter(obj, event);
    }
}
