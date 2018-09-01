#include "gen_eolico.h"
#include "ui_gen_eolico.h"
#include <QMessageBox>
#include <QtCore>
#include <QtGui>
#include <QString>
#include <QTimer>

Gen_Eolico::Gen_Eolico(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Gen_Eolico)
{
    ui->setupUi(this);

    time_2sec = new QTimer();

//COMBO BOX MODO
    ui->combo_modo->addItem("Velocidad (0)");
    ui->combo_modo->addItem("Potencia (1)");
    ui->combo_modo->addItem("Torque (2)");
    ui->combo_modo->addItem("Potencia (3)");
    actual_mode=ui->combo_modo->currentIndex();

    connect(ui->combo_modo,SIGNAL(activated(int)),this,SLOT(mode_changed()));

//LINES EDIT
    ui->lineEdit_iconv->setInputMask("99999");         //para insertar solo numeros
    ui->lineEdit_isd_ref->setInputMask("99999");
    ui->lineEdit_lim_ibat->setInputMask("99999");
    ui->lineEdit_lim_ief->setInputMask("99999");
    ui->lineEdit_lim_vdc->setInputMask("99999");
    ui->lineEdit_pot_ref->setInputMask("99999");
    ui->lineEdit_speed_ref->setInputMask("99999");
    ui->lineEdit_torque_ref->setInputMask("99999");
    ui->lineEdit_vdc->setInputMask("99999");
    ui->lineEdit_ibat->setInputMask("99999");
    new_mode();

//TIMER
    connect(time_2sec, SIGNAL(timeout()), this, SLOT(timer_handler()));
    time_2sec->start(2000);
}

Gen_Eolico::~Gen_Eolico()
{
    delete ui;
}

void Gen_Eolico::mode_changed(){
    QMessageBox::StandardButton reply;
    QString str="Esta seguro que desea cambiar el modo a: ";
    str.append(ui->combo_modo->currentText());
    reply = QMessageBox::question(this,"Cambiar Modo",str, QMessageBox::Yes | QMessageBox::No );

    if(reply==QMessageBox::Yes){
    actual_mode = ui->combo_modo->currentIndex();
    new_mode();
    }
    else{
    ui->combo_modo->setCurrentIndex(actual_mode);
    }
}

void Gen_Eolico::new_mode(){
    switch (actual_mode) {
    case 0:     //Velocidad
        ui->lineEdit_iconv->setDisabled(true);
        ui->lineEdit_isd_ref->setDisabled(true);
        ui->lineEdit_lim_ibat->setDisabled(true);
        ui->lineEdit_lim_ief->setDisabled(true);
        ui->lineEdit_lim_vdc->setDisabled(true);
        ui->lineEdit_pot_ref->setDisabled(true);
        ui->lineEdit_speed_ref->setEnabled(true);
        ui->lineEdit_torque_ref->setDisabled(true);
        ui->lineEdit_vdc->setDisabled(true);
        ui->lineEdit_ibat->setDisabled(true);
        break;
    case 1:     //Potencia
        ui->lineEdit_iconv->setDisabled(true);
        ui->lineEdit_isd_ref->setDisabled(true);
        ui->lineEdit_lim_ibat->setDisabled(true);
        ui->lineEdit_lim_ief->setDisabled(true);
        ui->lineEdit_lim_vdc->setDisabled(true);
        ui->lineEdit_pot_ref->setEnabled(true);
        ui->lineEdit_speed_ref->setDisabled(true);
        ui->lineEdit_torque_ref->setDisabled(true);
        ui->lineEdit_vdc->setDisabled(true);
        ui->lineEdit_ibat->setDisabled(true);
        break;
    case 2:     //Torque
        ui->lineEdit_iconv->setDisabled(true);
        ui->lineEdit_isd_ref->setDisabled(true);
        ui->lineEdit_lim_ibat->setDisabled(true);
        ui->lineEdit_lim_ief->setDisabled(true);
        ui->lineEdit_lim_vdc->setDisabled(true);
        ui->lineEdit_pot_ref->setDisabled(true);
        ui->lineEdit_speed_ref->setDisabled(true);
        ui->lineEdit_torque_ref->setEnabled(true);
        ui->lineEdit_vdc->setDisabled(true);
        ui->lineEdit_ibat->setDisabled(true);
        break;
    case 3:     //Potencia
        ui->lineEdit_iconv->setDisabled(true);
        ui->lineEdit_isd_ref->setDisabled(true);
        ui->lineEdit_lim_ibat->setDisabled(true);
        ui->lineEdit_lim_ief->setDisabled(true);
        ui->lineEdit_lim_vdc->setDisabled(true);
        ui->lineEdit_pot_ref->setEnabled(true);
        ui->lineEdit_speed_ref->setDisabled(true);
        ui->lineEdit_torque_ref->setDisabled(true);
        ui->lineEdit_vdc->setDisabled(true);
        ui->lineEdit_ibat->setDisabled(true);
        break;
    default:
        break;
    }

}

void Gen_Eolico::timer_handler(){
    refresh_values();
}

void Gen_Eolico::refresh_values(){
    //DEPENDE DEL MODO SE VAN A MOSTRAR LOS VALORES (EN GENERAL SE MUESTRAN LOS MISMOS QUE ARRIBA ESTAN DISABLE)
    //en cada uno hay que poner la LACAN_VARIABLE que corresponda
    switch (actual_mode) {
    case 0:     //Velocidad
        ui->lineEdit_iconv->setText("123");
        ui->lineEdit_isd_ref->setText("123");
        ui->lineEdit_lim_ibat->setText("123");
        ui->lineEdit_lim_ief->setText("123");
        ui->lineEdit_lim_vdc->setText("123");
        ui->lineEdit_pot_ref->setText("123");
        ui->lineEdit_torque_ref->setText("123");
        ui->lineEdit_vdc->setText("123");
        ui->lineEdit_ibat->setText("123");
        break;
    case 1:     //Potencia
        ui->lineEdit_iconv->setText("123");
        ui->lineEdit_isd_ref->setText("123");
        ui->lineEdit_lim_ibat->setText("123");
        ui->lineEdit_lim_ief->setText("123");
        ui->lineEdit_lim_vdc->setText("123");
        ui->lineEdit_speed_ref->setText("123");
        ui->lineEdit_torque_ref->setText("123");
        ui->lineEdit_vdc->setText("123");
        ui->lineEdit_ibat->setText("123");
        break;
    case 2:     //Torque
        ui->lineEdit_iconv->setText("123");
        ui->lineEdit_isd_ref->setText("123");
        ui->lineEdit_lim_ibat->setText("123");
        ui->lineEdit_lim_ief->setText("123");
        ui->lineEdit_lim_vdc->setText("123");
        ui->lineEdit_pot_ref->setText("123");
        ui->lineEdit_speed_ref->setText("123");
        ui->lineEdit_vdc->setText("123");
        ui->lineEdit_ibat->setText("123");
        break;
    case 3:     //Potencia
        ui->lineEdit_iconv->setText("123");
        ui->lineEdit_isd_ref->setText("123");
        ui->lineEdit_lim_ibat->setText("123");
        ui->lineEdit_lim_ief->setText("123");
        ui->lineEdit_lim_vdc->setText("123");
        ui->lineEdit_speed_ref->setText("123");
        ui->lineEdit_torque_ref->setText("123");
        ui->lineEdit_vdc->setText("123");
        ui->lineEdit_ibat->setText("123");
        break;
    default:
        break;
    }
}
