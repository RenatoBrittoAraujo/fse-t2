#ifndef MODBUS_H
#define MODBUS_H

#define DEVICE_ADDRESS 0x1
#define FUNCTION_CODE_REQUEST 0x23
#define FUNCTION_CODE_SEND 0x16

// não precisa ter todos esses tipos de req
// porque o function_code já indica se é req ou send
// mas to com preguiça de consertar
#define REQ_TYPE_REQUEST_INT 0xA1
#define REQ_TYPE_REQUEST_FLT 0xA2
#define REQ_TYPE_REQUEST_STR 0xA3
#define REQ_TYPE_SEND_INT 0xB1
#define REQ_TYPE_SEND_FLT 0xB2
#define REQ_TYPE_SEND_STR 0xB3
#define REQ_TYPE_SEND_SINGLE_BYTE 0xB4

// response type none may also mean to read and just ignore the result
#define RES_TYPE_NONE 0xC0
#define RES_TYPE_FLT 0xC1
#define RES_TYPE_INT 0xC2
#define RES_TYPE_STR 0xC3

#define SUBCODE_BYTES 1
#define DEVICE_ADDRESS_BYTES 1
#define FUNCTION_CODE_BYTES 1
#define CRC16_BYTES 2
#define REQUEST_CODE_BYTES 1
#define INT_BYTES 4
#define SINGLE_BYTE_BYTES 1
#define FLOAT_BYTES 4
#define STR_SIZE_BYTES 1
#define MATRICULA_BYTES 4

struct ModbusRequest
{
    int type;

    char *matricula;

    int int_num;
    float float_num;
    char *str;
    int str_size;

    int frame_size;

    int response_type;
    int device_address;
    int function_code;
    int subcode;
    int crc16;

    // will print all the information about the request
    int debug_print;
};
typedef struct ModbusRequest ModbusRequest;

ModbusRequest new_modbus_request(
    int device_address,
    int function_code,
    int subcode,
    int type,
    const char *matricula,
    int response_type);

void set_req_str(ModbusRequest *req, char *str);

struct ModbusResponse
{
    int type;

    int int_num;
    float float_num;
    char *str;
    int str_size;

    int subcode;
    int device_address;
    int function_code;
    int crc16;
};
typedef struct ModbusResponse ModbusResponse;

ModbusResponse *make_modbus_request(ModbusRequest *req);

#endif