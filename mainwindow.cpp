#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "comandar.h"
#include "consultar.h"
#include "enviar_mensaje.h"
#include "estadored.h"
#include "PC.h"
#include <QtGui>
#include <QGridLayout>
#include <QLabel>
#include <QString>
#include <QDateTime>
#include <QMessageBox>
#include "lacan_detect.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "addnewdevdialog.h"
#include <QColor>
#include "bytesend.h"
#include <stdlib.h>
#include <string.h>
#include "lacan_limits_vol.h"
#include "lacan_limits_broad.h"
#include "lacan_limits_gen.h"
#include <QThread>
#include "assert.h"


/***Funciones genericas***/
//Carga de mensajes al log en un csv, se crea un archivo nuevo por mes en una carpeta llamada Log de Mensajes LACAN, la cual
//(normalmente) se guarda en la carpeta Documentos del usuario.
void agregar_textlog(ABSTRACTED_MSG abs_msg, QString way){
    static uint8_t cont=0;
    //Guardo el path que por defecto tiene configurado el SO como carpeta personal del usuario (normalmente Documentos)
    //Si dicho path no existFe configurado se guarda en la carpeta del programa
    QString file_folder=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString file_path="";

    //Si la carpeta Log de Mensajes LACAN no se encuentra creada la creo antes de guardar los txt
    if(QDir(file_folder+"/Log de Mensajes LACAN").exists()){
        //Especifico el nombre del archivo segun el mes, si no existe se crea automaticamente al guardar datos
        file_path = file_folder+"/Log de Mensajes LACAN/"+abs_msg.curr_time.left(7)+".csv";
    }else if(QDir(file_folder).exists()){
        QDir(file_folder).mkdir("Log de Mensajes LACAN");
        file_path = file_folder+"/Log de Mensajes LACAN/"+abs_msg.curr_time.left(7)+".csv";
    }else {
        file_path = QDir::currentPath()+"/"+abs_msg.curr_time.left(7)+".csv";
    }

    //Abro el archivo solamente para la escritura en modo "Append" y "Text" con lo cual agrega cada nueva linea de manera consecutiva
    QFile file(file_path);
    file.open(QFile::WriteOnly | QFile::Append | QFile::Text);
    //Armo un "flujo" de datos hacia el archivo, en el se pondran las lineas que deberan reflejarse en el archivo luego
    QTextStream out(&file);
    //Para mantener la legibilidad del txt se vuelven a escribir los nombres de columnas cuando se inicia el programa
    //o cuando se insertaron 25 lineas (mensajes) en el archivo
    if(!(cont%25)){
        out<<"Sentido"<<";"<<"Fecha y hora"<<";"<<"Destino"<<";"<<"Funcion"<<";"<<"Tipo de variable"<<";"<<"Valor de variable"<<";"<<"Comando"<<";"<<"Codigo de ACK"<<";"<<"Respuesta ACK"<<";"<<"Codigo de error"<<"\n";
        cont=0;
    }
    out<<way<<";"<<abs_msg.curr_time<<";"<<abs_msg.dest<<";"<<abs_msg.fun<<";"<<abs_msg.var_type<<";"<<abs_msg.var_val<<";"<<abs_msg.com<<";"<<abs_msg.ack_code<<";"<<abs_msg.ack_res<<";"<<abs_msg.err_code<<"\n";
    //Vacio buffers (lo cual obliga al flujo a plasmarse de manera definitiva en el archivo) y luego cierro el archivo
    file.flush();
    file.close();
    cont++;
}

//Funcion interna para transformar el contenido de los mensajes en strings para poder cargarlos en el txt
ABSTRACTED_MSG abstract_msg(LACAN_MSG msg){
    QString format_time="hh:mm:ss";
    QString format_date="yyyy-MM-dd";
    QDateTime curr_date_time=QDateTime::currentDateTime();
    ABSTRACTED_MSG abs_msg={"","","","","","","","","", curr_date_time.toString(format_date)+" "+curr_date_time.toString(format_time)+"hs"};
    float val_float;
    data_can val_union;

    //Transformacion del destino y remitente en strings segun el ID del mensaje
    //DESTINO
    switch((msg.BYTE0 >> LACAN_BYTE0_RESERVED)&LACAN_IDENT_MASK){
    case LACAN_ID_BOOST:
        abs_msg.dest="Boost\t";
        break;
    case LACAN_ID_GEN:
        abs_msg.dest="Generador Eolico";
        break;
    case LACAN_ID_BROADCAST:
        abs_msg.dest="Broadcast\t";
        break;
    case LACAN_ID_VOLANTE:
        abs_msg.dest="Volante de Inercia";
        break;
    default:
        abs_msg.dest="No especificada/No soportada";
        break;
    }

    //REMITENTE
    switch(msg.ID&LACAN_IDENT_MASK){
    case LACAN_ID_BOOST:
        abs_msg.orig="Boost\t";
        break;
    case LACAN_ID_GEN:
        abs_msg.orig="Generador Eolico";
        break;
    case LACAN_ID_BROADCAST:
        abs_msg.orig="Broadcast\t";
        break;
    case LACAN_ID_VOLANTE:
        abs_msg.orig="Volante de Inercia";
        break;
    default:
        abs_msg.orig="No especificada/No soportada";
        break;
    }

    //Transformacion de la funcion y sus correspondientes campos de datos
    //La debida transformacion de los campos se realizan con las funciones detect, las cuales se encuentran en lacan_detect
    //(no son mas que switches gigantes)
    //FUNCION
    switch((msg.ID&LACAN_FUN_MASK)>>LACAN_IDENT_BITS){
    case LACAN_FUN_ERR:
        abs_msg.fun="Error";
        abs_msg.err_code=detect_err(msg.BYTE1);
        break;

    case LACAN_FUN_DO:
        abs_msg.fun="Do";
        abs_msg.ack_code=QString::number(msg.BYTE1);
        abs_msg.com=detect_cmd(msg.BYTE2);
        break;

    case LACAN_FUN_SET:
        abs_msg.fun="Set";
        abs_msg.ack_code=QString::number(msg.BYTE1);
        abs_msg.var_type=detect_var(msg.BYTE2);

        if(abs_msg.var_type=="Modo"){
            abs_msg.var_val = detect_mode(msg.BYTE3);
        }
        else{
            val_union.var_char[0]=uchar(msg.BYTE3);
            val_union.var_char[1]=uchar(msg.BYTE4);
            val_union.var_char[2]=uchar(msg.BYTE5);
            val_union.var_char[3]=uchar(msg.BYTE6);

            val_float = val_union.var_float;
            abs_msg.var_val=QString::number(double(val_float),'f',2);
        }
        break;

    case LACAN_FUN_QRY:
        abs_msg.fun="Query";
        abs_msg.ack_code=QString::number(msg.BYTE1);
        abs_msg.var_type=detect_var(msg.BYTE2);
        break;

    case LACAN_FUN_ACK:
        abs_msg.fun="Acknowledge";
        abs_msg.ack_res=detect_res(msg.BYTE2);
        abs_msg.ack_code=QString::number(msg.BYTE1);
        break;

    case LACAN_FUN_POST:
        abs_msg.fun="Post";
        abs_msg.var_type=detect_var(msg.BYTE1);
        if(abs_msg.var_type=="Modo"){
            abs_msg.var_val = detect_mode(msg.BYTE2);
        }
        else{
            val_union.var_char[0]=uchar(msg.BYTE2);
            val_union.var_char[1]=uchar(msg.BYTE3);
            val_union.var_char[2]=uchar(msg.BYTE4);
            val_union.var_char[3]=uchar(msg.BYTE5);

            val_float = val_union.var_float;
            abs_msg.var_val=QString::number(double(val_float),'f',2);
        }
        break;

    case LACAN_FUN_HB:
        abs_msg.fun="Heartbeat";
        break;

    default:
        abs_msg.fun="No especificada/soportada";
    }
    //Se devuelve el mensaje "abstraido"
    return abs_msg;
}

