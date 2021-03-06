#include "gen_eolico.h"
#include "ui_gen_eolico.h"
#include <QMessageBox>
#include <QtCore>
#include <QtGui>
#include <QTimer>
#include "PC.h"
#include "lacan_limits_gen.h"
#include "assert.h"

Gen_Eolico::Gen_Eolico(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Gen_Eolico)
{
    ui->setupUi(this);
    mw = qobject_cast<MainWindow*>(this->parent());

    this->setWindowTitle("Generador Eolico");
    this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    send_queries = true;

//Configuracion del CombBox para los Modos
    ui->combo_modo->addItem("Velocidad (0)",QVariant(LACAN_VAR_MOD_VEL));
    ui->combo_modo->addItem("MPPT (1)",QVariant(LACAN_VAR_MOD_MPPT));
    ui->combo_modo->addItem("Torque (2)",QVariant(LACAN_VAR_MOD_TORQ));
    ui->combo_modo->addItem("Potencia (3)",QVariant(LACAN_VAR_MOD_POT));
    connect(ui->combo_modo,SIGNAL(activated(int)),this,SLOT(verificar_mode_changed()));
    on_combo_modo_currentIndexChanged(0);

//Inicializacion de Labels
    ui->label_gen_io->setText("----");
    ui->label_gen_vo->setText("----");
    ui->label_gen_ibat->setText("----");
    ui->label_gen_po->setText("----");
    ui->label_gen_vel->setText("----");
    ui->label_gen_tor->setText("----");

    blockAllSpinSignals(true);

    ui->spin_gen_isd_ref->setMinimum(LACAN_VAR_GEN_ISD_MIN);
    ui->spin_gen_lim_ibat_ref->setMinimum(LACAN_VAR_GEN_IBAT_MIN);
    ui->spin_gen_lim_ief_ref->setMinimum(LACAN_VAR_GEN_IEF_MIN);
    ui->spin_gen_lim_vdc_ref->setMinimum(LACAN_VAR_GEN_VO_MIN);
    ui->spin_gen_pot_ref->setMinimum(LACAN_VAR_GEN_PO_MIN);
    ui->spin_gen_speed_ref->setMinimum(LACAN_VAR_GEN_W_MIN);
    ui->spin_gen_torque_ref->setMinimum(LACAN_VAR_GEN_TORQ_MIN);

    ui->spin_gen_isd_ref->setMaximum(LACAN_VAR_GEN_ISD_MAX);
    ui->spin_gen_lim_ibat_ref->setMaximum(LACAN_VAR_GEN_IBAT_MAX);
    ui->spin_gen_lim_ief_ref->setMaximum(LACAN_VAR_GEN_IEF_MAX);
    ui->spin_gen_lim_vdc_ref->setMaximum(LACAN_VAR_GEN_VO_MAX);
    ui->spin_gen_pot_ref->setMaximum(LACAN_VAR_GEN_PO_MAX);
    ui->spin_gen_speed_ref->setMaximum(LACAN_VAR_GEN_W_MAX);
    ui->spin_gen_torque_ref->setMaximum(LACAN_VAR_GEN_TORQ_MAX);

//TIMER ENCARGADO DE REFRESCAR LOS VALORES Y DE ENVIAR LAS NUEVAS CONSULTAS
    time_2sec = new QTimer();
    connect(time_2sec, SIGNAL(timeout()), this, SLOT(timer_handler()));
    time_2sec->start(2000); //velocidad de refresco (en ms)

    //envio las primeras consultas
    send_qry_references();
    send_qry_variables();
    referenceChanged = false;

    editHotKey = new QShortcut(QKeySequence(tr("Ctrl+E", "Edit")), this);
    connect(editHotKey, SIGNAL(activated()), this, SLOT(changeEditState()));

    ui->label_edit->setDisabled(true);

//INICIALIZAR ICONO DEL BOTON STOP
    QPixmap pixmap(":/Imagenes/stop_normal.png");
    QIcon ButtonIcon(pixmap);
    ui->pushButton_stop->setIcon(ButtonIcon);
}

Gen_Eolico::~Gen_Eolico()
{
    delete ui;
    disconnect(time_2sec, SIGNAL(timeout()), this, SLOT(timer_handler()));
    delete time_2sec;
    disconnect(editHotKey, SIGNAL(activated()), this, SLOT(changeEditState()));
    delete editHotKey;
}

