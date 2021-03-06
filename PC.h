#include <iostream>
#include <QTimer>
#include <QString>
#include <stdint.h>
#include <QMap>

#ifndef PC_H_INCLUDED
#define PC_H_INCLUDED

#define LACAN_LOCAL_ID 0x01 //ID de dispositivo, en este caso de la PC
#define LACAN_FUNCTION_BITS 6 //cantidad de bits de funci n dentro del campo MSG_ID de CAN standard(11 bits en total)
#define LACAN_IDENT_BITS 5 //cantidad de bits de ID del nodo de la red dentro del campo MSG_ID de CAN standard(11 bits en total)
#define LACAN_IDENT_MASK 0x1f //mascara para extraer la ID del nodo dentro de la ID del mensaje CAN
#define LACAN_FUN_MASK (~LACAN_IDENT_MASK) //mascara para extraer la funcion del mensaje
#if (LACAN_IDENT_BITS + LACAN_FUNCTION_BITS != 11) //verificacion de la cantidad de bits en MSG_ID
    #error LACAN_IDENT_BITS + LACAN_FUNCTION_BITS != 11
#endif
#define LACAN_ID_STANDARD_MASK 0x7FF //mascara de 11 bits para poder extraer la MSG_ID
#define LACAN_BYTE0_RESERVED (8-(LACAN_IDENT_BITS)) //bits reservados dentro de un byte del mensaje donde tambien se manda la ID de dispositivo
#define LACAN_BYTE0_RESERVED_MASK (0x3) //mascara para extraer el campo reservado del byte0

// Diccionario de direcciones (ID). 5 bits (0x00 a 0x1f)
#define LACAN_ID_BROADCAST      0x00
#define LACAN_ID_MASTER         0x01
#define LACAN_ID_GEN            0x02
#define LACAN_ID_BOOST          0x03
#define LACAN_ID_VOLANTE        0x04

// Diccionario de funciones  (FUN). 6 bits (0x00 a 0x3f)
#define LACAN_FUN_ERR           0x01
#define LACAN_FUN_DO            0x04
#define LACAN_FUN_SET           0x08
#define LACAN_FUN_QRY           0x0C
#define LACAN_FUN_ACK           0x10
#define LACAN_FUN_POST          0x14
#define LACAN_FUN_HB            0x18

// Diccionario de comandos (CMD). 8 bits (0x00 a 0xff)
#define LACAN_CMD_TRIP          0x08
#define LACAN_CMD_STOP          0x0F //DESHABILITAR SALIDA
#define LACAN_CMD_RESET         0x10
#define LACAN_CMD_SHUTDOWN      0x11
#define LACAN_CMD_COUPLE        0x18
#define LACAN_CMD_START         0x1F // HABILITAR SALIDA
#define LACAN_CMD_ENABLE        0x20 // HABILITAR COMUNICACION
#define LACAN_CMD_DISABLE       0x21 // DESHABILITAR COMUNICACION
#define LACAN_CMD_DECOUPLE      0x28
#define LACAN_CMD_MAGNETIZE     0x2F

// Diccionario de variables (VAR). 8 bits (0x00 a 0xff)
#define LACAN_VAR_STATUS        0x02
#define LACAN_VAR_II_INST       0x06
#define LACAN_VAR_II_SETP       0x07
#define LACAN_VAR_IO_INST       0x0C
#define LACAN_VAR_IO_SETP       0x0D
#define LACAN_VAR_ISD_INST      0x16
#define LACAN_VAR_ISD_SETP      0x17
#define LACAN_VAR_IEF_INST      0x1C
#define LACAN_VAR_IEF_SETP      0x1D
#define LACAN_VAR_PI_INST       0x26
#define LACAN_VAR_PI_SETP       0x27
#define LACAN_VAR_PO_INST       0x2C
#define LACAN_VAR_PO_SETP       0x2D
#define LACAN_VAR_VI_INST       0x36
#define LACAN_VAR_VI_SETP       0x37
#define LACAN_VAR_VO_INST       0x3C
#define LACAN_VAR_VO_SETP       0x3D
#define LACAN_VAR_W_INST        0x46
#define LACAN_VAR_W_SETP        0x47
#define LACAN_VAR_I_BAT_INST    0x56
#define LACAN_VAR_I_BAT_SETP    0x57
#define LACAN_VAR_V_BAT_INST    0x5C
#define LACAN_VAR_V_BAT_SETP    0x5D
#define LACAN_VAR_TORQ_INST     0x6C
#define LACAN_VAR_TORQ_SETP     0x6D
//#define LACAN_VAR_ID_INST       0x66
//#define LACAN_VAR_ID_SETP       0x67
#define LACAN_VAR_STANDBY_W_INST 0x7C   //esta no existe
#define LACAN_VAR_STANDBY_W_SETP 0x7D

#define LACAN_VAR_MOD           0xC0

//Modos NOTA: estos valores corresponden al campo de datos y no al de tipo de variable en los mensajes
#define LACAN_VAR_MOD_VEL           0x00
#define LACAN_VAR_MOD_MPPT          0x01
#define LACAN_VAR_MOD_TORQ          0x02
#define LACAN_VAR_MOD_POT           0x03
#define LACAN_VAR_MOD_INER          0x04
#define LACAN_VAR_MOD_PREARRANQUE   0x05
#define LACAN_VAR_MOD_INICIO        0x06
#define LACAN_VAR_MOD_ARRANQUE      0x07
#define LACAN_VAR_MOD_COMPENSACION  0x08
#define LACAN_VAR_MOD_LIMITACION    0x09
#define LACAN_VAR_MOD_APAGADO       0x10
#define LACAN_VAR_MOD_RECUPERACION  0x11
#define LACAN_VAR_MOD_PROTEGIDO     0x12