/***Funciones de la Main Windows***/
MainWindow::MainWindow(QSerialPort &serial_port0,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("LACAN");

    //Inicializacion de variables(el puerto serie se obtiene de una ventana previa)
    serial_port=&serial_port0;
    do_log=false;
    ERflag= false;
    NoUSB=false;
    outlog_cont=0;
    inlog_cont=0;
    filter_both_lists = true;

    //Se inicializa el timer con el cual se van a realizar acciones periodicas en el slot do_stuff, siendo la principal
    //mandar Heartbeats
    periodicTimer = new QTimer();
    periodicTimer->start(HB_TIME);
    connect(periodicTimer,SIGNAL(timeout()),this,SLOT(do_stuff()));
    //Se inicializa un thread que funciona basicamente como timer, a diferencia de los que provee Qt y la API de Windows, este
    //cuenta tics del microprocesador, obteniendose precision de por lo menos 100us, en este caso se usara cada 0,5s para
    //realizar el envio de los mensajes de manera escpaciada para no saturar el adaptador debido a la diferencia de velocidades
    //CAN y serie
    msgSender=new SenderThread();
    connect(msgSender,SIGNAL(sendTimeout()),this,SLOT(handleSendTimeout()));
    connect(this, SIGNAL(mustStopSenderThread(bool)), msgSender, SLOT(changeMustRun(bool)));
    msgSender->start();

    //Inicializacion de los dispositivos que se muestran como conectados en la lista de la UI, si pasa un tiempo
    //esecificado sin enviar HB sera dado de baja

    QListWidgetItem* bc = new QListWidgetItem("Broadcast");

    ui->the_one_true_list_DESTINO->addItem(bc);

    ui->the_one_true_list_DESTINO->setCurrentItem(bc);

    //Mapeo del nombre de los dispositivos conectados con su respectivo ID
    disp_map["Broadcast"]=LACAN_ID_BROADCAST;
    disp_map["Generador Eolico"]=LACAN_ID_GEN;
    disp_map["Volante de Inercia"]=LACAN_ID_VOLANTE;
    disp_map["Boost"]=LACAN_ID_BOOST;

    //Creo un objeo para cada dispositivo, el cual almacena su ID, su estado y timer
    //dicho timer servira para detectar la inactividad del dispositivo y cambiar su estado
    //lo cual se hace en el slot verificarHB()

    newdev=new HB_CONTROL();
    newdev->device=LACAN_ID_GEN;
    newdev->hb_status=INACTIVE;
    hb_con.push_back(newdev);
    //Para futuras implementaciones
//    newdev=new HB_CONTROL();
//    newdev->device=LACAN_ID_BOOST;
//    newdev->hb_status=INACTIVE;
//    hb_con.push_back(newdev);
    newdev=new HB_CONTROL();
    newdev->device=LACAN_ID_VOLANTE;
    newdev->hb_status=INACTIVE;
    hb_con.push_back(newdev);

    for(vector<HB_CONTROL*>::iterator it_hb=hb_con.begin(); it_hb < hb_con.end(); it_hb++){
        assert((*it_hb));
        connect(&((*it_hb)->hb_timer), SIGNAL(timeout()), this, SLOT(verificarHB()));
    }

    //Armado de las tablas de mensajes recibidos y enviados en la UI
    QStringList TableHeader_send;
    QStringList TableHeader_rece;
    TableHeader_send<<"Destino"<<"Funcion"<<"Variable"<<"Valor"<<"Comando"<<"Codigo de ack"<<"Resultado ack"<<"Codigo de error"<<"Fecha y Hora";
    TableHeader_rece<<"Origen"<<"Funcion"<<"Variable"<<"Valor"<<"Comando"<<"Codigo de ack"<<"Resultado ack"<<"Codigo de error"<<"Fecha y Hora";

    ui->tableWidget_received->setRowCount(list_rec_cont);
    ui->tableWidget_received->setColumnCount(9);
    ui->tableWidget_received->setHorizontalHeaderLabels(TableHeader_rece);
    ui->tableWidget_received->verticalHeader()->setVisible(false);
    ui->tableWidget_received->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_received->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_received->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_received->setShowGrid(false);
    ui->tableWidget_received->setStyleSheet("QTableView {selection-background-color: blue;}");

    ui->tableWidget_sent->setRowCount(list_send_cont);
    ui->tableWidget_sent->setColumnCount(9);
    ui->tableWidget_sent->setHorizontalHeaderLabels(TableHeader_send);
    ui->tableWidget_sent->verticalHeader()->setVisible(false);
    ui->tableWidget_sent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_sent->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_sent->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_sent->setShowGrid(false);
    ui->tableWidget_sent->setStyleSheet("QTableView {selection-background-color: blue;}");

    //creacion de los mapas que asocian los string de las variables con su respectivo ID segun a que
    //dispositivo pertenezcan, deberia haber un "create_varmap_" por cada dispositivo conectado a la red
    create_varmap_broadcast();
    create_varmap_gen();
    create_varmap_vol();

    readerth = new ReaderThread(*serial_port);
    readerth->start();
    //Conecto la señal que indica que hay datos para leer en el buffer del puerto con nuestro slot para procesarla
    connect(serial_port, SIGNAL(readyRead()), readerth, SLOT(handleRead()));
    connect(readerth, SIGNAL(receivedMsg(LACAN_MSG)), this, SLOT(handleProcessedMsg(LACAN_MSG)));

    //Conecto la señal que indica un error en el puerto serie con nuestro slot para manejarla
    connect(serial_port, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(handlePortError(QSerialPort::SerialPortError)));
}

MainWindow::~MainWindow()
{
    delete ui;
    this->mustStopSenderThread(true);
    msgSender->quit();
    msgSender->wait();
    delete msgSender;
    delete periodicTimer;
    delete serial_port;
    readerth->quit();
    readerth->wait();
    delete readerth;

    for(auto pAck : msg_ack){
        delete pAck;
    }
    msg_ack.clear();

    for(auto pHb : hb_con){
        delete pHb;
    }
    hb_con.clear();

    for(auto p : stack){
        delete p;
    }
    stack.clear();
}

//Funcion que verifica el destino segun lo que este seleccionado en la lista
uint16_t MainWindow::verificar_destino(){
    uint16_t dest=0;
    return dest=disp_map[(ui->the_one_true_list_DESTINO->currentItem()->text())];
}

