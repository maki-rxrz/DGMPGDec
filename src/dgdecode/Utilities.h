bool PutHintingData(unsigned char *video, unsigned int hint);
bool GetHintingData(unsigned char *video, unsigned int *hint);

#define HINT_INVALID 0x80000000
#define PROGRESSIVE  0x00000001
#define IN_PATTERN   0x00000002
#define COLORIMETRY  0x0000001C
#define COLORIMETRY_SHIFT  2
