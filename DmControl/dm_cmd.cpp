#include <stdio.h>
#include <unistd.h>

//#define LOG_NDEBUG 0   //Open ALOGV
#define LOG_TAG "DmCmd"
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <IDisplayManager.h>
#include <libDisplayManagerClient.h>

using namespace android;

enum {
    DMC_GETDISPLAYCONFIGS=0,
    DMC_SETACTIVECONFIG,
};

void Usage(int /* argc */, char** argv)
{
    printf("%s <opetion>\n", argv[0]);
    printf("    d : GetDisplayConfigs()\n");
    printf("    s<index> : setActiveConfig()\n");
}

int main(int argc, char** argv)
{
    int result;
    int32_t ret = 0;
    sp<IDisplayManager> DisplayManagerLocal;
    int32_t usage = 0;
    int32_t cmd = -1;
    int32_t index = -1;

    while((result=getopt(argc,argv,"ds:h"))!=-1){
        switch(result){
            case 'd': // GetDisplayConfigs
                cmd = DMC_GETDISPLAYCONFIGS;
                break;
            case 's':
                cmd = DMC_SETACTIVECONFIG;
                index = atoi(optarg);
                break;
            case '?':
            case 'h':
            default:
                usage = 1;
                break;
        }
    }

    if (usage == 1) {
        Usage(argc, argv);
        return 0;
    }

    // create a client to DisplayManager
    DisplayManagerLocal = getDemoServ();

    ALOGE_IF(DisplayManagerLocal == 0, "DisplayManagerLocal is null");

    switch (cmd) {
        case DMC_GETDISPLAYCONFIGS:
            {
                int ret = 0;
                int num_config = 0;
                dm_display_attribute Configs[16];
                dm_display_attribute *ptr = &Configs[0];
                ret = DisplayManagerLocal->GetDisplayConfigs(&num_config, &ptr);
                printf("GetDisplayConfigs()\n");
                if (ret != 0) {
                    printf("Error : line %d ret = %d", __LINE__, ret);
                    break;
                }
                printf("  NumOfConfig : %d\n", num_config);
                for (int i=0; i<num_config; i++) {
                    printf("    [%d] VBYONE = (%d, %d) : LOSD = (%d, %d) %f fps\n",
                        i,
                        Configs[i].xres, Configs[i].yres,
                        Configs[i].losd_xres, Configs[i].losd_yres,
                        Configs[i].fps
                    );
                }
            }
            break;
        case DMC_SETACTIVECONFIG:
            {
                int ret = 0;
                printf("setActiveConfig(%d)\n", index);
                if (index < 0) {
                    ret = index;
                    printf("Error : line %d ret = %d\n", __LINE__, ret);
                    break;
                }
                ret = DisplayManagerLocal->setActiveConfig(index);
                if (ret != 0) {
                    printf("Error : line %d ret = %d\n", __LINE__, ret);
                    break;
                }
            }
            break;
        default:
            break;
    }

    ALOGE_IF(ret != 0, "ret = %d", ret);

    return 0;
}