//Funcion para agregar los mensajes de salida al log correspondiente que se encuentra en la UI
//esta funcionalidad es similar a la que luego se implementa para los mensajes recibidos, con lo cual se explica una sola vez
void MainWindow::agregar_log_sent(){
    ABSTRACTED_MSG abs_msg;
    //Abstraigo a string el ultimo mensaje del vector con la funcion antes vista
    abs_msg=abstract_msg(msg_log.back());
    msg_log.pop_back();
    bool iWantQueries = !(ERflag && (abs_msg.fun == "Query"));
    //para la gran cantidad de queries q se mandan al estar en la ventana de DB
    //Si se encuentra activado el log de mensajes, verifico que no estemos por encima del limite para agregar
    //el mensaje al log widget
    if(do_log && iWantQueries){
        if(list_send_cont>=LOG_LIMIT){  //limite de mensajes
            //Si se supera el limite se le pregunta al usuario si quiere reiniciar el log (borrando mensajes anteriores)
            //o si quiere detener la sesion para mantener dichos mensajes en pantalla
            do_log=FALSE;
            QMessageBox::StandardButton reply;
            reply=QMessageBox::warning(this,"Limite de log alcanzado.","Se ha superado la cantidad maxima de mensajes del log. Desea iniciar una nueva sesion?\n Se borraran los mensajes de la sesion anterior",QMessageBox::Yes|QMessageBox::No);
            if(reply==QMessageBox::Yes){
                ui->tableWidget_received->clearContents();
                ui->tableWidget_sent->clearContents();
                outlog_cont=0;
                msg_log.clear();
                list_rec_cont = 0;
                list_send_cont = 0;
                ui->tableWidget_received->setRowCount(list_rec_cont);
                ui->tableWidget_sent->setRowCount(list_send_cont);
                do_log=TRUE;
            }
        }else{
            ui->tableWidget_sent->setRowCount(outlog_cont+1);
            ui->tableWidget_sent->setItem(outlog_cont, 0, new QTableWidgetItem(abs_msg.dest));
            ui->tableWidget_sent->setItem(outlog_cont, 1, new QTableWidgetItem(abs_msg.fun));
            ui->tableWidget_sent->setItem(outlog_cont, 2, new QTableWidgetItem(abs_msg.var_type));
            ui->tableWidget_sent->setItem(outlog_cont, 3, new QTableWidgetItem(abs_msg.var_val));
            ui->tableWidget_sent->setItem(outlog_cont, 4, new QTableWidgetItem(abs_msg.com));
            ui->tableWidget_sent->setItem(outlog_cont, 5, new QTableWidgetItem(abs_msg.ack_code));
            ui->tableWidget_sent->setItem(outlog_cont, 6, new QTableWidgetItem(abs_msg.ack_res));
            ui->tableWidget_sent->setItem(outlog_cont, 7, new QTableWidgetItem(abs_msg.err_code));
            ui->tableWidget_sent->setItem(outlog_cont, 8, new QTableWidgetItem(abs_msg.curr_time));
            outlog_cont++;
        }

        if(filter_both_lists){
            QString filter = ui->searchBar->text();
            filter_on_sent_searchBar(filter);
        }else{
            QString filter = ui->sent_searchBar->text();
            filter_on_sent_searchBar(filter);
        }

        ui->tableWidget_sent->scrollToBottom();
    }
    //No importan las configuraciones para el caso del log en el txt, este siempre debe estar vigente
    agregar_textlog(abs_msg,"Enviado");
}

void MainWindow::agregar_log_rec(){
    ABSTRACTED_MSG abs_msg;
    abs_msg=abstract_msg(msg_log.back());
    msg_log.pop_back();

    bool is_interesting = !ERflag && abs_msg.fun!=LACAN_FUN_POST;

    if(do_log && is_interesting){
        if(list_rec_cont>=LOG_LIMIT){  //limite de mensajes
            QMessageBox::StandardButton reply;
            reply=QMessageBox::warning(this,"Limite de log alcanzado.","Se ha superado la cantidad maxima de mensajes del log. Desea iniciar una nueva sesion?\n Se borraran los mensajes de la sesion anterior",QMessageBox::Yes|QMessageBox::No);
            if(reply==QMessageBox::Yes){
                ui->tableWidget_received->clearContents();
                ui->tableWidget_sent->clearContents();
                outlog_cont=0;
                msg_log.clear();
                list_rec_cont = 0;
                list_send_cont = 0;
                ui->tableWidget_received->setRowCount(list_rec_cont);
                ui->tableWidget_sent->setRowCount(list_send_cont);
                do_log=TRUE;
            }else{
                on_logButton_clicked();
            }
        }else{
            ui->tableWidget_received->setRowCount(inlog_cont+1);
            ui->tableWidget_received->setItem(inlog_cont, 0, new QTableWidgetItem(abs_msg.orig));
            ui->tableWidget_received->setItem(inlog_cont, 1, new QTableWidgetItem(abs_msg.fun));
            ui->tableWidget_received->setItem(inlog_cont, 2, new QTableWidgetItem(abs_msg.var_type));
            ui->tableWidget_received->setItem(inlog_cont, 3, new QTableWidgetItem(abs_msg.var_val));
            ui->tableWidget_received->setItem(inlog_cont, 4, new QTableWidgetItem(abs_msg.com));
            ui->tableWidget_received->setItem(inlog_cont, 5, new QTableWidgetItem(abs_msg.ack_code));
            ui->tableWidget_received->setItem(inlog_cont, 6, new QTableWidgetItem(abs_msg.ack_res));
            ui->tableWidget_received->setItem(inlog_cont, 7, new QTableWidgetItem(abs_msg.err_code));
            ui->tableWidget_received->setItem(inlog_cont, 8, new QTableWidgetItem(abs_msg.curr_time));
            inlog_cont++;
        }

        if(filter_both_lists){
            QString filter = ui->searchBar->text();
            filter_on_rec_searchBar(filter);
        }else{
            QString filter = ui->received_searchBar->text();
            filter_on_rec_searchBar(filter);
        }

        ui->tableWidget_received->scrollToBottom();
    }

    agregar_textlog(abs_msg,"Recibido");
}

void MainWindow::no_ACK_Handler(void){}

//Se encarga de cambiar el valor de la ERflag cualquiera sea su valor
void MainWindow::change_ERflag(bool value){
    ERflag = value;
}

//Agregado de nuevo dispositivo luego de su deteccion
void MainWindow::add_new_device(uint16_t source){
    //Cargo los datos del nuevo dispositivo en un objeto buffer e inicializo el timer de HB seteando el estado como activo
    newdev = new HB_CONTROL();
    newdev->device=source;
    newdev->hb_timer.start(DEAD_HB_TIME);
    newdev->hb_status=ACTIVE;
    //Abro una ventana para pedirle al usuario que ingrese un nombre para este nuevo dispositivo,
    //este sera el nombre con el cual aparezca en toda la plataforma, cuando acepta la venta se redirige al proceso
    //a la siguiente funcion
    AddNewDevDialog *diag=new AddNewDevDialog(this);
    diag->setAttribute(Qt::WA_DeleteOnClose);
    diag->setModal(true);
    connect(diag, SIGNAL(dev_name_set(QString)), this, SLOT(add_dev_name(QString)));
    diag->show();

}

//Borra un dispositivo de la lista de conectados segun su ID
void MainWindow::erase_device_ui(uint16_t inactiveDev){
    QString name = disp_map.key(inactiveDev);//Hago un mapeo inverso para obtener el nombre que se muestra en la lista
    qDeleteAll(ui->the_one_true_list_DESTINO->findItems(name, Qt::MatchFixedString));//Borro dicho nombre
}

//Agrega un dispositivo a la lista de conectados segun su ID (sabiendo que ya esta cargado en el mapa)
void MainWindow::add_device_ui(uint16_t reactivatedDev){
    QString name = disp_map.key(reactivatedDev);
    ui->the_one_true_list_DESTINO->addItem(name);
}

//Se utiliza para preguntar si un cierto dispositivo, identificado por su ID, esta conectado o no
bool MainWindow::device_is_connected(uint8_t id){
    //Recorremos el vector de dispositivos en busca de uno que coincida con la ID proporcionada
    //En caso de encontrarlo se devuelve su estado (ACTIVE-true, INACTIVE-false)
    for(vector<HB_CONTROL*>::iterator it_hb=hb_con.begin(); it_hb < hb_con.end(); it_hb++){
        assert((*it_hb));
        if((*it_hb)->device == id){
            return (*it_hb)->hb_status;
        }
    }
    //Si no se encuentra dicho dispositivo se devuelve false (nunca estuvo conectado)
    return INACTIVE;
}

