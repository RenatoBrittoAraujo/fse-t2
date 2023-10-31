#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <endian.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <arpa/inet.h>

#include <shared/inc/time.h>
#include <shared/inc/garbage_collector.h>
#include <shared/inc/shared_util.h>
#include <main/inc/modbus.h>

#define FRAME_SIZE 1000
#define WAIT_REPONSE_MS 150000 // microsegundos
#define READ_AGAIN_WAIT 30000  // microsegundos

int debug_print = 0;

// COMM

struct CommState
{
    int uart_filestream;
};
typedef struct CommState CommState;

CommState *create_comm_state();

void write_request(CommState *comm_state, char *message, size_t message_size);
char *read_response(CommState *comm_state, int *size, int tries);

// PROTOCOL

ModbusRequest *make_request(int type, int inum, float fnum, char *sstr);
char *parse_request(ModbusRequest *req);
ModbusResponse *parse_response(ModbusRequest *res, char *res_str);
void print_request(ModbusRequest *req);
void print_response(ModbusResponse *res);
void print_bits(char *str, size_t size);
short CRC16(short crc, char data);
short calcula_CRC(char *commands, int size);

// YOU NEED TO FREE THE REQUEST AND RESPONSE POINTER
ModbusResponse *make_modbus_request(ModbusRequest *req)
{
    debug_print = req->debug_print;

    CommState *comm_state = create_comm_state();

    if (debug_print)
        print_request(req);

    char *req_str = parse_request(req);

    if (debug_print)
        print_bits(req_str, req->frame_size);

    write_request(comm_state, req_str, req->frame_size);

    usleep(WAIT_REPONSE_MS);

    char *res_str = read_response(comm_state, NULL, 0);

    if (res_str != NULL)
    {
        if (debug_print)
            log_print("got response string (req_str not null)\n", LEVEL_DEBUG);

        ModbusResponse *res = parse_response(req, res_str);
        if (res != NULL)
        {
            if (debug_print)
                log_print("got response\n", LEVEL_DEBUG);

            if (debug_print)
                print_response(res);
            return res;
        }
    }

    if (debug_print)
        log_print("no response string\n", LEVEL_DEBUG);
    return NULL;
}

CommState *create_comm_state()
{
    CommState *comm_state = (CommState *)g_malloc(sizeof(CommState));

    {
        int uart0_filestream = -1;

        uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY); // Open in non blocking read/write mode
        if (uart0_filestream == -1)
        {
            if (debug_print)
                log_print("Erro - Não foi possível iniciar a UART", LEVEL_ERROR);
        }
        else
        {
            add_file_to_garbage_collector(uart0_filestream);
        }

        struct termios options;
        tcgetattr(uart0_filestream, &options);
        options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; //<Set baud rate
        options.c_iflag = IGNPAR;
        options.c_oflag = 0;
        options.c_lflag = 0;
        tcflush(uart0_filestream, TCIFLUSH);
        tcsetattr(uart0_filestream, TCSANOW, &options);

        comm_state->uart_filestream = uart0_filestream;
    }

    return comm_state;
}

void write_request(CommState *comm_state, char *message, size_t message_size)
{
    unsigned char tx_buffer[FRAME_SIZE];
    unsigned char *p_tx_buffer = (unsigned char *)&tx_buffer;

    {
        for (int i = 0; i < message_size; i++)
        {
            *p_tx_buffer++ = message[i];
        }

        if (comm_state->uart_filestream != -1)
        {
            int count = write(comm_state->uart_filestream, &tx_buffer, message_size);
            if (count <= 0)
            {
                if (debug_print)
                    log_print("UART TX error\n", LEVEL_ERROR);
            }
        }
    }
}