void Gen_Eolico::timer_handler(){
    static uint count = 0;

    if(mw->device_is_connected(LACAN_ID_GEN)){
        if(send_queries){
            ui->pushButton_start->blockSignals(false);  //lamentablemente la unica solucion que le encontramos al triggereo erroneo de la
                                                        //señal clicked del boton start fue bloquearla al utilizar el modo editable pero
                                                        //volver a habilitarla luego de un tiempo
            refresh_values();       //actualiza los valores de la pantalla
            count++;
            send_qry_variables();
            if(count%5==0||referenceChanged){//si pasan 10seg o cambio alguna referencia (desde la UI), consulto las referencias al dispositivo
                send_qry_references();
                referenceChanged = false;
                count = 0;
            }
        }
    }
    else{   //si no esta conectado, se cierra la pantalla
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(this,"Conexion perdida","El generador se ha desconectado de la red. Esta ventana se cerrara inmediatamente");
        if(reply){
            this->close();
        }
    }
}

//Me fijo que variable es la que llego, y le asigno el valor correspondiente, a la variable propia de la clase
//Esta funcion recibe los post que les pasa Estado de red
void Gen_Eolico::GENpost_Handler(LACAN_MSG msg){

    int actual_mode_index = -1;

    recibed_val.var_char[0]=msg.BYTE2;
    recibed_val.var_char[1]=msg.BYTE3;
    recibed_val.var_char[2]=msg.BYTE4;
    recibed_val.var_char[3]=msg.BYTE5;
    switch (msg.BYTE1) {
    case LACAN_VAR_VO_INST:
        gen_vo = recibed_val.var_float;
        break;
    case LACAN_VAR_IO_INST:
        gen_io = recibed_val.var_float;
        break;
    case LACAN_VAR_PO_INST:
        gen_po = recibed_val.var_float;
        break;
    case LACAN_VAR_W_INST:
        gen_vel = recibed_val.var_float;
        break;
    case LACAN_VAR_TORQ_INST:
        gen_tor = recibed_val.var_float;
        break;
    case LACAN_VAR_I_BAT_INST:
        gen_ibat = recibed_val.var_float;
        break;
    case LACAN_VAR_VO_SETP:
        lim_vdc = recibed_val.var_float;
        break;
    case LACAN_VAR_W_SETP:
        speed_ref=recibed_val.var_float;
        break;
    case LACAN_VAR_TORQ_SETP:
        torque_ref=recibed_val.var_float;
        break;
    case LACAN_VAR_PO_SETP:
        pot_ref = recibed_val.var_float;
        break;
    case LACAN_VAR_I_BAT_SETP:
        lim_ibat = recibed_val.var_float;
        break;
    case LACAN_VAR_IEF_SETP:
        lim_ief = recibed_val.var_float;
        break;
    case LACAN_VAR_ISD_SETP:
        isd_ref = recibed_val.var_float;
        break;
    case LACAN_VAR_MOD:
        actual_mode=recibed_val.var_char[0];
        actual_mode_index = ui->combo_modo->findData(actual_mode);
        if(actual_mode_index>-1){
            ui->combo_modo->setEnabled(true);
            ui->combo_modo->setCurrentIndex(actual_mode_index);
        }
        refresh_values();
        break;
    default:
        break;
    }
}

void Gen_Eolico::send_qry_variables(){
    mw->LACAN_Query(LACAN_VAR_VO_INST,false,dest);  //gen_vo
    mw->LACAN_Query(LACAN_VAR_IO_INST,false,dest);  //gen_io
    mw->LACAN_Query(LACAN_VAR_I_BAT_INST,false,dest);   //gen_ibat
    mw->LACAN_Query(LACAN_VAR_W_INST,false,dest);   //gen_ibat
    mw->LACAN_Query(LACAN_VAR_TORQ_INST,false,dest);   //gen_ibat
    mw->LACAN_Query(LACAN_VAR_PO_INST,false,dest);   //gen_po
}

void Gen_Eolico::send_qry_references(){
    mw->LACAN_Query(LACAN_VAR_W_SETP,false,dest);   //speed_ref
    mw->LACAN_Query(LACAN_VAR_PO_SETP,false,dest);   //po_ref
    mw->LACAN_Query(LACAN_VAR_TORQ_SETP,false,dest);   //torq_ref
    mw->LACAN_Query(LACAN_VAR_IEF_SETP,false,dest);   //ief
    mw->LACAN_Query(LACAN_VAR_ISD_SETP,false,dest);   //isd
    mw->LACAN_Query(LACAN_VAR_I_BAT_SETP,false,dest);   //lim_ibat
    mw->LACAN_Query(LACAN_VAR_VO_SETP,false,dest);   //lim_vdc

    mw->LACAN_Query(LACAN_VAR_MOD,false,dest);   //modo
}