//Las funciones create_varmap_"nombre de dispositivo" crean el mapeo de variables para cada dispositivo en la red
//Si fuera necesario agregar un dispositivo hay que recrear su propia funcion create_varmap como las que se muestran
//a continuacion
void MainWindow::create_varmap_gen(){
    LACAN_VAR ISD_GEN;
    ISD_GEN.instantanea=LACAN_VAR_ISD_INST;
    ISD_GEN.setp=LACAN_VAR_ISD_SETP;
    ISD_GEN.max=LACAN_VAR_GEN_ISD_MAX;
    ISD_GEN.min=LACAN_VAR_GEN_ISD_MIN;
    LACAN_VAR IEF_GEN;
    IEF_GEN.instantanea=LACAN_VAR_IEF_INST;
    IEF_GEN.setp=LACAN_VAR_IEF_SETP;
    IEF_GEN.max=LACAN_VAR_GEN_IEF_MAX;
    IEF_GEN.min=LACAN_VAR_GEN_IEF_MIN;
    LACAN_VAR PO_GEN;
    PO_GEN.instantanea=LACAN_VAR_PO_INST;
    PO_GEN.setp=LACAN_VAR_PO_SETP;
    PO_GEN.max=LACAN_VAR_GEN_PO_MAX;
    PO_GEN.min=LACAN_VAR_GEN_PO_MIN;
    LACAN_VAR VO_GEN;
    VO_GEN.instantanea=LACAN_VAR_VO_INST;
    VO_GEN.setp=LACAN_VAR_VO_SETP;
    VO_GEN.max=LACAN_VAR_GEN_VO_MAX;
    VO_GEN.min=LACAN_VAR_GEN_VO_MIN;
    LACAN_VAR TORQ_GEN;
    TORQ_GEN.instantanea=LACAN_VAR_TORQ_INST;
    TORQ_GEN.setp=LACAN_VAR_TORQ_SETP;
    TORQ_GEN.max=LACAN_VAR_GEN_TORQ_MAX;
    TORQ_GEN.min=LACAN_VAR_GEN_TORQ_MIN;
    LACAN_VAR W_GEN;
    W_GEN.instantanea=LACAN_VAR_W_INST;
    W_GEN.setp=LACAN_VAR_W_SETP;
    W_GEN.max=LACAN_VAR_GEN_W_MAX;
    W_GEN.min=LACAN_VAR_GEN_W_MIN;
    LACAN_VAR IBAT_GEN;
    IBAT_GEN.instantanea=LACAN_VAR_I_BAT_INST;
    IBAT_GEN.setp=LACAN_VAR_I_BAT_SETP;
    IBAT_GEN.max=LACAN_VAR_GEN_IBAT_MAX;
    IBAT_GEN.min=LACAN_VAR_GEN_IBAT_MIN;

    varmap_gen["Corriente de ISD"]=ISD_GEN;
    varmap_gen["Corriente Eficaz"]=IEF_GEN;
    varmap_gen["Potencia de Salida"]=PO_GEN;
    varmap_gen["Tension de Salida"]=VO_GEN;
    varmap_gen["Torque"]=TORQ_GEN;
    varmap_gen["Velocidad Angular"]=W_GEN;
    varmap_gen["Corriente de Bateria"]=IBAT_GEN;
}

void MainWindow::create_varmap_vol(){
    LACAN_VAR ISD_VOL;
    ISD_VOL.instantanea=LACAN_VAR_ISD_INST;
    ISD_VOL.setp=LACAN_VAR_ISD_SETP;
    ISD_VOL.max=LACAN_VAR_VOL_ISD_MAX;
    ISD_VOL.min=LACAN_VAR_VOL_ISD_MIN;
    LACAN_VAR W_STBY_VOL;
    W_STBY_VOL.instantanea=LACAN_VAR_STANDBY_W_INST;
    W_STBY_VOL.setp=LACAN_VAR_STANDBY_W_SETP;
    W_STBY_VOL.max=LACAN_VAR_VOL_STANDBY_W_MAX;
    W_STBY_VOL.min=LACAN_VAR_VOL_STANDBY_W_MIN;
    LACAN_VAR PO_VOL;
    PO_VOL.instantanea=LACAN_VAR_PO_INST;
    PO_VOL.setp=LACAN_VAR_PO_SETP;
    LACAN_VAR VO_VOL;
    VO_VOL.instantanea=LACAN_VAR_VO_INST;
    VO_VOL.setp=LACAN_VAR_VO_SETP;
    LACAN_VAR TORQ_VOL;
    TORQ_VOL.instantanea=LACAN_VAR_TORQ_INST;
    TORQ_VOL.setp=LACAN_VAR_TORQ_SETP;
    LACAN_VAR IBAT_VOL;
    IBAT_VOL.instantanea=LACAN_VAR_I_BAT_INST;
    IBAT_VOL.setp=LACAN_VAR_I_BAT_SETP;

    varmap_vol["Corriente de ID"]=ISD_VOL;
    varmap_vol["Velocidad angular Standby"]=W_STBY_VOL;
    varmap_vol["Potencia de Salida"]=PO_VOL;
    varmap_vol["Torque"]=TORQ_VOL;
    varmap_vol["Tension de Salida"]=VO_VOL;
    varmap_vol["Corriente de Bateria"]=IBAT_VOL;
}

void MainWindow::create_varmap_broadcast(){
    LACAN_VAR ISD_BROAD;
    ISD_BROAD.instantanea=LACAN_VAR_ISD_INST;
    ISD_BROAD.setp=LACAN_VAR_ISD_SETP;
    ISD_BROAD.max=LACAN_VAR_BROAD_ISD_MAX;
    ISD_BROAD.min=LACAN_VAR_BROAD_ISD_MIN;

    LACAN_VAR IO_BROAD;
    IO_BROAD.instantanea=LACAN_VAR_IO_INST;
    IO_BROAD.setp=LACAN_VAR_IO_SETP;
    IO_BROAD.max=LACAN_VAR_BROAD_IO_MAX;
    IO_BROAD.min=LACAN_VAR_BROAD_IO_MIN;

    LACAN_VAR PO_BROAD;
    PO_BROAD.instantanea=LACAN_VAR_PO_INST;
    PO_BROAD.setp=LACAN_VAR_PO_SETP;
    PO_BROAD.max=LACAN_VAR_BROAD_PO_MAX;
    PO_BROAD.min=LACAN_VAR_BROAD_PO_MIN;

    LACAN_VAR VO_BROAD;
    VO_BROAD.instantanea=LACAN_VAR_VO_INST;
    VO_BROAD.setp=LACAN_VAR_VO_SETP;
    VO_BROAD.max=LACAN_VAR_BROAD_VO_MAX;
    VO_BROAD.min=LACAN_VAR_BROAD_VO_MIN;

    LACAN_VAR IBAT_BROAD;
    IBAT_BROAD.instantanea=LACAN_VAR_I_BAT_INST;
    IBAT_BROAD.setp=LACAN_VAR_I_BAT_SETP;
    IBAT_BROAD.max=LACAN_VAR_BROAD_IBAT_MAX;
    IBAT_BROAD.min=LACAN_VAR_BROAD_IBAT_MIN;

    varmap_broad["Corriente de ISD"]=ISD_BROAD;
    varmap_broad["Potencia de Salida"]=PO_BROAD;
    varmap_broad["Tension de Salida"]=VO_BROAD;
    varmap_broad["Corriente de Bateria"]=IBAT_BROAD;
    varmap_broad["Corriente de Salida"]=IO_BROAD;
}

void MainWindow::filter_on_sent_searchBar(QString filter){
    for( int i = 0; i < ui->tableWidget_sent->rowCount(); ++i )
    {
        bool match = false;
        for( int j = 0; j < ui->tableWidget_sent->columnCount(); ++j )
        {
            QTableWidgetItem *item = ui->tableWidget_sent->item( i, j );
            if( item->text().contains(filter, Qt::CaseInsensitive) )
            {
                match = true;
                break;
            }
        }
        ui->tableWidget_sent->setRowHidden( i, !match );
    }
}

void MainWindow::filter_on_rec_searchBar(QString filter){
    for( int i = 0; i < ui->tableWidget_received->rowCount(); ++i )
    {
        bool match = false;
        for( int j = 0; j < ui->tableWidget_received->columnCount(); ++j )
        {
            QTableWidgetItem *item = ui->tableWidget_received->item( i, j );
            if( item->text().contains(filter, Qt::CaseInsensitive) )
            {
                match = true;
                break;
            }
        }
        ui->tableWidget_received->setRowHidden( i, !match );
    }
}

/* *Funciones de recepcion de mensajes* */

