#ifndef __BSP_GSENSOR_H__
#define __BSP_GSENSOR_H__

/*
    gsensor module opcode define
*/
typedef enum{
    /* Send */
    GSENSOR_OPCODE_INIT,
    GSENSOR_OPCODE_SET_NORMAL_MODE,
    GSENSOR_OPCODE_SET_WAKEUP_MODE,
    GSENSOR_OPCODE_CLOSE,

    /* Notify */
    GSENSOR_OPCODE_NTF_DATA,
    GSENSOR_OPCODE_WAKEUP,

    GSENSOR_OPCODE_LOWPOWER_WAKEUP,
}gsensor_module_opcode_t;

typedef enum{
    GSENSOR_DR_1HZ,
    GSENSOR_DR_10HZ,
    GSENSOR_DR_25HZ,
    GSENSOR_DR_50HZ,
    GSENSOR_DR_100HZ,
    GSENSOR_DR_200HZ,
    GSENSOR_DR_400HZ,
}gsensor_dr_t;

typedef enum{
    GSENSOR_MODE_NOMAL,
    GSENSOR_MODE_WAKEUP,
}gsensor_mode_t;

typedef enum{
    GSENSOR_RANGE_2G,
    GSENSOR_RANGE_4G,
    GSENSOR_RANGE_8G,
    GSENSOR_RANGE_16G,
}gsensor_range_t;

typedef struct
{
    short x;
    short y;
    short z;
}gsensor_xyz_t;

typedef struct
{
    unsigned int count;
    gsensor_xyz_t xyz[0];
}gsensor_data_t;

typedef void (*gsensor_cb)(void *handle,gsensor_data_t *data);

typedef struct{
    const char *name;
    int (*init)(void);
    void (*deinit)(void);
    int (*open)(void);
    void (*close)(void);
    int (*setDatarate)(gsensor_dr_t dr);
    int (*setMode)(gsensor_mode_t mode);
    int (*setDataRange)(gsensor_range_t rg);
    int (*registerCallback)(gsensor_cb cb);
}gsensor_device_t;

void* bk_gsensor_init(const char *devname);
void bk_gsensor_deinit(void *handle);
int bk_gsensor_open(void *handle);
void bk_gsensor_close(void *handle);
int bk_gsensor_setDatarate(void *handle,gsensor_dr_t dr);
int bk_gsensor_setMode(void *handle,gsensor_mode_t mode);
int bk_gsensor_setDateRange(void *handle,gsensor_range_t rg);
int bk_gsensor_registerCallback(void *handle,gsensor_cb cb);

#endif //__BSP_GSENSOR_H__