//Se actualizan todos los valores del GENERADOR
void Gen_Eolico::refresh_values(){

    refresh_mode();

    //Verifica  que haya llegado al menos un valor valido. Solo se aplica a las variables de SET (que tienen SpinBox)
    if(double(isd_ref) > refValue)
        ui->spin_gen_isd_ref->setEnabled(true);
    if(double(lim_ibat) > refValue)
        ui->spin_gen_lim_ibat_ref->setEnabled(true);
    if(double(lim_ief) > refValue)
        ui->spin_gen_lim_ief_ref->setEnabled(true);
    if(double(lim_vdc) > refValue)
        ui->spin_gen_lim_vdc_ref->setEnabled(true);

    //Variables SET (SpinBox)
    ui->spin_gen_isd_ref->setValue(double(isd_ref));
    ui->spin_gen_lim_ibat_ref->setValue(double(lim_ibat));
    ui->spin_gen_lim_ief_ref->setValue(double(lim_ief));
    ui->spin_gen_lim_vdc_ref->setValue(double(lim_vdc));
    ui->spin_gen_pot_ref->setValue(double(pot_ref));
    ui->spin_gen_speed_ref->setValue(double(speed_ref));
    ui->spin_gen_torque_ref->setValue(double(torque_ref));

    //Variables de Salida (Labels)
    if(double(gen_vo)>refValue)
        ui->label_gen_vo->setText(QString::number(double(gen_vo),'f',2));
    if(double(gen_io)>refValue)
        ui->label_gen_io->setText(QString::number(double(gen_io),'f',2));
    if(double(gen_ibat)>refValue)
        ui->label_gen_ibat->setText(QString::number(double(gen_ibat),'f',2));
    if(double(gen_po)>refValue)
        ui->label_gen_po->setText(QString::number(double(gen_po),'f',2));
    if(double(gen_tor)>refValue)
        ui->label_gen_tor->setText(QString::number(double(gen_tor),'f',2));
    if(double(gen_vel)>refValue)
        ui->label_gen_vel->setText(QString::number(double(gen_vel),'f',2));
}

void Gen_Eolico::on_pushButton_start_clicked()
{
    cmd=LACAN_CMD_START;
    mw->LACAN_Do(cmd,false,dest);
    assert(mw->msg_ack.back());
    connect(&(mw->msg_ack.back()->ack_timer),SIGNAL(timeout()), mw, SLOT(verificarACK()));
    mw->agregar_log_sent();

}

void Gen_Eolico::on_pushButton_stop_clicked()
{
    cmd=LACAN_CMD_STOP;
    mw->LACAN_Do(cmd,false,dest);
    assert(mw->msg_ack.back());
    connect(&(mw->msg_ack.back()->ack_timer),SIGNAL(timeout()), mw, SLOT(verificarACK()));
    mw->agregar_log_sent();
}

//Para hacer que el boton de stop lo parezca realmente se da esa idea mediante dos imagenes distintas
//que cambian para cada estado del boton
void Gen_Eolico::on_pushButton_stop_pressed()
{
    QPixmap pixmap(":/Imagenes/stop_press.png");
    QIcon ButtonIcon(pixmap);
    ui->pushButton_stop->setIcon(ButtonIcon);
}


void Gen_Eolico::on_pushButton_stop_released()
{
    QPixmap pixmap(":/Imagenes/stop_normal.png");
    QIcon ButtonIcon(pixmap);
    ui->pushButton_stop->setIcon(ButtonIcon);
}

void Gen_Eolico::on_pushButton_comandar_clicked()
{
   Comandar *comwin = new Comandar(mw,dest);
   comwin->setAttribute(Qt::WA_DeleteOnClose);
   comwin->setModal(true);
   comwin->show();
   send_queries = false;
   referenceChanged = true;
   connect(comwin, SIGNAL(comWindowsClosed()), this, SLOT(focusReturned()));

}