//En el caso de que llegue a la computadora un mensaje que no tiene sentido, como por ejemplo un SET, DO o QUERY,
//se ejecuta esta funcion para monitorear el flujo de mensajes recibidos no soportados
void MainWindow::LACAN_NOTSUP_Handler(uint16_t source, uint16_t& notsup_count, uint16_t& notsup_gen, uint8_t code){
    uint16_t dest=source;
    LACAN_Acknowledge(code,uint16_t(LACAN_FAILURE), dest); //Envia un acknowledge anunciando que dicho mensaje es incorrecto
    //Aumento contadores de errores correspondientes
    notsup_count++;
    if(source&LACAN_ID_GEN){
        notsup_gen++;
    }
}

//Identifica el tipo de mensaje recibido para luego darle el correcto tratamiento
int MainWindow::LACAN_Msg_Handler(LACAN_MSG &mje, uint16_t& notsup_count, uint16_t& notsup_gen){
    //Extraemos remitente y funcion del mensaje
    uint16_t source=mje.ID&LACAN_IDENT_MASK;
    uint16_t fun=mje.ID>>LACAN_IDENT_BITS;
    uint8_t code;
    //Segun la funcion se procede a manejar el contenido de los mensajes, en caso de mensajes incorrectos se ejecuta la
    //funcion antes vista. En caso de no identificar la funcion se devuelve un codigo que indica que no existe dicho tipo
    //de mensaje. En condiciones normales de funcionamiento el codigo devuelto indica un procesamiento exitoso
    switch(fun){
    case LACAN_FUN_DO:
        code=mje.BYTE1;
        LACAN_NOTSUP_Handler(source, notsup_count, notsup_gen,code);
    break;
    case LACAN_FUN_SET:
        code=mje.BYTE1;
        LACAN_NOTSUP_Handler(source, notsup_count, notsup_gen,code);
    break;
    case LACAN_FUN_QRY:
        code=mje.BYTE1;
        LACAN_NOTSUP_Handler(source, notsup_count, notsup_gen,code);
    break;
    case LACAN_FUN_ACK:
        LACAN_ACK_Handler(mje.BYTE1);
    break;
    case LACAN_FUN_POST:
        data_can data;
        data.var_char[0] = mje.BYTE2;
        data.var_char[1] = mje.BYTE3;
        data.var_char[2] = mje.BYTE4;
        data.var_char[3] = mje.BYTE5;
        LACAN_POST_Handler(source,mje.BYTE1,data);
    break;
    case LACAN_FUN_ERR:
        LACAN_ERR_Handler(source,mje.BYTE1);
    break;
    case LACAN_FUN_HB:
        LACAN_HB_Handler(source);
        break;
    default:
        return LACAN_NO_SUCH_MSG;
    }
    return LACAN_SUCCESS;
}

//Maneja el caso de la llegada de un mensaje de error
void MainWindow::LACAN_ERR_Handler(uint16_t source,uint16_t err_cod){
    QString msg_err ="Se ha recibido un mensaje de error. \n Dispositivo: ";
    msg_err = msg_err +  QString::number(source) + "\nError: " + QString::number(err_cod) ;
    QMessageBox::warning(this,"Mensaje de Error recibido",msg_err,QMessageBox::Ok);
}

//Maneja la llegada de un mensaje del tipo Post. Por el momento solo se loguea como cualquier otro mensaje.
//Se deja este dummy para futuras implementaciones
void MainWindow::LACAN_POST_Handler(uint16_t source,uint16_t variable, data_can data){}

//Frente la llegada de un ack, esta funcion marca el estado de ack del mensaje correspondiente como recibido
void MainWindow::LACAN_ACK_Handler(uint16_t BYTE1){
    //Recorro el vector que almacena los mensajes que requieren ACK para identificar al que coincide con el
    //ACK entrante para asi cambiar su estado de ACK a recibido
    for(vector<TIMED_MSG*>::iterator it_ack=msg_ack.begin();it_ack<msg_ack.end();it_ack++){
        assert((*it_ack));
        if((*it_ack)->msg.BYTE1==BYTE1){
            (*it_ack)->ack_status=RECEIVED;
            (*it_ack)->ack_timer.start(DEAD_MSJ_ACK_TIME);//Resetea el tiempo del mensaje para borrarlo luego segun un tiempo limite
            break;
        }
    }
}

//Maneja la llegada de los HB
void MainWindow::LACAN_HB_Handler(uint16_t source){
    static uint hb_cont = 0;
    static vector<int> ignored;//Vector que se utiliza para almacenar dispositivos conectados pero que el usuario no quiere agregar a la red (en principio)
    bool devFound=false;
    bool stalkerFound=false;
    hb_cont++;
    //Recorro el vector de dispositivos conectados para encontrar el que coincide con el ID del HB entrante
    for(vector<HB_CONTROL*>::iterator it_hb=hb_con.begin();it_hb<hb_con.end();it_hb++){
        assert((*it_hb));
        if((*it_hb)->device==source){
            //Una vez encontrado, cambia el estado en caso de que haya estado inactivo y lo vuelvo a agregar a la lista de la UI
            if((*it_hb)->hb_status==INACTIVE){
                (*it_hb)->hb_status=ACTIVE;
                add_device_ui(source);
            }
            //Reseteo el timer y anuncio que el dispositivo fue encontrado y debe salirse del loop de busqueda
            (*it_hb)->hb_timer.start(DEAD_HB_TIME);
            devFound=true;
            break;
        }
    }
    //Si no se encontro el dispositivo en el vector de conectados se busca en los ignorados
    if(!devFound){
        for(vector<int>::iterator it_ig=ignored.begin();it_ig<ignored.end();it_ig++){
            if((*it_ig)==source){
                //En caso de encontrar el dispositivo en este vector se anuncia que tenemos un dispositivo ignorado insistente
                stalkerFound=true;
                break;
            }
        }
    }
    //Si no se encontro en ningun lado, se pregunta al usuario si quiere agregar este dispositivo
    if(!(devFound||stalkerFound)){
        QMessageBox *addnewdev= new QMessageBox(this);
        addnewdev->setAttribute(Qt::WA_DeleteOnClose);
        addnewdev->setIcon(QMessageBox::Question);
        addnewdev->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        addnewdev->setDefaultButton(QMessageBox::Yes);
        addnewdev->setText("Ha llegado un Heartbeat de un dispositivo"
                           " desconocido,\nDesea agregarlo a la red?\n"
                           "En caso afirmativo se le pedira que ingrese"
                           " un nombre para el mismo.");
        addnewdev->setWindowTitle("Nuevo dispositivo encontrado");
        int reply = addnewdev->exec();
        //Si el usuario acepta se lo agrega, caso contrario se lo suma a los ignorados
        switch (reply) {
        case (QMessageBox::Yes):
        {
            add_new_device(source);
            break;
        }
        case (QMessageBox::No):
        {
            ignored.push_back(source);
            break;
        }
        }
    }
}

/* *Funciones de envio de mensajes* */

//Implementacion de todas las funciones involucradas en el envio de mensajes segun del tipo que se requiera
//Cada tipo tiene una cierta cantidad de bytes de datos caracteristica, la cual depende de las funciones del mensaje,
//pero en principio el funcionamiento de cada uno es similar pudiendose identificar dos categorias de mensaje distintas
//en las cuales se agrega otra funcionalidad. Estas son marcadas por los mensajes que requieren ACK y los que no

//Envia mensajes de error a partir de un codigo de error proporcionado
int16_t MainWindow::LACAN_Error(uint16_t errCode){
    LACAN_MSG* msg=new LACAN_MSG;
    //Se arma la ID del mensaje mediante el identificador de funcion(6 bits mas significativos) y
    //el de dispositivo (5 bits menos significativos)
    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_ERR<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;  //el LACAN_ID_STANDARD_MASK es para que los 5 bits de la izquierda se pongan en 0
    msg->DLC=2;
    msg->BYTE0=LACAN_ID_BROADCAST << LACAN_BYTE0_RESERVED;//direccion de destino, corrida una cierta cantidad de bits reservados
    msg->BYTE1=uchar(errCode);//codigo de error a enviar

    //Se agrega el mensaje a la pila de envio y a la de mensajes a loguear
    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    return LACAN_SUCCESS; // si el mensaje fue correctamente enviado se devuelve success, ver implementacion de codigos de fracaso
}

