#define SDK_MAJOR_VERSION   "001" // For Major version
#define SDK_MINOR_VERSION   "017" // For minor version
#define SDK_RA_VERSION      "xxx" // For jenkins release use
#define SDK_PATCH_VERSION   "pp22.021"   // For patch verion, modify when patch release
#define EVB_MAJOR_VERSION   "1"
#define EVB_MINOR_VERSION   "0"
#if defined CHIP_EC718
#define EC_CHIP_VERSION     "EcChipVerEc718CoreAp"
#elif defined CHIP_EC716
#define EC_CHIP_VERSION     "EcChipVerEc716CoreAp"
#else
#error "need define chip to make EC_CHIP_VERSION"
#endif