//Esta funcion esta conectada con el cambio en el combo box de modo
//bajo un cambio del modo seleccionado, se confirma con el usuario y se envia el mensaje correspondiente
void Gen_Eolico::verificar_mode_changed(){
    QMessageBox::StandardButton reply;
    QString str="¿Esta seguro que desea cambiar al modo ";
    str.append(ui->combo_modo->currentText());
    str.append(" ?");
    reply = QMessageBox::question(this,"Confirm",str, QMessageBox::Yes | QMessageBox::No );
    if(reply==QMessageBox::Yes){  
        //version2
        data_can modo;
        modo.var_char[0] = uchar(actual_mode);
        modo.var_char[1] = 0;
        modo.var_char[2] = 0;
        modo.var_char[3] = 0;
        mw->LACAN_Set(LACAN_VAR_MOD,modo,false,dest);
        assert(mw->msg_ack.back());
        connect(&(mw->msg_ack.back()->ack_timer),SIGNAL(timeout()), mw, SLOT(verificarACK()));
        mw->agregar_log_sent();
        referenceChanged = true;
    }
    else{
        actual_mode=previous_mode;
        ui->combo_modo->setCurrentIndex(ui->combo_modo->findData(QVariant(actual_mode)));
    }
}

//Habilita y deshabilita los campos, dependiendo el modo actual
void Gen_Eolico::refresh_mode(){

    switch (actual_mode) {
    case LACAN_VAR_MOD_VEL:     //Velocidad
        ui->label_pot_ref->setDisabled(true);
        ui->spin_gen_pot_ref->setDisabled(true);

        ui->label_speed_ref->setEnabled(true);
        if(speed_ref > refValue)
            ui->spin_gen_speed_ref->setEnabled(true);

        ui->label_torque_ref->setDisabled(true);
        ui->spin_gen_torque_ref->setDisabled(true);
        break;
    case LACAN_VAR_MOD_POT:     //Potencia
        ui->label_pot_ref->setEnabled(true);
        if(pot_ref > refValue)
            ui->spin_gen_pot_ref->setEnabled(true);

        ui->label_speed_ref->setDisabled(true);
        ui->spin_gen_speed_ref->setDisabled(true);

        ui->label_torque_ref->setDisabled(true);
        ui->spin_gen_torque_ref->setDisabled(true);
        break;
    case LACAN_VAR_MOD_TORQ:     //Torque
        ui->label_pot_ref->setDisabled(true);
        ui->spin_gen_pot_ref->setDisabled(true);

        ui->label_speed_ref->setDisabled(true);
        ui->spin_gen_speed_ref->setDisabled(true);

        ui->label_torque_ref->setEnabled(true);
        if(torque_ref > refValue)
            ui->spin_gen_torque_ref->setEnabled(true);
        break;
    case LACAN_VAR_MOD_MPPT:     //MPPT
        ui->label_pot_ref->setDisabled(true);
        ui->spin_gen_pot_ref->setDisabled(true);

        ui->label_speed_ref->setDisabled(true);
        ui->spin_gen_speed_ref->setDisabled(true);

        ui->label_torque_ref->setDisabled(true);
        ui->spin_gen_torque_ref->setDisabled(true);
        break;
    default:
        break;
    }
}

void Gen_Eolico::on_combo_modo_currentIndexChanged(int index)
{
   previous_mode = actual_mode;     //guardo el modo anterior por si el usuario cancela el cambio
   actual_mode = uint16_t(ui->combo_modo->itemData(index).toInt());
}

void Gen_Eolico::closeEvent(QCloseEvent *e){
    time_2sec->stop();
    emit genWindowsClosed();
    QDialog::closeEvent(e);
}

//Para volver a habilitar los queries cuando se cierra comandar(abierta desde esta ventana)
void Gen_Eolico::focusReturned(){
    send_queries = true;
}

//Esta funcion se utiliza para procesar los cambios que se quieren realizar mediante un spinbox
void Gen_Eolico::processEditingFinished(QDoubleSpinBox* spin, uint16_t var, float prevValue)
{
    //Se bloquean todas las señales de los spinboxes para prevenir errores
    blockAllSpinSignals(true);
    //Se guardan los valores y se envia el mensaje correspondiente luego de confirmar
    // con el usuario
    data_can data;
    float value = float(spin->value());
    int reply;
    QString str = "El valor a enviar es: ";
    str.append(QString::number(double(value)));
    str.append(". Confirma que desea enviar este valor?");
    reply=QMessageBox::question(this,"Valor a enviar",str,QMessageBox::Yes|QMessageBox::No);

    if(reply==QMessageBox::Yes){
        data.var_float = value;
        mw->LACAN_Set(var, data, 1, dest);
        mw->agregar_log_sent();
        referenceChanged = true;//flag para triggear el refresco del campo mediante el envio de una query
    }

    spin->setValue(double(prevValue));//se vuelve a mostrar el valor previo hasta que se confirmen los cambios mediante una query

    ui->edit_checkBox->setCheckState(Qt::CheckState::Unchecked);//deshabilitamos el modo editable al terminar
}

