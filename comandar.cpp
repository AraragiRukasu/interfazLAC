#include "comandar.h"
#include "ui_comandar.h"
#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include "PC.h"
#include <QWidget>
#include <QMap>
#include "assert.h"

Comandar::Comandar(QWidget *parent, uint16_t destEx) :
    QDialog(parent),
    ui(new Ui::Comandar)
{
    ui->setupUi(this);

    this->setWindowTitle("Comandar");

    mw = qobject_cast<MainWindow*>(this->parent());
    dest=destEx;

    switch(dest){
        case LACAN_ID_GEN:
            ui->label_DESTINO->setText("Generador Eolico");

            varmap = mw->varmap_gen;

            ui->list_VARIABLE->addItem("Potencia de Salida");
            ui->list_VARIABLE->addItem("Velocidad Angular");
            ui->list_VARIABLE->addItem("Torque");
            ui->list_VARIABLE->addItem("Tension de Salida");
            ui->list_VARIABLE->addItem("Corriente de ISD");
            ui->list_VARIABLE->addItem("Corriente Eficaz");
            ui->list_VARIABLE->addItem("Corriente de Bateria");
            ui->list_VARIABLE->addItem("Modo");

            ui->list_MOD_SET->addItem("Velocidad",QVariant(LACAN_VAR_MOD_VEL));
            ui->list_MOD_SET->addItem("Potencia",QVariant(LACAN_VAR_MOD_POT));
            ui->list_MOD_SET->addItem("Torque",QVariant(LACAN_VAR_MOD_TORQ));
            ui->list_MOD_SET->addItem("MPPT",QVariant(LACAN_VAR_MOD_MPPT));
            ui->list_MOD_SET->setDisabled(true);
            mode_set = LACAN_VAR_MOD_VEL;   //inicializo con el primero

            ui->list_COMANDO->addItem("Start",QVariant(LACAN_CMD_START));
            ui->list_COMANDO->addItem("Stop",QVariant(LACAN_CMD_STOP));
            ui->list_COMANDO->addItem("Reset",QVariant(LACAN_CMD_RESET));
            ui->list_COMANDO->addItem("Enable",QVariant(LACAN_CMD_ENABLE));
            ui->list_COMANDO->addItem("Disable",QVariant(LACAN_CMD_DISABLE));
            cmd = LACAN_CMD_START;  //inicializo con el primero
        break;
        case LACAN_ID_VOLANTE:
            ui->label_DESTINO->setText("Volante de Inercia");

            varmap = mw->varmap_vol;

            ui->list_VARIABLE->addItem("Corriente de ID");
            ui->list_VARIABLE->addItem("Velocidad angular Standby");
            ui->list_COMANDO->addItem("Start",QVariant(LACAN_CMD_START));
            ui->list_COMANDO->addItem("Shutdown",QVariant(LACAN_CMD_SHUTDOWN));
            ui->list_COMANDO->addItem("Stop",QVariant(LACAN_CMD_STOP));
            ui->list_COMANDO->addItem("Reset",QVariant(LACAN_CMD_RESET));
            ui->list_COMANDO->addItem("Enable",QVariant(LACAN_CMD_ENABLE));
            ui->list_COMANDO->addItem("Disable",QVariant(LACAN_CMD_DISABLE));
            cmd = LACAN_CMD_START;

        break;
        default:
        break;
    }

    SET_selected();//por defecto arranca seleccionado el SET
    connect(ui->radio_DO,SIGNAL(clicked(bool)),this,SLOT(DO_selected()));
    connect(ui->radio_SET,SIGNAL(clicked(bool)),this,SLOT(SET_selected()));

    connect(ui->list_VARIABLE,SIGNAL(currentTextChanged(QString)),this,SLOT(SET_VAR_Changed()));
}

