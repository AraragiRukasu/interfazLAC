#include "lacan_detect.h"

//Funciones utilizadas para transformar los valores numericos de los mensajes en su equivalente cualitativo String
//para poder loguearlo y sea entendible para el usuario

//Resultados
QString detect_res(uint8_t res){
switch(res){
case LACAN_RES_OK:
    return "OK";

case LACAN_RES_MISSING_PREREQ:
    return "MISSING_PREREQ";

case LACAN_RES_RECEIVED:
    return "RECEIVED";

case LACAN_RES_NOT_IMPLEMENTED:
    return "NOT_IMPLEMENTED";

case LACAN_RES_OUT_OF_RANGE:
    return "OUT_OF_RANGE";

case LACAN_RES_BUSY:
    return "BUSY";

case LACAN_RES_DENIED:
    return "DENIED";

case LACAN_RES_GENERIC_FAILURE:
    return "GENERIC_FAILURE";

default:
    return "No especificada/soportada";
}}

//Errores
QString detect_err(uint8_t err){
    switch (err){
        case LACAN_ERR_GENERIC_ERR:
        return "Generic Error";

        case LACAN_ERR_OVERVOLTAGE:
        return "Over Voltage";

        case LACAN_ERR_UNDERVOLTAGE:
        return "Under Voltage";

        case LACAN_ERR_OVERCURRENT:
        return "Over Current";

        case LACAN_ERR_BAT_OVERCURRENT:
        return "Bat Over Current";

        case LACAN_ERR_OVERTEMPERATURE:
        return "Over Temperatura";

        case LACAN_ERR_OVERSPEED:
        return "Over Speed";

        case LACAN_ERR_UNDERSPEED:
        return "Under Speed";

        case LACAN_ERR_NO_HEARTBEAT:
        return "No HeartBeat";

        case LACAN_ERR_INTERNAL_TRIP:
        return "Internal Trip";

        case LACAN_ERR_EXTERNAL_TRIP:
        return "External Trip";

        default:
            return "No especificada/soportada";
        }
}

//Variables
QString detect_var(uint8_t var){
       switch (var){
       case LACAN_VAR_II_INST:
           return "Corriente de Entrada Instantanea";

       case LACAN_VAR_II_SETP:
           return "Corriente de Entrada SetPoint";

       case LACAN_VAR_IO_INST:
           return "Corriente de Salida Instantanea";

       case LACAN_VAR_IO_SETP:
           return "Corriente de Salida SetPoint";

       case LACAN_VAR_ISD_INST:
           return "Corriente de ISD Instantanea";

       case LACAN_VAR_ISD_SETP:
           return "Corriente de ISD SetPoint";

       case LACAN_VAR_IEF_INST:
           return "Corriente Eficaz Instantanea";

       case LACAN_VAR_IEF_SETP:
           return "Corriente Eficaz SetPoint";

       case LACAN_VAR_PI_INST:
           return "Potencia de Entrada Instantanea";

       case LACAN_VAR_PI_SETP:
           return "Potencia de Entrada SetPoint";

       case LACAN_VAR_PO_INST:
           return "Potencia de Salida Instantanea";

       case LACAN_VAR_PO_SETP:
           return "Potencia de Salida SetPoint";

       case LACAN_VAR_VI_INST:
           return "Tension de Entrada Instantenea";

       case LACAN_VAR_VI_SETP:
           return "Tension de Entrada SetPoint";

       case LACAN_VAR_VO_INST:
           return "Tension de Salida Instantenea";

       case LACAN_VAR_VO_SETP:
           return "Tension de Salida SetPoint";

       case LACAN_VAR_W_INST:
           return "Velocidad Angular Instantanea";

       case LACAN_VAR_W_SETP:
           return "Velocidad Angular SetPoint";

       case LACAN_VAR_TORQ_INST:
           return "Torque Instantaneo";

       case LACAN_VAR_TORQ_SETP:
           return "Torque SetPoint";

       case LACAN_VAR_I_BAT_INST:
           return "Corriente de Bateria Instantanea";

       case LACAN_VAR_I_BAT_SETP:
           return "Corriente de Bateria SetPoint";

       case LACAN_VAR_V_BAT_INST:
           return "Tension de Bateria Instantanea";

       case LACAN_VAR_V_BAT_SETP:
           return "Tension de Bateria SetPoint";

       case LACAN_VAR_STANDBY_W_INST:
           return "Velocidad StandBy Instantanea";

       case LACAN_VAR_STANDBY_W_SETP:
           return "Velocidad StandBy SetPoint";

       case LACAN_VAR_MOD:
           return "Modo";

        default:
            return "No especificada/soportada";
        }

}

//Comandos
QString detect_cmd(uint8_t cmd){
    switch(cmd){
    case LACAN_CMD_TRIP:
        return "Trip";

    case LACAN_CMD_COUPLE:
        return "Couple";

    case LACAN_CMD_DECOUPLE:
        return "Decouple";

    case LACAN_CMD_MAGNETIZE:
        return "Magnetize";

    case LACAN_CMD_RESET:
        return "Reset";

    case LACAN_CMD_START:
        return "Start";

    case LACAN_CMD_STOP:
        return "Stop";

    case LACAN_CMD_SHUTDOWN:
        return "Shutdown";

    case LACAN_CMD_ENABLE:
        return "Enable";

    case LACAN_CMD_DISABLE:
        return "Disable";

    default:
        return "No especificada o soportada";
    }
}

//Modo
QString detect_mode(uint16_t modo){
    switch (modo){
    case LACAN_VAR_MOD_POT:
        return "Modo Potencia";

    case LACAN_VAR_MOD_VEL:
        return "Modo Velocidad";

    case LACAN_VAR_MOD_TORQ:
        return "Modo Torque";

    case LACAN_VAR_MOD_MPPT:
        return "Modo MPPT";

    case LACAN_VAR_MOD_INER:
        return "Modo ";

    case LACAN_VAR_MOD_PREARRANQUE:
        return "Modo Prearranque";

    case LACAN_VAR_MOD_INICIO:
        return "Modo Inicio";

    case LACAN_VAR_MOD_ARRANQUE:
        return "Modo Arranque";

    case LACAN_VAR_MOD_COMPENSACION:
        return "Modo Compensacion";

    case LACAN_VAR_MOD_LIMITACION:
        return "Modo Limitacion";

    case LACAN_VAR_MOD_APAGADO:
        return "Modo Apagado";

    case LACAN_VAR_MOD_RECUPERACION:
        return "Modo Recuperacion";

    case LACAN_VAR_MOD_PROTEGIDO:
        return "Modo Protegido";

    default:
        return "No especificado/soportado";

    }
}