//Bloquea todas las señales de los spinboxes debido a errores de eventos espureos
void Gen_Eolico::blockAllSpinSignals(bool b){

    ui->spin_gen_isd_ref->blockSignals(b);
    ui->spin_gen_lim_ibat_ref->blockSignals(b);
    ui->spin_gen_lim_ief_ref->blockSignals(b);
    ui->spin_gen_lim_vdc_ref->blockSignals(b);
    ui->spin_gen_pot_ref->blockSignals(b);
    ui->spin_gen_speed_ref->blockSignals(b);
    ui->spin_gen_torque_ref->blockSignals(b);
}

//Cuando se recibe la señal de un spinbox(al apretar enter o perder focus) se procede a procesarlo mediante
//la funcion antes vista, identificando la variable a la cual corresponde
void Gen_Eolico::on_spin_gen_speed_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_speed_ref, LACAN_VAR_W_SETP, speed_ref);
}

void Gen_Eolico::on_spin_gen_pot_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_pot_ref, LACAN_VAR_PO_SETP, pot_ref);
}

void Gen_Eolico::on_spin_gen_torque_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_torque_ref, LACAN_VAR_TORQ_SETP, torque_ref);
}

void Gen_Eolico::on_spin_gen_lim_ief_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_lim_ief_ref, LACAN_VAR_IEF_SETP, lim_ief);
}

void Gen_Eolico::on_spin_gen_isd_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_isd_ref, LACAN_VAR_ISD_SETP, isd_ref);
}

void Gen_Eolico::on_spin_gen_lim_ibat_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_lim_ibat_ref, LACAN_VAR_I_BAT_SETP, lim_ibat);
}

void Gen_Eolico::on_spin_gen_lim_vdc_ref_editingFinished()
{
    processEditingFinished(ui->spin_gen_lim_vdc_ref, LACAN_VAR_VO_SETP, lim_vdc);
}

//Cambia los estados de los widgets correspondientes segun el checkbox de modo editable
//Principalmente deshabilita los botones y habilita el modo editable de los spinbox,
//volviendo a habilitar todas sus señales
void Gen_Eolico::on_edit_checkBox_stateChanged(int checked)
{
    if(checked)
    {
        send_queries = false;

        ui->pushButton_start->blockSignals(true);

        ui->pushButton_comandar->setDisabled(true);
        ui->pushButton_start->setDisabled(true);
        ui->pushButton_stop->setDisabled(true);
        ui->combo_modo->setDisabled(true);


        ui->spin_gen_isd_ref->clearFocus();
        ui->spin_gen_lim_ibat_ref->clearFocus();
        ui->spin_gen_lim_ief_ref->clearFocus();
        ui->spin_gen_lim_vdc_ref->clearFocus();
        ui->spin_gen_pot_ref->clearFocus();
        ui->spin_gen_speed_ref->clearFocus();
        ui->spin_gen_torque_ref->clearFocus();

        blockAllSpinSignals(false);

        ui->spin_gen_isd_ref->setReadOnly(false);
        ui->spin_gen_lim_ibat_ref->setReadOnly(false);
        ui->spin_gen_lim_ief_ref->setReadOnly(false);
        ui->spin_gen_lim_vdc_ref->setReadOnly(false);
        ui->spin_gen_pot_ref->setReadOnly(false);
        ui->spin_gen_speed_ref->setReadOnly(false);
        ui->spin_gen_torque_ref->setReadOnly(false);

        ui->label_edit->setEnabled(true);

    }else{
        send_queries = true;

        ui->pushButton_comandar->setDisabled(false);
        ui->pushButton_start->setDisabled(false);
        ui->pushButton_stop->setDisabled(false);
        ui->combo_modo->setDisabled(false);

        blockAllSpinSignals(true);

        ui->spin_gen_isd_ref->setReadOnly(true);
        ui->spin_gen_lim_ibat_ref->setReadOnly(true);
        ui->spin_gen_lim_ief_ref->setReadOnly(true);
        ui->spin_gen_lim_vdc_ref->setReadOnly(true);
        ui->spin_gen_pot_ref->setReadOnly(true);
        ui->spin_gen_speed_ref->setReadOnly(true);
        ui->spin_gen_torque_ref->setReadOnly(true);

        ui->label_edit->setDisabled(true);
    }
}

void Gen_Eolico::changeEditState()
{
    ui->edit_checkBox->toggle();
}