// Diccionario de resultados (RES). 8 bits (0x00 a 0xff)
#define LACAN_RES_OK                0x00
#define LACAN_RES_MISSING_PREREQ    0x01
#define LACAN_RES_RECEIVED          0x02
#define LACAN_RES_NOT_IMPLEMENTED   0x08
#define LACAN_RES_OUT_OF_RANGE      0x10
#define LACAN_RES_BUSY              0x18
#define LACAN_RES_DENIED            0x20
#define LACAN_RES_GENERIC_FAILURE   0x28

// Diccionario de errores (ERR). 8 bits (0x00 a 0xff)
#define LACAN_ERR_GENERIC_ERR       0x00
#define LACAN_ERR_OVERVOLTAGE       0x08
#define LACAN_ERR_UNDERVOLTAGE      0x10
#define LACAN_ERR_OVERCURRENT       0x18
#define LACAN_ERR_BAT_OVERCURRENT   0x19
#define LACAN_ERR_OVERTEMPERATURE   0x20
#define LACAN_ERR_OVERSPEED         0x28
#define LACAN_ERR_UNDERSPEED        0x30
#define LACAN_ERR_NO_HEARTBEAT      0x38
#define LACAN_ERR_INTERNAL_TRIP     0x39
#define LACAN_ERR_EXTERNAL_TRIP     0x40

// C digos de respuesta del protocolo
#define LACAN_SUCCESS           0
#define LACAN_FAILURE           -1
#define LACAN_NO_SUCH_MSG       -2  //no usados en la PC, pero se debe tener en cuenta que estan reservados en los DSP
#define LACAN_NO_BOXES          -3

//Estados de acknowledge del mensaje TIMED_MSG (campo status de la struct)
#define PENDACK         0
#define RECEIVED        1
#define ACK_TIMEOUT     2

//Numero de reintentos de ack
#define RETRIES 2

//Estados de los dispositivos segun heartbeat

#define ACTIVE      true
#define INACTIVE    false




//Mascaras y corrimientos
#define DLC_MASK            15//mascara para extraer el dlc del segundo byte del mensaje
#define BOTTOM_FUN_MASK     0xE0//mascaras y corrimientos para poder armar el campo de funcion a partir de lo que llega (que esta el reves)
#define UPPER_FUN_MASK      0x07
#define FUN_MOV_BOTTOM      5
#define FUN_MOV_UPPER       3
#define FUN_MOV_FORSOURCE   5//movimiento para poder armar la ID con la source y la fun


#define HB_TIME 5000                 //en milisegundos(5 seg), es el periodo en el cual los integrantes de la red deben enviar sus HB
#define DEAD_HB_TIME HB_TIME*3+500   //tiempo que debe transcurrir desde el ultimo HB para considerar un nodo inactivo (15.5 seg)
#define DEAD_MSJ_ACK_TIME 30000      //tiempo para borrar del vector un mensaje desde que recibio su correspondiente ack
#define WAIT_ACK_TIME 3000            //tiempo de espera un ack

//union apta para manejar un vector que contiene el estado del dispositivo, es decir,
//que variables esta tomando como referencia para realizar el control (de esta manera se puede
//deducir indirectamente el modo del mismo)
union LACAN_32b_DATA{
    uint32_t uint_32b;
    int32_t int_32b;
    float float_32b;
};

union data_can{
    float    var_float;
    uchar    var_char[4];
    uint32_t var_int;
};

//Estructura a base de bits para armar el mensaje con el formato CAN standard
struct LACAN_MSG{
    uint16_t SENTIDO:1;
    uint16_t DLC:4;
    uint16_t ID:11;
    uchar BYTE0;
    uchar BYTE1;
    uchar BYTE2;
    uchar BYTE3;
    uchar BYTE4;
    uchar BYTE5;
    uchar BYTE6;
    uchar BYTE7;
};

typedef struct LACAN_MSG LACAN_MSG;

//Estructura armada para poder manejar de manera conjunta el manejo de los mensajes que esperan un ack, est
//provista del mensaje propiamente dicho, un timer para controlar los tiempos caracteristicos del ack
//y un campo de estado, las posibilidades del mismo se encuentran mas arriba como "Estados de acknowledge del mensaje TIMED_MSG"
struct  TIMED_MSG{
        LACAN_MSG msg;
        QTimer ack_timer;
        uint8_t ack_status;
        uint8_t retries;
        bool show_miss_ack;
};


typedef struct TIMED_MSG TIMED_MSG;


//Se procede con un una idea parecida al TIMED_MSG para controlar el estado de los nodos en la red, en este caso tenemos un
//identificador de dispositivo en vez de un mensaje, el temporizador y el campo de estado(m s en
//"Estados de los dispositivos segun heartbeat" arriba) cumplen las mismas funciones
struct  HB_CONTROL{
        QTimer hb_timer;
        bool hb_status;
        uint16_t device;
};

typedef struct HB_CONTROL HB_CONTROL;

struct ABSTRACTED_MSG{

    QString fun, com, dest, orig, var_type, var_val, err_code, ack_res, ack_code,curr_time;

};

struct variable {
  QString tipo;
  uint16_t id;
};

struct LACAN_VAR {
  uint16_t instantanea; //ID de instantena
  uint16_t setp;        //ID de setpoint
  double max;         //valor del limite superior
  double min;         //valor del limite inferior
};

#define SERIAL_BAUD 1228800 //bps
#define CAN_BAUD_HEXA 0x05 //opcion del adaptador
#define CAN_BAUD 250000 //bps
#define BIGGEST_CAN_MSG 13 //Bytes

#endif // PC_H_INCLUDED


