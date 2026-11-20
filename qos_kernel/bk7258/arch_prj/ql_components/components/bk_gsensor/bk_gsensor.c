#include <string.h>
#include <components/bk_gsensor.h>

extern const gsensor_device_t gs_sc7a20;
static const gsensor_device_t * const gsensor_devices[] = {
    &gs_sc7a20,
    NULL
};
void* bk_gsensor_init(const char *devname) {
    if (devname == NULL) {
        return NULL;
    }
    for (size_t i = 0; gsensor_devices[i] != NULL; i++) {
        if (gsensor_devices[i]->name && strcmp(gsensor_devices[i]->name, devname) == 0) {
            if (gsensor_devices[i]->init) {
                if (gsensor_devices[i]->init() != 0) {
                    return NULL;
                }
            }
            return (void*)gsensor_devices[i];
        }
    }
    return NULL;
}

void bk_gsensor_deinit(void *handle)
{
    if(handle == 0) return;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->deinit)
        dev->deinit();
}

int bk_gsensor_open(void *handle)
{
    if(handle == 0) return -1;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->open)
        return dev->open();
    return 0;
}

void bk_gsensor_close(void *handle)
{
    if(handle == 0) return;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->close)
        dev->close();
}

int bk_gsensor_setDatarate(void *handle,gsensor_dr_t dr)
{
    if(handle == 0) return -1;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->setDatarate)
        return dev->setDatarate(dr);
    return 0;
}

int bk_gsensor_setMode(void *handle,gsensor_mode_t mode)
{
    if(handle == 0) return -1;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->setMode)
        return dev->setMode(mode);
    return 0;
}

int bk_gsensor_setDateRange(void *handle,gsensor_range_t rg)
{
    if(handle == 0) return -1;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->setDataRange)
        return dev->setDataRange(rg);
    return 0;
}

int bk_gsensor_registerCallback(void *handle,gsensor_cb cb)
{
    if(handle == 0) return -1;
    gsensor_device_t *dev = (gsensor_device_t*)handle;
    if(dev->registerCallback)
        return dev->registerCallback(cb);
    return 0;
}

