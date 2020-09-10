#include "dialog_dpv.h"
#include "ui_dialog_dpv.h"

Dialog_DPV::Dialog_DPV(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_DPV)
{
    ui->setupUi(this);
}

Dialog_DPV::~Dialog_DPV()
{
    delete ui;
}