// size is optional, may be null
char *read_response(CommState *comm_state, int *size, int tries)
{
    if (comm_state->uart_filestream != -1)
    {
        // Read up to 255 characters from the port if they are there
        unsigned char *rx_buffer = (unsigned char *)g_malloc(256);
        int rx_length = read(comm_state->uart_filestream, (void *)rx_buffer, 255); // Filestream, buffer to store in, number of bytes to read (max)

        if (rx_length <= 0)
        {
            if (debug_print)
                log_print("Nenhum dado disponível...\n", LEVEL_INFO);
            if (tries > 0)
            {
                if (debug_print)
                    log_print("Lendo de novo...\n", LEVEL_INFO);
                usleep(READ_AGAIN_WAIT);
                return read_response(comm_state, size, tries - 1);
            }
            if (debug_print)
                log_print("Retorno sem reposta\n", LEVEL_INFO);
            return NULL;
        }

        // Bytes received
        rx_buffer[rx_length] = '\0';
        if (debug_print)
        {
            printf("Resposta recebida, %i bytes lidos\n", rx_length);
            printf("Resposta string = '%s'\n", &rx_buffer[1]);
            print_bits(rx_buffer, rx_length);
        }
        if (size != NULL)
        {
            *size = rx_length;
        }
        return rx_buffer;
    }
    return NULL;
}

// send -1 if number not applicable, null if str not applicable
ModbusRequest *make_request(int type, int inum, float fnum, char *sstr)
{
    ModbusRequest *req = (ModbusRequest *)g_malloc(sizeof(ModbusRequest));
    req->type = type;
    req->device_address = DEVICE_ADDRESS;

    // overwrite below if needed
    req->function_code = FUNCTION_CODE_SEND;

    switch (type)
    {
    case REQ_TYPE_REQUEST_FLT:
    case REQ_TYPE_REQUEST_INT:
    case REQ_TYPE_REQUEST_STR:
        req->function_code = FUNCTION_CODE_REQUEST;
        break;
    case REQ_TYPE_SEND_FLT:
        req->float_num = fnum;
        break;
    case REQ_TYPE_SEND_INT:
    case REQ_TYPE_SEND_SINGLE_BYTE:
        req->int_num = inum;
        break;
    case REQ_TYPE_SEND_STR:
        req->str = sstr;
        req->str_size = strlen(sstr);
        break;
    default:
        if (debug_print)
            printf("ERROR: invalid type '%d' for request\n", type);
        return NULL;
    }

    return req;
}

char *parse_request(ModbusRequest *req)
{
    int req_size = 0;
    req_size += DEVICE_ADDRESS_BYTES + FUNCTION_CODE_BYTES + REQUEST_CODE_BYTES + CRC16_BYTES + MATRICULA_BYTES;

    switch (req->type)
    {
    case REQ_TYPE_REQUEST_FLT:
    case REQ_TYPE_REQUEST_INT:
    case REQ_TYPE_REQUEST_STR:
        break;
    case REQ_TYPE_SEND_STR:
        req_size += STR_SIZE_BYTES + req->str_size;
        break;
    case REQ_TYPE_SEND_INT:
        req_size += INT_BYTES;
        break;
    case REQ_TYPE_SEND_SINGLE_BYTE:
        req_size += SINGLE_BYTE_BYTES;
        break;
    case REQ_TYPE_SEND_FLT:
        req_size += FLOAT_BYTES;
        break;
    default:
        if (debug_print)
            printf("ERROR: invalid type '%d' for request\n", req->type);
        return NULL;
    }

    char *req_str = g_malloc(sizeof(char) * (req_size + 1));
    int req_pos = 0;

    memcpy(&req_str[req_pos], &req->device_address, DEVICE_ADDRESS_BYTES);
    req_pos += DEVICE_ADDRESS_BYTES;

    memcpy(&req_str[req_pos], &req->function_code, FUNCTION_CODE_BYTES);
    req_pos += FUNCTION_CODE_BYTES;

    memcpy(&req_str[req_pos], &req->subcode, SUBCODE_BYTES);
    req_pos += SUBCODE_BYTES;

    req->matricula[0] -= '0';
    req->matricula[1] -= '0';
    req->matricula[2] -= '0';
    req->matricula[3] -= '0';
    memcpy(&req_str[req_pos], req->matricula, MATRICULA_BYTES);
    req_pos += MATRICULA_BYTES;

    // convert to server byte order
    req->int_num = htonl(req->int_num);
    req->float_num = htonl(req->float_num);

    int v;

    switch (req->type)
    {
    case REQ_TYPE_REQUEST_FLT:
    case REQ_TYPE_REQUEST_INT:
    case REQ_TYPE_REQUEST_STR:
        break;
    case REQ_TYPE_SEND_STR:
        memcpy(&req_str[req_pos], &req->str_size, STR_SIZE_BYTES);
        req_pos += STR_SIZE_BYTES;
        memcpy(&req_str[req_pos], req->str, req->str_size);
        req_pos += req->str_size;
        break;
    case REQ_TYPE_SEND_INT:
        memcpy(&req_str[req_pos], &req->int_num, INT_BYTES);
        req_pos += INT_BYTES;
        break;
    case REQ_TYPE_SEND_SINGLE_BYTE:
        htonl(req->int_num);
        memcpy(&req_str[req_pos], &v, SINGLE_BYTE_BYTES);
        req_pos += SINGLE_BYTE_BYTES;
        break;
    case REQ_TYPE_SEND_FLT:
        memcpy(&req_str[req_pos], &req->float_num, FLOAT_BYTES);
        req_pos += FLOAT_BYTES;
        break;
    default:
        if (debug_print)
            printf("ERROR: invalid type '%d' for request\n", req->type);
        return NULL;
    }

    short crc = calcula_CRC(req_str, req_pos);
    if (debug_print)
        printf("CRC CALCULADO: %d (%x)\n", crc, crc);

    memcpy(&req_str[req_pos], &crc, CRC16_BYTES);
    req_pos += CRC16_BYTES;

    if (req_pos != req_size)
    {
        if (debug_print)
            printf("VALIDATION ERROR: wrote invalid number of chars, expected %d wrote %d\n", req_size + 1, req_pos + 1);
    }

    req->frame_size = req_pos;

    return req_str;
}

