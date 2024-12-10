
extern char token[256];
extern char macaddr[32];
extern char routersn[64];
extern int8_t GetToken();
extern void DoToken();
extern int8_t TokenInit();

int32_t parseIntFromData(const char *key,void *data,int *value);
int32_t parseStringFromData(const char *key,void *data,char *value);
size_t OnHttpReponseData(void* buffer, size_t size, size_t nmemb, void* lpVoid);
int32_t getSfHardwareConfig(const char *key,char *retBuffer);
void get_router_macaddr(char *mac_in);
void remote_addr_init(char* addr);