void Comandar::on_button_ENVIAR_clicked()
{
    uint prevsize= mw->msg_ack.size();

    if(ui->radio_DO->isChecked()){
        mw->LACAN_Do(cmd,1,dest);
        mw->agregar_log_sent();
    }else if(ui->radio_SET->isChecked()){
        data_can data;
        //Si es un SET hay que revisar si la variable es el modo, debido a que es la unica que no tiene un valor
        //numerico que complete la orden, sino que se debe tomar la opcion de a que modo se quiere cambiar
        if (ui->list_VARIABLE->currentText() == "Modo"){
            data.var_char[0]=uchar(mode_set);
            data.var_char[1]=0;
            data.var_char[2]=0;
            data.var_char[3]=0;
            mw->LACAN_Set(var_set,data,1,dest);
            mw->agregar_log_sent();
        }
        else{
            if(ui->spin_valor->value()>minimo){
                data.var_float=float(ui->spin_valor->value()); //si esta seleccionado algo que no sea modo, manda el valor de spin
                mw->LACAN_Set(var_set,data,1,dest);
                mw->agregar_log_sent();
            }
            else{ // En caso de que se quiera mandar algo que no cumple con el requisito minimo para la variable
                  //se ignora el valor ingresado y se le avisa al usuario que esta por mandar el valor minimo
                  //si el valor del spin no cumple con el minimo, pasa a tener su valor minimo cuando pierde el focus
                QMessageBox::StandardButton reply;
                QString str = "El valor minimo para esta variable es ";
                str.append(QString::number(minimo));
                str.append(". Confirma que desea enviar este valor?");
                reply = QMessageBox::question(this,"Valor Minimo",str, QMessageBox::Yes | QMessageBox::No );
                if(reply==QMessageBox::Yes){
                    data.var_float=float(ui->spin_valor->value()); //si esta seleccionado algo que no sea modo, manda el valor de spin
                    mw->LACAN_Set(var_set,data,1,dest);
                    mw->agregar_log_sent();
                }
            }
        }
    }else{
        QMessageBox::warning(this,"Ups... Algo salio mal","Ninguna de las dos opciones seleccionadas");
    }

    if(mw->msg_ack.size()>prevsize){ //si hay un mensaje que nuevo que necesita ack, se conecta su timer con un slot de verificacion
        assert(mw->msg_ack.back());
        connect(&(mw->msg_ack.back()->ack_timer),SIGNAL(timeout()), mw, SLOT(verificarACK()));
    }

    this->close();
}

void Comandar::DO_selected(){
    ui->list_VARIABLE->setDisabled(true);
    ui->spin_valor->setDisabled(true);
    ui->list_MOD_SET->setDisabled(true);
    ui->list_COMANDO->setEnabled(true);
}

void Comandar::SET_selected(){
    ui->list_COMANDO->setDisabled(true);
    ui->list_VARIABLE->setEnabled(true);
    SET_VAR_Changed();
}

void Comandar::SET_VAR_Changed(){               //habilita y deshabilita los campos dependiendo si es una variable o un modo
    if (ui->list_VARIABLE->currentText() == "Modo"){
        ui->list_MOD_SET->setEnabled(true);
        ui->spin_valor->setDisabled(true);
        var_set=LACAN_VAR_MOD;}
    else{
        ui->list_MOD_SET->setDisabled(true);
        ui->spin_valor->setEnabled(true);
        SET_ACTUAL_VAR();}
}

void Comandar::SET_ACTUAL_VAR(){ //actualiza la variable seleccionada en el momento
   QString var_selectedstr;
   var_selectedstr=ui->list_VARIABLE->currentText();
   var_set = varmap[var_selectedstr].setp;
   ui->spin_valor->setMaximum(varmap[var_selectedstr].max);
   ui->spin_valor->setMinimum(varmap[var_selectedstr].min);
   minimo=varmap[var_selectedstr].min;
}


void Comandar::on_list_MOD_SET_currentIndexChanged(int index)
{
    mode_set = uint16_t(ui->list_MOD_SET->itemData(index).toInt());
}

void Comandar::on_list_COMANDO_currentIndexChanged(int index)
{
    cmd = uint16_t(ui->list_COMANDO->itemData(index).toInt());
}

void Comandar::closeEvent(QCloseEvent *e){
    emit comWindowsClosed();
    QDialog::closeEvent(e);
}

Comandar::~Comandar()
{
    delete ui;
}