ModbusResponse *parse_response(ModbusRequest *req, char *res_str)
{
    if (req->response_type == RES_TYPE_NONE)
        return NULL;

    ModbusResponse *res = (ModbusResponse *)malloc(sizeof(ModbusResponse));

    int res_pos = 0;
    memcpy(&res->device_address, &res_str[res_pos], DEVICE_ADDRESS_BYTES);
    res_pos += DEVICE_ADDRESS_BYTES;

    memcpy(&res->function_code, &res_str[res_pos], FUNCTION_CODE_BYTES);
    res_pos += FUNCTION_CODE_BYTES;

    memcpy(&res->subcode, &res_str[res_pos], REQUEST_CODE_BYTES);
    res_pos += REQUEST_CODE_BYTES;

    res->type = req->response_type;

    size_t str_size = 0;
    switch (res->type)
    {
    case RES_TYPE_FLT:
        memcpy(&res->float_num, &res_str[res_pos], FLOAT_BYTES);
        res_pos += FLOAT_BYTES;
        break;
    case RES_TYPE_INT:
        memcpy(&res->int_num, &res_str[res_pos], INT_BYTES);
        res_pos += INT_BYTES;
        break;
    case RES_TYPE_STR:
        memcpy(&str_size, &res_str[res_pos], STR_SIZE_BYTES * sizeof(char));
        res->str = g_malloc(sizeof(char) * (str_size + 1));
        res->str_size = str_size;
        res_pos += STR_SIZE_BYTES;
        memcpy(res->str, &res_str[res_pos], str_size);
        res_pos += str_size;
        res->str[str_size] = '\0';
        break;
    case RES_TYPE_NONE:
        return NULL;
    default:
        if (debug_print)
            printf("ERROR: invalid request type '%d' for response\n", res->type);
        return NULL;
    }

    char *validation_str = malloc(sizeof(char) * (res_pos + 1));
    memcpy(validation_str, res_str, res_pos);
    validation_str[res_pos] = '\0';

    short crc = calcula_CRC(res_str, res_pos);

    short msg_crc;
    memcpy(&msg_crc, &res_str[res_pos], CRC16_BYTES);
    res_pos += CRC16_BYTES;

    if (crc != msg_crc)
    {
        if (debug_print)
            printf("ERROR: response CRC does not match, expected %d got %d\n", crc, msg_crc);
        return NULL;
    }

    // convert to our byte order
    res->int_num = ntohl(res->int_num);
    res->float_num = ntohl(res->float_num);

    return res;
}

