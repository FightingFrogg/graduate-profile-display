#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>
#include <QFile>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    ui->radioButton_image->setChecked(true);

}


void Widget::readData(){
    QString path = QFileDialog::getOpenFileName(this, "open", "./");
    if (false == path.isEmpty()){
        char *path_char = nullptr;
        FILE *fi;
        QByteArray ba = path.toLatin1();
        path_char = ba.data();              // 路径转换成 char*
        fi = fopen(path_char, "rb");

        row = ui->text_row->text().toInt();
        column = ui->text_column->text().toInt();
        float data[row][column];  
        fread(data, sizeof (float), row*column, fi);  // 读取到 data 数组中

        if (ui->radioButton_image->isChecked()){
            image(*data, row, column);
        }
        else if (ui->radioButton_wigb->isChecked()) {
            wigb(*data, row, column);
        }

        fclose(fi);
    }
}


void Widget::wigb(float *data, int row, int column){
    ui->customplot->clearPlottables();
    ui->customplot->xAxis2->setVisible(true);
    ui->customplot->xAxis->setVisible(false);
    ui->customplot->yAxis->setRangeReversed(true);
    ui->customplot->xAxis2->setRange(-5, row);
    ui->customplot->yAxis->setRange(0, column);

    QVector<QVector<double>> vecList;
    QVector<double> vecY;
    advG(data, row, column);  //全局均一化

    for (int i=0; i<row; i++) {
        QVector<double> vec;
//        float max = adv(data, i, column);  // 找单道最大值，做均一化

        for (int j=0; j<column; j++) {
//            vec.append((double) (*(data+i*column+j)/max) + (double) i);  // 单道均一化，设置 x 轴数据，均一化
            vec.append((double) (*(data+i*column+j)) + (double) i);  // 全局均一化
        }
        vecList.append(vec);
    }

    for (int i=0; i<column; i++) {  // 设置 y 轴数据
        vecY.append((double) i);
    }

    // 设置第一个点和最后一个点赋值为 0
    for (int i=0; i<row; i++) {
        vecList[i][0] = (double) i;
        vecList[i][column-1] = (double) i;
    }

    for (int i=0; i<row; i++) {  // 显示曲线
        QCPCurve *curve = new QCPCurve(ui->customplot->xAxis2, ui->customplot->yAxis);
        curve->setData(vecList[i], vecY);
        curve->setPen(QPen(Qt::black));
    }

    QVector<QCPCurve *> curveList;
    for (int i=0; i<row; i++) {         // 创建阴影 curve 指针，在 shadow() 函数外定义，提高效率，防止程序阻塞。
        QCPCurve *curve = new QCPCurve(ui->customplot->xAxis2, ui->customplot->yAxis);
        curveList.append((curve));
    }

    shadow(vecList, row, column, curveList);  // 正值涂黑

    ui->customplot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
//    ui->customplot->setSelectionRectMode(QCP::srmZoom);
    ui->customplot->replot();
}



void Widget::shadow(QVector<QVector<double>> vecList, int row, int column, QVector<QCPCurve *> curveList){

    /*   第一个点和最后一个点赋值为 0
     * 1.找到第一个大于 0 的点，确定 0 的位置，返回列值
     * 2.判断下一个点是否大于 0
     * 3.大于等于 0 回到 1
     * 4.否则，确定 0 的位置
     * 5.涂黑
    */

    // 绘制阴影
    for (int i=0; i<row; i++) {
        double fR = 0.0, fC = 0.0;
        double *fRow = &fR, *fColumn = &fC;

        QVector<double> x, y;
        while ( *fColumn < column ) {
            if ( vecList[i][*fColumn] > i ) {  // 找到第一个大于 0 的点

                *fRow = vecList[i][*fColumn];

                double yy = vecList[i][*fColumn-1.0];   // fRow --> y
                double xx = *fColumn-1.0;
                double k = (*fRow - yy)/(*fColumn - xx);
                double zeroX = (i-(*fRow - k*(*fColumn))) / k;
                x.append(zeroX);
                y.append(i);
                x.append(*fColumn);
                y.append(*fRow);

                *fColumn = *fColumn + 1;
                while (*fColumn < column) {  // 找其他大于 0 的点

                    if (vecList[i][*fColumn] > i) {
                        *fRow = vecList[i][*fColumn];
                        x.append(*fColumn);
                        y.append(*fRow);
                        *fColumn = *fColumn + 1;
                    }
                    else {
                        break;
                    }

                }
                *fColumn = *fColumn - 1;
                if (*fColumn < column) {
                    double yy = vecList[i][*fColumn + 1];   // fRow --> y
                    double xx = *fColumn + 1;
                    double k = (*fRow - yy)/(*fColumn - xx);
                    double zeroX = (i-(*fRow - k*(*fColumn))) / k;
                    x.append(zeroX);
                    y.append(i);
                }

                curveList[i]->setData(y, x);
                curveList[i]->setPen(QPen(Qt::transparent));
                curveList[i]->setBrush(QBrush(QColor(0,0,0)));

                *fColumn = *fColumn + 1;

            }
            else {
                *fColumn = *fColumn + 1;
            }

        }

    }

}


