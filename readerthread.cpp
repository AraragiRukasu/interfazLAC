#include "readerthread.h"
#include "assert.h"

ReaderThread::ReaderThread(QSerialPort& serial_port)
{
    thread_serial_port = &serial_port;
}

void ReaderThread::handleRead(){
    uint cant_msg=0, msgLeft=0;     //Cantidad de mensajes(enteros) que se extrajeron del buffer, mensajes que quedan por procesar
    uint16_t first_byte[2000]={0};    //Array de enteros que guarda la posicion del primer byte de un mensaje en pila, notar que el primer elemento de este vector siempre es 0
    static vector<char> pila;       //Vector utilizado para almacenar los bytes leidos

    try {
        //Leemos el puerto pasando la pila en la cual se guardaran los mensajes, el array donde se indica la posicion del
        //principio de cada uno, y obviamente el puerto utilizado. Devuelve la cantidad de mensajes enteros leidos
        cant_msg=readport(pila, first_byte, *thread_serial_port);
        msgLeft = cant_msg;//En un principio la cantidad de mensajes que faltan procesar es la misma que los leidos del puerto
        //Se procesa cada mensaje en cada ciclo
        for(uint i=0;i<cant_msg;i++){
            LACAN_MSG msg;
            char sub_pila[30]={0}; //Buffer para guardar los mensajes individuales (notar que tiene la longitud maxima posible de un mensaje)

            //Copio un mensaje de la pila al buffer(el primero que llego)
            for(uint j=0;j<first_byte[1];j++){
                assert(j<14);
                sub_pila[j]=pila.at(j);
            }

            //Borro el mensaje que acabo de copiar a la sub pila
            pila.erase(pila.begin()+first_byte[0],pila.begin()+first_byte[1]);

            //Reconfiguro los indices q indican el comienzo de cada mensaje (saco la longitud del proximo mensaje y se la sumo
            //al indice que indica el final del anterior)
            for(uint k=1; k<msgLeft; k++){
                first_byte[k]=first_byte[k+1]-first_byte[k]+first_byte[k-1];
            }
            msgLeft--;//Disminuyo la cantidad de mensajes que faltan procesar

            //Transformo el mensaje de bytes al tipo de dato LACAN_MSG para trabajarlo mas facilmente
            msg=mensaje_recibido(sub_pila);
            emit receivedMsg(msg);

        }
    } catch (...) {
        pila.clear();
    }


}