void print_request(ModbusRequest *req)
{
    if (!debug_print)
        return;

    printf("request: ");
    switch (req->type)
    {
    case REQ_TYPE_REQUEST_FLT:
        printf("requesting float");
        break;
    case REQ_TYPE_REQUEST_INT:
        printf("requesting int");
        break;
    case REQ_TYPE_REQUEST_STR:
        printf("requesting str");
        break;
    case REQ_TYPE_SEND_FLT:
        printf("send float: %f", req->float_num);
        break;
    case REQ_TYPE_SEND_INT:
        printf("send int: %d", req->int_num);
        break;
    case REQ_TYPE_SEND_STR:
        printf("send str (size %d): '%s'", req->str_size, req->str);
        break;
    case REQ_TYPE_SEND_SINGLE_BYTE:
        printf("send single byte: %d", req->int_num);
        break;
    default:
        printf("invalid type '%d' for request", req->type);
    }
    printf("\n");
}

void print_response(ModbusResponse *res)
{
    if (!debug_print)
        return;

    printf("response: ");
    switch (res->type)
    {
    case REQ_TYPE_REQUEST_FLT:
        printf("float: %f", res->float_num);
        break;
    case RES_TYPE_INT:
        printf("int: %d", res->int_num);
        break;
    case RES_TYPE_STR:
        printf("str (size %d): '%s'", res->str_size, res->str);
        break;
    case RES_TYPE_NONE:
        printf("response is not expected");
        break;
    default:
        printf("invalid type '%d' for response", res->type);
    }
    printf("\n");
}

void print_bits(char *str, size_t size)
{
    if (!debug_print)
        return;

    for (size_t i = 0; i < size * 8; i++)
    {
        int d = i / 8, r = i % 8;
        if (r == 0)
            printf("0x%x \t| ", str[d]);
        printf("%d", !!(str[d] & (1 << r)));
        if (r == 7)
            printf("\n");
    }
    printf("\n");
}

short CRC16(short crc, char data)
{
    const short tbl[256] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040};
    return ((crc & 0xFF00) >> 8) ^ tbl[(crc & 0x00FF) ^ (data & 0x00FF)];
}

short calcula_CRC(char *commands, int size)
{
    int i;
    short crc = 0;
    for (i = 0; i < size; i++)
    {
        crc = CRC16(crc, commands[i]);
    }
    return crc;
}

ModbusRequest new_modbus_request(int device_address, int function_code, int subcode, int type, const char *matricula, int response_type)
{
    ModbusRequest req;

    req.device_address = device_address;
    req.function_code = function_code;
    req.subcode = subcode;
    req.type = type;
    req.response_type = response_type;

    req.matricula = malloc(sizeof(char) * 4);
    memcpy(req.matricula, matricula, 4);

    req.str = malloc(sizeof(char) * 1000);
    req.str_size = 0;
    req.int_num = 0;
    req.float_num = 0;

    req.frame_size = 0;
    req.crc16 = 0;

    req.debug_print = 0;

    return req;
}

void set_req_str(ModbusRequest *req, char *str)
{
    strcpy(req->str, str);
    req->str_size = strlen(str);
}

void test_modbus()
{
    create_garbage_collector();

    int req_count = 1;
    ModbusRequest *reqs[] = {
        make_request(
            REQ_TYPE_REQUEST_FLT,
            -1, -1, NULL),
        make_request(
            REQ_TYPE_REQUEST_STR,
            -1, -1, NULL),
        make_request(
            REQ_TYPE_REQUEST_INT,
            -1, -1, NULL),
        make_request(
            REQ_TYPE_SEND_INT,
            17, -1, NULL),
        make_request(
            REQ_TYPE_SEND_FLT,
            -1, 69.420, NULL),
        make_request(
            REQ_TYPE_SEND_STR,
            -1, -1, "imposto eh roubo"),
    };

    for (int i = 0; i < req_count && debug_print; i++)
    {
        printf("--------------\n");
        make_modbus_request(reqs[i]);
    }

    cleanup_garbage(NULL);
}