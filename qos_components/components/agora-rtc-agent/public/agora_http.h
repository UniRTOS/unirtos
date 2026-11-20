#ifndef  AGORA_HTTP_H
#define  AGORA_HTTP_H

typedef enum{
    HTTP_METHOD_GET      =  1,
    HTTP_METHOD_POST     =  2,
    HTTP_METHOD_PUT      =  3,
}http_method_type_e;

typedef struct{
    int(*read_data_cb)(uint8_t **data, uint32_t  *len, void *cb_param);
    int(*write_data_cb)(const char *data, uint32_t  len, void *cb_param);
    void(*exception_cb)(void *cb_param);
}agora_http_callback_t;

int  agora_http_request(char *url, uint8_t  method, agora_http_callback_t  cb_fn, void *fn_data);

#endif//AGORA_HTTP_H