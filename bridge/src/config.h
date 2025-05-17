#ifndef __EW_TO_APRS_CONFIG_PARSER__
#define __EW_TO_APRS_CONFIG_PARSER__

extern char APRS_CALLSIGN_SSID[32];
extern char APRS_PASSCODE[32];
extern char APRS_LATITUDE[32];
extern char APRS_LONGITUDE[32];
extern char APRS_DESTINATION[48];
extern char APRS_SERVER_HOST[100];
extern int APRS_SERVER_PORT;
extern char APRS_SOFTWARE_NAME[64];
extern char APRS_SOFTWARE_VERSION[20];

void load_config(const char *filename);

#endif /*__EW_TO_APRS_CONFIG_PARSER__*/