//Envia mensajes de Heartbeat para anunciar a los demas dispositivos la existencia de la PC
int16_t MainWindow::LACAN_Heartbeat(){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_HB<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=1;
    msg->BYTE0=LACAN_ID_BROADCAST << LACAN_BYTE0_RESERVED;

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    return LACAN_SUCCESS;
}

//Utilizada para enviar los mensajes de ACK correspondiente a mensajes recibidos que lo requieran
int16_t MainWindow::LACAN_Acknowledge(uint16_t code, uint16_t result, uint16_t dest){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_ACK<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=3;
    msg->BYTE0=uchar(dest << LACAN_BYTE0_RESERVED);
    msg->BYTE1=uchar(code);
    msg->BYTE2=uchar(result);

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    return LACAN_SUCCESS;
}

//Encargada de enviar mensajes de POST
int16_t MainWindow::LACAN_Post(uint16_t  variable, data_can data, uint16_t dest){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_POST<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=6;
    msg->BYTE0=uchar(dest << LACAN_BYTE0_RESERVED);
    msg->BYTE1=uchar(variable);
    msg->BYTE2=uchar(data.var_char[0]);
    msg->BYTE3=uchar(data.var_char[1]);
    msg->BYTE4=uchar(data.var_char[2]);
    msg->BYTE5=uchar(data.var_char[3]);

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    return LACAN_SUCCESS;
}

int16_t MainWindow::LACAN_Set(uint16_t variable, data_can data, bool show_ack, uint16_t dest){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_SET<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=7;
    msg->BYTE0=uchar(dest << LACAN_BYTE0_RESERVED);
    msg->BYTE1=code;	//Se implementa un codigo en los mensajes que requieren un ack, asi de esta manera poder identificar a que mensaje hacen referencia
    msg->BYTE2=uchar(variable);
    msg->BYTE3=uchar(data.var_char[0]);
    msg->BYTE4=uchar(data.var_char[1]);
    msg->BYTE5=uchar(data.var_char[2]);
    msg->BYTE6=uchar(data.var_char[3]);

    //Se considera que no se acumularan nunca 250 mensajes en espera de acknowledge
    if(code>=250)
        code=0;
    else
        code++;

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    //Al requerir ACK son un tipo especial de mensaje que requiere de temporizacion para detectar la no llegada
    //del reconocimiento de llegada, en cuyo caso se considera perdido el mensaje y luego de un cierto numero de reintentos
    //pasa a considerarse un problema de conexion con el dispositivo de destino
    TIMED_MSG* new_msg= new TIMED_MSG;

    new_msg->show_miss_ack = show_ack;
    new_msg->msg=*msg;
    new_msg->ack_status=PENDACK;
    new_msg->ack_timer.setSingleShot(true);
    new_msg->ack_timer.start(WAIT_ACK_TIME);
    new_msg->retries = RETRIES;

    //Se agrega a la pila de mensajes que esperan un ACK
    assert(new_msg);
    msg_ack.push_back(new_msg);

    return LACAN_SUCCESS;
}

int16_t MainWindow::LACAN_Query(uint16_t variable, bool show_ack, uint16_t dest){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_QRY<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=3;
    msg->BYTE0=uchar(dest << LACAN_BYTE0_RESERVED);
    msg->BYTE1=code;
    msg->BYTE2=uchar(variable);

    if(code>=250)
        code=0;
    else
        code++;

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    //El envio de Queries se loguea unicamente si no se esta en la MainWindows
    //(tampoco se trackean debido a que en las demas ventanas se envian sincronicamente cada un cierto periodo)
    if(!ERflag)
    {
        TIMED_MSG* new_msg=new TIMED_MSG();

        new_msg->show_miss_ack = show_ack;
        new_msg->msg=*msg;
        new_msg->ack_status=PENDACK;
        new_msg->ack_timer.setSingleShot(true);
        new_msg->ack_timer.start(WAIT_ACK_TIME);
        new_msg->retries = RETRIES;

        assert(new_msg);
        msg_ack.push_back(new_msg);
    }

    return LACAN_SUCCESS;
}

int16_t MainWindow::LACAN_Do(uint16_t cmd, bool show_ack, uint16_t dest){
    LACAN_MSG* msg=new LACAN_MSG;

    msg->ID=(LACAN_LOCAL_ID | LACAN_FUN_DO<<LACAN_IDENT_BITS)&LACAN_ID_STANDARD_MASK;
    msg->DLC=3;
    msg->BYTE0=uchar(dest << LACAN_BYTE0_RESERVED);
    msg->BYTE1=code;
    msg->BYTE2=uchar(cmd);

    if((code)>=250)
        code=0;
    else
        code++;

    assert(msg);
    stack.push_back(msg);
    msg_log.push_back(*msg);

    TIMED_MSG* new_msg= new TIMED_MSG;

    new_msg->show_miss_ack = show_ack;
    new_msg->msg=*msg;
    new_msg->ack_status=PENDACK;
    new_msg->ack_timer.start(WAIT_ACK_TIME);
    new_msg->ack_timer.setSingleShot(true);
    new_msg->retries = RETRIES;

    assert(new_msg);
    msg_ack.push_back(new_msg);

    return LACAN_SUCCESS;
}

/***Slots***/

/***Slots Propietarios***/

//Maneja el caso de que el puerto serie arroje un error, por ejemplo, si se desconecta el cable mientras el programa
//esta funcionando
void MainWindow::handlePortError(QSerialPort::SerialPortError error){
    //Si hay algun error levanto la flag que indica que el USB se desconecto y cierro la comunicacion con el puerto

    if(error!=QSerialPort::NoError){
        //Si habia algun usb conectado antes del error muestro el cartel (puede ocurrir que la señal de error se
        //emita mas de una vez, con esto evitamos el spamming de ventanas)
        if(!NoUSB){
            QMessageBox *noConnection= new QMessageBox();
            noConnection->setAttribute(Qt::WA_DeleteOnClose);
            noConnection->setIcon(QMessageBox::Warning);
            noConnection->setStandardButtons(QMessageBox::Ok);
            noConnection->setText("Error con el puerto USB serie");
            noConnection->setWindowTitle("Ups");
            noConnection->show();
        }

        NoUSB=true;
        if(serial_port->isOpen()){
            serial_port->close();
        }
    }

}

//Encargada de verificar que todos los dispositivos de la red esten activos mediante el HB,
//cada nodo debe enviar HB cada un cierto tiempo(HB_TIME), si este no se recibe dentro de un periodo de
//tolerancia (DEAD_HB_TIME), esta funcion se encarga de que el programa lo considere inactivo
void MainWindow::verificarHB(){
    //Recorro el vector en el cual se almacenan los dispositivos de la red para verificar sus respectivos timers
    for(vector<HB_CONTROL*>::iterator it_hb=hb_con.begin(); it_hb < hb_con.end(); it_hb++){
        //Detectamos cual es el timer que emitio la señal, suponemos que si el tiempo restante en el timer es mayor
        //al timeout del timer menos 0,5 es porq el timer se acaba de reiniciar
        assert((*it_hb));
        if(((*it_hb)->hb_timer.remainingTime()> DEAD_HB_TIME-500) && ((*it_hb)->hb_status==ACTIVE)){
            //Cuando lo encuentra pasa su estado a inactivo, para el respectivo timer y borra su entrada correspondiente
            //en la lista de dispositivos conectados en la UI
            (*it_hb)->hb_status=INACTIVE;
            (*it_hb)->hb_timer.stop();
            erase_device_ui(uint16_t((*it_hb)->device));
        }
    }
}