//Se encarga de leer el puerto, busca un nuevo dato, si ReadChar no lo encuentra regresa automaticamente
//Verifica los primeros 12bits para verificar que es un mensaje valido, no se contempla la verificacion del final del mensaje
uint ReaderThread::readport(vector<char> &pila, uint16_t* first_byte, QSerialPort& serial_port){
    qint64 newdataflag = 0;
    static char buffer[20000];
    uint cantBytes=0;
    static uint losedMsgCount = 0;
    uint16_t cant_msg = 0;
    static uint16_t index_buffer=0;
    static uint16_t dlc=0;
    first_byte[0]=0;    //esto es redundante pero fue, guarda la posicion del primer byte del proximo mensaje
    bool lastMsgIsFull = false;
    bool llegoAA = false;

    while((newdataflag=serial_port.read(buffer+index_buffer,1))==1){ //devuelve la cantidad de bytes leidos (deberia ser 1 por el limite impuesto)
        lastMsgIsFull=false;
        cantBytes++;
        index_buffer++;        //ya apunta a la siguiente
        uint current_byte = index_buffer - 1;

        assert(current_byte<20000);

        try {
            if((buffer[current_byte]&0xFF)==0xAA){
                if(current_byte-first_byte[cant_msg]!=0){
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    continue;
                }else{
                    llegoAA = true;
                    continue;
                }
            }
            else{
                if(current_byte-first_byte[cant_msg]==0){
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    continue;
                }
            }

            if((((buffer[current_byte]&0xFF)>>4)==0xC) && llegoAA){  //ultimos 4 bits de cabecera
                if(current_byte-first_byte[cant_msg]==1){
                    dlc=buffer[current_byte]&15;                //extraigo dlc
                    llegoAA = false;
                    continue;
                }
                else{
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    llegoAA = false;
                    continue;
                }
            }
            else{
                if(current_byte-first_byte[cant_msg]==1){
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    llegoAA = false;
                    continue;
                }
            }


            if(((buffer[current_byte])&0xFF)==0x55){
                if(uchar(current_byte-first_byte[cant_msg])>=uchar((dlc+5)-1)){
                    dlc=0;
                    cant_msg++;
                    lastMsgIsFull=true;
                    first_byte[cant_msg]=index_buffer;    //notar que index buffer ya apunta a la siguiente
                    continue;                                   //guardo la primer direccion del siguiente mensaje, si es que existe (current_byte ya apunta al proximo)
                }                                  //osea, si es la primera vez que entra, estoy guardando en el segundo elemento de first_byte, la posicion del 0xAA del segundo mensaje que puede llegar
                else{
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    continue;
                }
            }
            else{
                if(uchar(current_byte-first_byte[cant_msg])>=uchar((dlc+5)-1)){
                    index_buffer=first_byte[cant_msg];
                    losedMsgCount++;
                    continue;
                }
            }
        } catch (...) {
            index_buffer = first_byte[cant_msg];
        }

    }

    try {
        for(int i=(first_byte[cant_msg]-1);i>-1;i--){
            pila.push_back(buffer[first_byte[cant_msg]-1-i]);
        }

        if(lastMsgIsFull){
            index_buffer=0;
        }
        else{
            index_buffer -= first_byte[cant_msg];

            for(uint j=0;j<index_buffer;j++){
                buffer[j]=buffer[first_byte[cant_msg]+j];
            }
        }

        emit msgLost(losedMsgCount);

    } catch (...) {
        cant_msg = 0;
        index_buffer = 0;
    }

    return cant_msg;
}

LACAN_MSG ReaderThread::mensaje_recibido(char *sub_pila){
    LACAN_MSG mje;
    mje.DLC=sub_pila[1]&DLC_MASK;//Me quedo unicamente con el DLC (la primer mitad del byte es 0xC)
    //Como los bytes de ID se mandan al reves, tenemos la parte menos significativa del campo de funcion
    //en el primer byte (primeros 3 bits) y la mas significativa en el segundo (ultimos 3 bits)
    uint16_t fun=uint16_t(((sub_pila[2]&BOTTOM_FUN_MASK)>>FUN_MOV_BOTTOM)|((sub_pila[3]&UPPER_FUN_MASK)<<FUN_MOV_UPPER));
    uint16_t source=sub_pila[2]&LACAN_IDENT_MASK;
    mje.ID=uint16_t((fun<<FUN_MOV_FORSOURCE)|source);//armamos la ID de la forma en la cual esta diseñado CAN para facil entendimiento y utilizacion
    //Se almacenan los datos en la struct diseñada para el mensaje, el switch se comporta como cascada (sin breaks)
    switch(mje.DLC){
    case(8):
        mje.BYTE7=uchar(sub_pila[11]);
    [[clang::fallthrough]]; case(7):
        mje.BYTE6=uchar(sub_pila[10]);
    [[clang::fallthrough]]; case(6):
        mje.BYTE5=uchar(sub_pila[9]);
    [[clang::fallthrough]]; case(5):
        mje.BYTE4=uchar(sub_pila[8]);
    [[clang::fallthrough]]; case(4):
        mje.BYTE3=uchar(sub_pila[7]);
    [[clang::fallthrough]]; case(3):
        mje.BYTE2=uchar(sub_pila[6]);
    [[clang::fallthrough]]; case(2):
        mje.BYTE1=uchar(sub_pila[5]);
    [[clang::fallthrough]]; case(1):
        mje.BYTE0=uchar(sub_pila[4]);
    }
     return mje;
}
