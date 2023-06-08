#ifndef QSLABEL_H
#define QSLABEL_H
#include<QLabel>
#include <QWidget>
#include<QTextEdit>
#include<QPaintEvent>
#include<QMouseEvent>
#include<QRgb>
#include <QGuiApplication>
#include "ocr/text_angle_cls.h"
#include "ocr/text_det.h"
#include "ocr/text_rec.h"
//#include "include/paddleocr.h"
//#include "include/paddlestructure.h"
//using namespace PaddleOCR;
class Screen;

typedef struct myLine{
    QPoint startPoint;
    QPoint endPoint;
}myLine;
typedef struct myRectangle{
    QPoint startPoint;
    QPoint endPoint;
}myRectangle;

typedef struct myRound{
    QPoint startPoint;
    QPoint endPoint;
}myRound;
typedef struct myArrow{
    QPoint startPoint;
    QPoint endPoint;
}myArrow;
typedef struct myText{
    QString mText;
    QRect mRect;

}myText;


struct OCRPredictResult {

  vector<cv::Point2f> box;
  //std::vector<std::vector<int>> box;
  std::string text;
};



class QSLabel : public QLabel
{
    Q_OBJECT
public:
    QSLabel(QWidget *parent);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void showEvent(QShowEvent *event);
    void paintEvent(QPaintEvent *event);
    void setdrawlineenable();
    void setrectangleenable();
    void setocrenable();
    void setdrawarrowenable();
    void setroundenable();
    void settexteditenable();
    void settextedittovector();
    void drawarrow(QPoint startpoint,QPoint endpoint,QPainter &p);
    void setimagetolabel(const QImage &image);
    void setReturnEditEnable();
    bool getDrawStatus();
    bool getOCRStatus();
    void keyPressEvent(QKeyEvent *e);
    QImage resultimage();
    void ocr(cv::Mat inputMat);
    QList<vector<cv::Point2f>> getOcrBoxes();
    vector<OCRPredictResult> getOcrResults();
    QList<QString> getOcrTexts();


    cv::Mat qim2mat(QImage &image)
    {

        image = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat = cv::Mat(image.height(),
                      image.width(),
                      CV_8UC(3),
                      image.bits(),
                      image.bytesPerLine());
        cvtColor(mat,mat,cv::COLOR_RGB2BGR);
        return mat;
    }

    QImage cvMat2QImage(const cv::Mat &mat)
    {
        // 8-bits unsigned, NO. OF CHANNELS = 1
        if(mat.type() == CV_8UC1)
        {
            QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
            image.setColorCount(256);
            for(int i = 0; i < 256; i++)
            {
                image.setColor(i, qRgb(i, i, i));
            }
            uchar *pSrc = mat.data;
            for(int row = 0; row < mat.rows; row ++)
            {
                uchar *pDest = image.scanLine(row);
                memcpy(pDest, pSrc, mat.cols);
                pSrc += mat.step;
            }
            return image;
        }
        // 8-bits unsigned, NO. OF CHANNELS = 3
        else if(mat.type() == CV_8UC3)
        {
            // Copy input Mat
            cv::cvtColor(mat,mat,cv::COLOR_BGR2RGB);
            const uchar *pSrc = (const uchar*)mat.data;
            QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
            //image = image.rgbSwapped();
            return image.copy();
        }
        else if(mat.type() == CV_8UC4)
        {
            // Copy input Mat
            const uchar *pSrc = (const uchar*)mat.data;
            QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
            return image.copy();
        }
        else
        {
            qDebug() << "ERROR: Mat could not be converted to QImage.";
            return QImage();
        }
    }
    void sorted_boxes(std::vector<OCRPredictResult> &ocr_result) {
      std::sort(ocr_result.begin(), ocr_result.end(), comparison_box);

      for (int i = 0; i < ocr_result.size() - 1; i++) {
        if (abs(ocr_result[i + 1].box[0].y - ocr_result[i].box[0].y) < 10 &&
            (ocr_result[i + 1].box[0].x < ocr_result[i].box[0].x)) {
          std::swap(ocr_result[i], ocr_result[i + 1]);
        }
      }
    }


public slots:
    void ontextchanged();
private:
    QPoint startPoint;
    QPoint endPoint;
    QVector<myLine*> lines;
    QVector<myRectangle*> rectangles;
    QVector<myRound*> rounds;
    QVector<myText*> texts;
    QVector<myArrow*> arrows;
    QVector<QString> actionVec;
    QString text;
    QImage selectimage;
    QTextEdit *m_plaintextedit;
    bool ispressed;
    bool isdrawline;
    bool isdrawrectangle;
    bool isdrawround;
    bool istextedit;
    bool isdrawarrow;
    bool isreturnedit;
    bool isOcr;
    bool ocrDone = false;
    Screen *screen;
    TextDetector *detect_model;
    TextClassifier *angle_model;
    TextRecognizer *rec_model;
    QList<vector<cv::Point2f>> ocrBoxList;
    QList<QString> ocrTextList;
    vector<OCRPredictResult> ocr_results;

    static bool comparison_box(const OCRPredictResult &result1,
                               const OCRPredictResult &result2) {
      if (result1.box[0].y < result2.box[0].y) {
        return true;
      } else if (result1.box[0].y == result2.box[0].y) {
        return result1.box[0].x < result2.box[0].x;
      } else {
        return false;
      }
    }
};

#endif // QSLABEL_H