float Widget::adv(float *data, int ii, int column){
    float max = 0.0f;
    for (int i=0; i<column; i++) {
        if( abs(*(data+ii*column+i)) > max ){  // 取绝对值
            max =abs(*(data+ii*column+i));
        }
    }

    return max;
}


void Widget::advG(float *data, int row, int column){
    float max = 0.0f, min = 0.0f;
    for (int i=0; i<row; i++) {             // 找到正向和负向各自的最大值
        for (int j=0; j<column; j++) {
            if( *(data+i*column+j) > 0.0 ){
                if ( *(data+i*column+j) > max ){
                    max = *(data+i*column+j);
                }
            }
            else {
                if ( *(data+i*column+j) < min ){
                    min = *(data+i*column+j);
                }
            }
        }
    }

    if( max == 0.0 || min == 0.0){
        qDebug() << "被除数为0";
    }
    else {
        for (int i=0; i<row; i++) {             // 均一化
            for (int j=0; j<column; j++) {
                if( *(data+i*column+j) > 0.0 ){
                    *(data+i*column+j) /= max;
                }
                else {
                    *(data+i*column+j) /= abs(min);
                }
            }
        }
    }

}


void Widget::image(float *data, int row, int column){  // 通过手动寻址的方式，传递二维数组

    // set up the colorMap
    ui->customplot->clearPlottables();  // 清屏
    ui->customplot->xAxis2->setVisible(true);
    ui->customplot->xAxis->setVisible(false);        // 下面的横坐标不可见
    ui->customplot->yAxis->setRangeReversed(true);   // 设置坐标轴反转
    advG(data, row, column);  //全局均一化

    QCPColorMap *colorMap = new QCPColorMap(ui->customplot->xAxis2, ui->customplot->yAxis); // 创建 colorMap plottable
    colorMap->data()->setSize(row, column);           // have nx*ny data points
    colorMap->data()->setRange(QCPRange(0,row), QCPRange(column,0));
    double z;
    for (int i=0; i<row; ++i) {
//        float max = adv(data, i, column);  // 找单道最大值，做均一化
        for (int j=0; j<column; ++j) {
            z = *(data + i*column + j);
            colorMap->data()->setCell(i, j, z);  // 设置数据
        }
    }

//    QCPColorScale *colorScale = new QCPColorScale(ui->customplot);  // 设置图例
//    ui->customplot->plotLayout()->addElement(0,1,colorScale);
//    colorScale->setType(QCPAxis::atRight);
//    colorMap->setColorScale(colorScale);
//    colorScale->axis()->setLabel("image");

    QCPColorGradient *colorGradient = new QCPColorGradient();  // 创建颜色渐变
    colorGradient->clearColorStops();
    colorGradient->setColorStopAt(1, QColor(191,0,0));          // 设置颜色渐变 红色
    colorGradient->setColorStopAt(0.5, QColor(225,225,225));    // 白色
    colorGradient->setColorStopAt(0, QColor(0,0,64));           // 蓝色
//    colorGradient->setLevelCount(100);
    colorMap->setGradient(*colorGradient);

    colorMap->rescaleDataRange();
    QCPMarginGroup *marginGroup = new QCPMarginGroup(ui->customplot);
    ui->customplot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
//    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);  // 图例位置

    ui->customplot->rescaleAxes();
    ui->customplot->replot();

}



Widget::~Widget()
{
    delete ui;
}


void Widget::on_button_open_clicked()
{
    readData();

}