//Esta función verifica principalmente que llegue el reconocimiento(ACK) de un mensaje enviado(SET, DO, QUERY), si el mismo
//no llega luego de un periodo de tiempo(WAIT_ACK_TIME) se setea al mensaje con la condicion ACK_TIMEOUT para que otra
//parte del programa realice las acciones correspondientes, además un mensaje que recibio su correspondiente ACK
//se sigue almacenando dentro del vector por un tiempo determinado (DEAD_MSJ_ACK_TIME) antes de que sea borrado para poder
//darle tiempo a la aplicacion de usarlo
void MainWindow::verificarACK(){

    for(vector<TIMED_MSG*>::iterator it_ack=msg_ack.begin();it_ack<msg_ack.end();it_ack++){
        //Si se recibio el ACK y el timer no esta activo (es decir ya finalizo el conteo) se borra el mensaje
        assert((*it_ack));
        if((*it_ack)->ack_status==RECEIVED){
            if(!((*it_ack)->ack_timer.isActive())){
                delete (*it_ack);
                msg_ack.erase(it_ack);
            }
        }
        else{
            //Si no se recibio ACK y el timer vencio, quiere decir que no vamos a esperar mas su llegada y tomamos acciones
             if(!((*it_ack)->ack_timer.isActive())){
                 //Cambiamos el estado del mensaje
                (*it_ack)->ack_status=ACK_TIMEOUT;
                 //Si no llega el ACK de un mensaje original, se intentaran 3 reenvios de dicho mensaje en caso de
                 //una perdida de mensajes esporadica (aunque no deberia ocurrir)
                if((*it_ack)->retries<=0){  //Si no quedan reintentos
                    //Desconectamos el mensaje de sus respectivos slots y lo borramos
                    disconnect(&(msg_ack.back()->ack_timer),SIGNAL(timeout()), this, SLOT(verificarACK()));
                    no_ACK_Handler();
                    if((*it_ack)->show_miss_ack==true){ //El mensaje se mando desde la mainwindows
                        //Se muestra una ventana para comunicarle al usuario que el dispositivo con el cual se quiere
                        //comunicar no esta disponible
                        QString name = disp_map.key(((*it_ack)->msg.BYTE0) >> LACAN_BYTE0_RESERVED);
                        QString mje = "Se ha agotado el tiempo de espera de la respuesta del " + name + ".";
                        QMessageBox::warning(this,"Error al enviar",mje,QMessageBox::Ok);
                        //El error ya fue enunciado con lo cual bajamos la flag para no mostrar nuevamente la ventana
                    }
                    delete (*it_ack);
                    msg_ack.erase(it_ack);
                }
                else{   //Si aun quedan reintentos, vuelve a enviar el mensaje y descuenta reintentos
                    serialsend(*(this->serial_port),(*it_ack)->msg);
                    //Se repite la secuencia de configuracion al igual que cuando se envia por primera vez
                    (*it_ack)->ack_status=PENDACK;
                    (*it_ack)->ack_timer.setSingleShot(true);
                    (*it_ack)->ack_timer.start(WAIT_ACK_TIME);
                    (*it_ack)->retries--;
                    this->msg_log.push_back((*it_ack)->msg);
                    this->agregar_log_sent();
                }
             }
        }
    }
}

//Es la encargada de manejar la señal del sistema que indica que hay datos por leer en el buffer del puerto serie
void MainWindow::handleProcessedMsg(LACAN_MSG msg){
    msg_log.push_back(msg);//Agrego el mensaje al vector de mensajes a loguear

    uint prevsize = hb_con.size();//Guardo la cantidad de dispositivos en la red (antes de que pueda ser modificada)

    //Si el mensaje es un POST y esta activa la ventana estado de red, emito la señal para que este
    //mensaje lo maneje esa ventana
    if((msg.ID>>LACAN_IDENT_BITS==LACAN_FUN_POST)&&ERflag){
        emit postforER_arrived(msg);
    }
    //Proceso el mensaje
    static uint16_t notsup_count, notsup_gen;
    int result;
    result=LACAN_Msg_Handler(msg,notsup_count,notsup_gen);
    //Si la cantidad de dispositivos conectados aumento, conecto la señal del timer del dispositivo agregado
    //(el ultimo) con el slot
    if(result==LACAN_SUCCESS){
        if(hb_con.size()>prevsize){
            assert(hb_con.back());
            connect(&(hb_con.back()->hb_timer),SIGNAL(timeout()),this,SLOT(verificarHB()));
        }
        //Agrego el mensaje al log de recibidos (y a su vez al txt)
        agregar_log_rec();
    }
}

//Seteo del nombre del nuevo dispositivo segun se ingreso y lo termino de agragar al procesamiento de la aplicacion
void MainWindow::add_dev_name(QString newdevname){
    //Se agrega todo normalmente siempre y cuando no se haya ingresado un nombre existente
    if(!disp_map.contains(newdevname)){
        //Al vector de verificacion de HB
        hb_con.push_back(newdev);
        //Al mapeo de nombres con IDs
        disp_map[newdevname]=newdev->device;
        //A la lista de dispositivos conectados en la UI
        ui->the_one_true_list_DESTINO->addItem(newdevname);
    }else if(disp_map[newdevname]==newdev->device){
        //Si el nombre coincide con otra entrada en el mapeo, reviso que el ID no coincida tambien (con lo cual es un
        // dispositivo existente, cuyo caso solo debo volver a agregarlo a la lista)
        ui->the_one_true_list_DESTINO->addItem(newdevname);
    }else{
        //Si el nombre existe pero la ID no, se considera un error por parte del usuario, con lo cual se vuelve a
        //repetir el proceso de agregado desde un principio
        QMessageBox::warning(this, "Error agregando dispositivo", "El nombre ingresado ya existe\n ", QMessageBox::Ok );
        add_new_device(newdev->device);
    }
}

//Se encarga de ciertas tareas que deben realizarse de manera periodica pero que no tienen una latencia tan
//critica como el envio de mensajes de la pila
void MainWindow::do_stuff(){
    static int cReconnect=0; //Usamos contadores para realizar acciones periodicas con distintos intervalos mediante un solo timer
    static int cExit=0;
    //Si el USB que conecta al adaptador no se encuentra conectado...
    if(NoUSB){
        cReconnect++;//se incrementa el contador que indica el momento de intentar una reconexion
        if(cReconnect>=6){//Luego de 6 ciclos en los cuales no se encuentra el USB...
            cExit++;//incrementamos el timer que indica que se debe salir de la aplicacion
            cReconnect=0;//Reiniciamos el contador de reconexion
            int retval = serial_port->open(QSerialPort::ReadWrite);//Intentamos abrir nuevamente el puerto
            if(retval){//Si se puede conectar con el puerto...
                //Abrimos una ventana para informarle al usuario de la reconexion
                QMessageBox *connectionRegained= new QMessageBox();
                connectionRegained->setAttribute(Qt::WA_DeleteOnClose);
                connectionRegained->setIcon(QMessageBox::Information);
                connectionRegained->setStandardButtons(QMessageBox::Ok);
                connectionRegained->setText("Se ha recuperado la conexion con el adaptador");
                connectionRegained->setWindowTitle("Conexion recuperada");
                connectionRegained->exec();
                //Reiniciamos el contador que indica la salida de la aplicacion
                cExit=0;
                //Bajamos la flag NoUSB, indicando que ya volvemos a tener conexion
                NoUSB=false;
                //Se vuelve a setear el baud del can y se envia la secuencia de inicializacion del adaptador
                uint8_t bdr=0x05;
                sendinit(*serial_port,bdr);
            }else{//Si no se pudo reconectar...
                if(cExit<5){//y todavia no se realizo el maximo de reintentos
                    //Avisa al usuario de la incapacidad para reconectar, pero indicando que se volvera a intentar
                    //un cierto numero de veces
                    QMessageBox *connectionLost= new QMessageBox();
                    connectionLost->setAttribute(Qt::WA_DeleteOnClose);
                    connectionLost->setIcon(QMessageBox::Warning);
                    connectionLost->setStandardButtons(QMessageBox::Ok);
                    connectionLost->setText("Se ha perdido la conexion con el adaptador"
                                       "\nPor favor revise el puerto USB,"
                                       "el programa intentara reconectar automaticamente "+ QString::number(5-cExit) + " veces mas.");
                    connectionLost->setWindowTitle("Error en la reconexion");
                    connectionLost->exec();
                }else{
                    //Si ya se alcanzo el numero maximo de reconexiones, se avisa al usuario y se cierra la aplicacion
                    QMessageBox::warning(this, "Ups",
                                                   "No se pudo reiniciar el puerto luego de 5 intentos,\n la aplicacion se cerrara, chau",
                                                   QMessageBox::Ok);
                    QCoreApplication::exit(1);
                }
            }
        }

    }else{
        //Si hay un USB conectado, se envia el HB como es debido, una vez por ciclo
        LACAN_Heartbeat();
    }

}

