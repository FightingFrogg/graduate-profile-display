#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void readData();
    void image(float *data, int row, int column);
    void wigb(float *data, int row, int column);
    float adv(float *data, int ii, int column);  // 找单道最大值，做均一化
    void advG(float *data, int row, int column); // 全局均一化
    void shadow(QVector<QVector<double>> vecList, int row, int column, QVector<QCPCurve *> curveLlist);

    QVector<QVector<float>> vecList;
    int row;
    int column;


private slots:
    void on_button_open_clicked();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
