/*
 * nvram_audio_init - Force-create all NVRAM items (including audio calibration)
 * using libnvram.so's NVM_GetFileDesc with ISWRITE.
 * Run once after flashing to populate /data/nvram/APCFG/APRDCL/Audio_*.
 */
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#define ISREAD  1
#define ISWRITE 0

typedef struct {
    int iFileDesc;
    int iRecordSize;
    int iRecordNum;
} F_ID;

typedef F_ID (*NVM_GetFileDesc_t)(int lid, int *recSize, int *recNum, int isRead);
typedef int  (*NVM_CloseFileDesc_t)(F_ID fid);

int main(void)
{
    void *libnvram = dlopen("libnvram.so", RTLD_NOW);
    if (!libnvram) {
        fprintf(stderr, "dlopen libnvram.so: %s\n", dlerror());
        return 1;
    }

    NVM_GetFileDesc_t  nvmGet  = (NVM_GetFileDesc_t)dlsym(libnvram, "NVM_GetFileDesc");
    NVM_CloseFileDesc_t nvmClose = (NVM_CloseFileDesc_t)dlsym(libnvram, "NVM_CloseFileDesc");
    if (!nvmGet || !nvmClose) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        return 1;
    }

    int created = 0;
    int lid;
    for (lid = 1; lid <= 200; lid++) {
        int recSize = 0, recNum = 0;
        F_ID fid = nvmGet(lid, &recSize, &recNum, ISWRITE);
        if (fid.iFileDesc >= 0) {
            printf("LID %3d: fd=%d size=%d num=%d\n", lid, fid.iFileDesc, recSize, recNum);
            nvmClose(fid);
            created++;
        }
    }
    printf("Created/opened %d NVRAM items\n", created);
    dlclose(libnvram);
    return 0;
}