//Maneja la señal enviada por el senderThread, la cual indica el momento de enviar uno de los mensajes
//que se encuentran en la pila
void MainWindow::handleSendTimeout(){
    //Si la pila no esta vacia, enviamos el mensaje que se encuentra ultimo y luego lo borramos de la misma
    if(!stack.empty()){
        assert(stack.front());
        serialsend(*serial_port,*stack.front());
        delete stack.front();
        stack.pop_front();
    }
}

/***Qt Widget's slots***/
//NOTA GENERAL: todas las funciones on_"nombreWidget"_"evento" son slots genericos ya existentes de Qt para los widgets de la UI

//La funcionalidad de todos los slots que abren nuevas ventanas son similares, se explica unicamente el
//proceso realizado para la ventana comandar
void MainWindow::on_button_COMANDAR_clicked()
{
    //Se verifica el destino y corrobora que no sea un Boradcast (lo cual seria inconsistente con este comando)
    //antes de abrir la correspondiente ventana
    uint16_t dest = verificar_destino();
    if(dest  != LACAN_ID_BROADCAST){
        Comandar *comwin = new Comandar(this,dest);//Creamos la ventana
        comwin->setAttribute(Qt::WA_DeleteOnClose);
        comwin->setModal(true);//La hacemos modal (no se puede hacer click en otra ventana hasta cerrar esta)
        comwin->show();//Mostramos la ventana
    }
    else{
        QMessageBox::warning(this, "Error","No puede comandar a todos los dispositivos juntos",QMessageBox::Ok);
    }
}

//Ventana Consultar
void MainWindow::on_button_CONSULTAR_clicked()
{
    uint16_t dest = verificar_destino();
    Consultar *conswin = new Consultar(this,dest);
    conswin->setAttribute(Qt::WA_DeleteOnClose);
    conswin->setModal(true);
    conswin->show();
}

//Ventana Enviar Mensaje
void MainWindow::on_button_ENVIAR_MENSAJE_clicked()
{
    Enviar_Mensaje *envwin = new Enviar_Mensaje(this);
    envwin->setAttribute(Qt::WA_DeleteOnClose);
    envwin->setModal(true);
    envwin->show();
}

//Ventana Estado de red
void MainWindow::on_button_ESTADO_RED_clicked()
{
    EstadoRed *estwin = new EstadoRed(this);
    estwin->setAttribute(Qt::WA_DeleteOnClose);
    estwin->setModal(true);
    estwin->show();    
}

//Ventana Envio de mensajes
void MainWindow::on_button_ByteSend_clicked()
{
    ByteSend *bytewin = new ByteSend(this);
    bytewin->setAttribute(Qt::WA_DeleteOnClose);
    bytewin->setModal(true);
    bytewin->show();
}

void MainWindow::on_logButton_clicked()
{
    if(ui->logButton->isChecked()){
        do_log=TRUE;
    }else{
        do_log=FALSE;
    }
}

void MainWindow::on_refreshButton_clicked()
{
    QMessageBox::StandardButton reply;
    //Si ya hay algo en el log se le pregunta al usuario si desea comenzar una nueva sesion, lo cual borra la anterior
    if(outlog_cont>0 || inlog_cont>0){
        reply=QMessageBox::warning(this,"Aviso de perdida de mensajes","¿Esta seguro de comenzar una nueva sesion de registro de mensajes?\n Se borraran los mensajes de la sesion anterior",QMessageBox::Yes|QMessageBox::No);
        if(reply==QMessageBox::Yes){
            ui->tableWidget_received->clearContents();
            ui->tableWidget_sent->clearContents();
            outlog_cont=0;
            inlog_cont=0;
            msg_log.clear();
            ui->tableWidget_received->setRowCount(0);
            ui->tableWidget_sent->setRowCount(0);
        }
    }
}

void MainWindow::on_searchBar_textChanged(const QString &filter)
{
    for( int i = 0; i < ui->tableWidget_received->rowCount(); ++i )
    {
        bool match = false;
        for( int j = 0; j < ui->tableWidget_received->columnCount(); ++j )
        {
            QTableWidgetItem *item = ui->tableWidget_received->item( i, j );
            if( item->text().contains(filter, Qt::CaseInsensitive) )
            {
                match = true;
                break;
            }
        }
        ui->tableWidget_received->setRowHidden( i, !match );
    }

    for( int i = 0; i < ui->tableWidget_sent->rowCount(); ++i )
    {
        bool match = false;
        for( int j = 0; j < ui->tableWidget_sent->columnCount(); ++j )
        {
            QTableWidgetItem *item = ui->tableWidget_sent->item( i, j );
            if( item->text().contains(filter, Qt::CaseInsensitive) )
            {
                match = true;
                break;
            }
        }
        ui->tableWidget_sent->setRowHidden( i, !match );
    }
}

void MainWindow::on_sent_searchBar_textChanged(const QString &filter)
{
    filter_on_sent_searchBar(filter);
}

void MainWindow::on_received_searchBar_textChanged(const QString &filter)
{
    filter_on_rec_searchBar(filter);
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    if(arg1){
        filter_both_lists = true;
        ui->sent_searchBar->setText("");
        ui->sent_searchBar->setEnabled(false);
        ui->received_searchBar->setEnabled(false);
        ui->searchBar->setEnabled(true);
    }else{
        filter_both_lists = false;
        ui->received_searchBar->setText("");
        ui->searchBar->setText("");
        ui->sent_searchBar->setEnabled(true);
        ui->received_searchBar->setEnabled(true);
        ui->searchBar->setEnabled(false);
    }
}

void MainWindow::on_pushButton_clicked()
{
    QString file_folder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString file_folder_backup = QDir::currentPath();
    QString file_path = "/Log de Mensajes LACAN";
    QString fullpath;

    if(QDir(file_folder).exists()){
        fullpath = file_folder + file_path;
    }else{
        fullpath = file_folder_backup + file_path;
    }

    if(QDir(fullpath).exists()){
        QDesktopServices::openUrl( QUrl::fromLocalFile( fullpath ) );
    }else{
        QMessageBox::warning(this, "Error abriendo el directorio", "Todavia no han llegado mensajes para loguear, con lo cual no existe la carpeta.",QMessageBox::Ok);
    }

}

void MainWindow::on_pushButton_gen_enable_clicked()
{
    cmd_enable = LACAN_CMD_ENABLE;
    this->LACAN_Do(cmd_enable,false,LACAN_ID_BROADCAST);
    assert(msg_ack.back());
    connect(&(this->msg_ack.back()->ack_timer),SIGNAL(timeout()), this, SLOT(verificarACK()));
    this->agregar_log_sent();
}

void MainWindow::on_pushButton_gen_disable_clicked()
{
    cmd_enable = LACAN_CMD_DISABLE;
    this->LACAN_Do(cmd_enable,false,LACAN_ID_BROADCAST);
    assert(msg_ack.back());
    connect(&(this->msg_ack.back()->ack_timer),SIGNAL(timeout()), this, SLOT(verificarACK()));
    this->agregar_log_sent();

}
