#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_log.h"
#include "mongoose.h"
#include "agora_http.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

#define HTTP_CLIENT_TASK_PRIORITY        5
#define HTTP_CLIENT_TASK_STACK_SIZE      8*1024

typedef struct{
    char                  url[256];
    uint8_t               method;
    agora_http_callback_t  cb_fn;
    void                 *cb_param;
    uint8_t               continue_send;
    uint8_t               done;
    int8_t                result;
}agora_http_session_t;

static qosa_task_t        http_thread;

static void agora_http_client_cb_fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    agora_http_session_t *session = (agora_http_session_t*)fn_data;
    // QLOGI("ev:%d, url:%s\n", ev, session->url);
    if (ev == MG_EV_OPEN) {
        
    }else if(ev == MG_EV_POLL){

    }else if (ev == MG_EV_CONNECT) {
        struct mg_str host = mg_url_host(session->url);
        if (mg_url_is_ssl(session->url)) {
            struct mg_tls_opts opts = {.ca = NULL, .cert = NULL, .certkey = NULL, .srvname = host};
            mg_tls_init(c, &opts);
        }else{
            if(session->cb_fn.read_data_cb != NULL){
                uint8_t *request_data = NULL;
                uint32_t  request_len = 0;
                if(session->cb_fn.read_data_cb(&request_data, &request_len, session->cb_param) == -2){
                    session->continue_send = 1;
                }else{
                    session->continue_send = 0;
                }
                if(request_data != NULL){
                    QLOGD("%s\n",(char *)request_data);
                    mg_send(c, request_data, request_len);
                    free(request_data);
                }            
            }
        }        
    }else if(ev == MG_EV_TLS_HS){
        if(session->cb_fn.read_data_cb != NULL){
            uint8_t *request_data = NULL;
            uint32_t  request_len = 0;
            if(session->cb_fn.read_data_cb(&request_data, &request_len, session->cb_param) == -2){
                session->continue_send = 1;
            }else{
                session->continue_send = 0;
            }
            if(request_data != NULL){
                QLOGD("\n%s\n",(char *)request_data);
                mg_send(c, request_data, request_len);
                free(request_data);
            }            
        }
    }else if(ev == MG_EV_WRITE){
        if(session->continue_send){
            if(session->cb_fn.read_data_cb != NULL){
                uint8_t *request_data = NULL;
                uint32_t  request_len = 0;
                if(session->cb_fn.read_data_cb(&request_data, &request_len, session->cb_param) == -2){
                    session->continue_send = 1;
                }else{
                    session->continue_send = 0;
                }
                if(request_data != NULL){
                    mg_send(c, request_data, request_len);
                    free(request_data);
                }            
            }
        }
    }else if(ev == MG_EV_HTTP_CHUNK){
		// !chunk reception cause json data truncation
		/*
        struct mg_http_message* hm = (struct mg_http_message*) ev_data;
        QLOGI("len:%d\n",hm->chunk.len);
        if (hm->chunk.len == 0) {
            c->is_closing = 1;         // Tell mongoose to close this connection
            session->done = true;  // Tell event loop to stop
            return;
        }
        if (mg_http_status(hm) == 200) {
            if(session->cb_fn.write_data_cb != NULL){
                if(session->cb_fn.write_data_cb(hm->chunk.ptr, (uint32_t)hm->chunk.len, session->cb_param) == 0){
                    session->result = 1;
                }
           }
        }
        mg_http_delete_chunk(c, hm);
		*/
    }else if (ev == MG_EV_HTTP_MSG){
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
		if (mg_http_status(hm) == 200) {
            if(session->cb_fn.write_data_cb != NULL){
                if(session->cb_fn.write_data_cb(hm->body.ptr, (uint32_t)hm->body.len, session->cb_param) == 0){
                    session->result = 1;
                }
           }
        }
    }else if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE){
        session->done = true; 
    } 
}


// static int http_client_perform(http_session_t  *sess_ptr)
// {
//     struct mg_mgr       https_mgr;
//     struct mg_connection *http_client = NULL;

//     mg_mgr_init(&https_mgr);        // Initialise event manager
//     http_client = mg_http_connect(&https_mgr, sess_ptr->url, http_client_cb_fn, sess_ptr);
    
//     while(http_client &&  sess_ptr->done == false){
//         mg_mgr_poll(&https_mgr, 1000);
//     }
    
//     mg_mgr_free(&https_mgr);
//     http_client = NULL;
//     return sess_ptr->result;
// }

static void agora_http_client_main(beken_thread_arg_t data)
{
    agora_http_session_t  *http_sess_ptr = (agora_http_session_t  *)data;
    struct mg_mgr       https_mgr;
    struct mg_connection *http_client = QOSA_NULL;

    mg_mgr_init(&https_mgr);        // Initialise event manager
    http_client = mg_http_connect(&https_mgr, http_sess_ptr->url, agora_http_client_cb_fn, http_sess_ptr);
    
    while(http_client &&  http_sess_ptr->done == QOSA_FALSE){
        mg_mgr_poll(&https_mgr, 1000);
    }
    
    mg_mgr_free(&https_mgr);
    http_client = QOSA_NULL;
    if(!http_sess_ptr->result){
        if(http_sess_ptr->cb_fn.exception_cb != QOSA_NULL)
            http_sess_ptr->cb_fn.exception_cb(http_sess_ptr->cb_param);
    }   
    QLOGI("http client thread exit %u\n", http_sess_ptr->result);
    qosa_free(http_sess_ptr);
    qosa_task_delete(QOSA_NULL);
}

int  agora_http_request(char *url, uint8_t  method, agora_http_callback_t  cb_fn, void *fn_data)
{
    agora_http_session_t  *http_sess_ptr = QOSA_NULL;

    if(url == QOSA_NULL || !(method >= HTTP_METHOD_GET && method <= HTTP_METHOD_PUT)||cb_fn.read_data_cb == QOSA_NULL){
        return -1;
    }
    QLOGI("url:%s\n", url);
    http_sess_ptr = qosa_calloc(1, sizeof(agora_http_session_t));

    if(http_sess_ptr == QOSA_NULL){
        return -1;
    }
    http_sess_ptr->method = method;
    qosa_strcpy(http_sess_ptr->url, url);
    http_sess_ptr->cb_fn.read_data_cb = cb_fn.read_data_cb;
    http_sess_ptr->cb_fn.write_data_cb = cb_fn.write_data_cb;
    http_sess_ptr->cb_fn.exception_cb = cb_fn.exception_cb;
    http_sess_ptr->cb_param = fn_data;

    int ret = qosa_task_create(&http_thread, 
                                HTTP_CLIENT_TASK_STACK_SIZE,
                                HTTP_CLIENT_TASK_PRIORITY, 
                                "agora_http_client", 
                                agora_http_client_main,
                                http_sess_ptr);
    // ret = http_client_perform(http_sess_ptr);

    // free(http_sess_ptr);
    if(ret != QOSA_ERROR_OK){
        qosa_free(http_sess_ptr);
        return -1;
    }
    return 0;
}