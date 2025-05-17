#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CFG_LINE 256

// Global config variables
char APRS_CALLSIGN_SSID[32];
char APRS_PASSCODE[32];
char APRS_LATITUDE[32];
char APRS_LONGITUDE[32];
char APRS_DESTINATION[48];
char APRS_SERVER_HOST[100];
int APRS_SERVER_PORT;
char APRS_SOFTWARE_NAME[64];
char APRS_SOFTWARE_VERSION[20];

void load_config(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open config file");
        exit(1);
    }

    char line[MAX_CFG_LINE];
    while (fgets(line, sizeof(line), file))
    {
        char *eq = strchr(line, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        // Remove newline.
        value[strcspn(value, "\r\n")] = 0;

        if (strcmp(key, "APRS_CALLSIGN_SSID") == 0)
            strncpy(APRS_CALLSIGN_SSID, value, sizeof(APRS_CALLSIGN_SSID) - 1);
        else if (strcmp(key, "APRS_PASSCODE") == 0)
            strncpy(APRS_PASSCODE, value, sizeof(APRS_PASSCODE) - 1);
        else if (strcmp(key, "APRS_LATITUDE") == 0)
            strncpy(APRS_LATITUDE, value, sizeof(APRS_LATITUDE) - 1);
        else if (strcmp(key, "APRS_LONGITUDE") == 0)
            strncpy(APRS_LONGITUDE, value, sizeof(APRS_LONGITUDE) - 1);
        else if (strcmp(key, "APRS_DESTINATION") == 0)
            strncpy(APRS_DESTINATION, value, sizeof(APRS_DESTINATION) - 1);
        else if (strcmp(key, "APRS_SERVER_HOST") == 0)
            strncpy(APRS_SERVER_HOST, value, sizeof(APRS_SERVER_HOST) - 1);
        else if (strcmp(key, "APRS_SERVER_PORT") == 0)
            APRS_SERVER_PORT = atoi(value);
        else if (strcmp(key, "APRS_SOFTWARE_NAME") == 0)
            strncpy(APRS_SOFTWARE_NAME, value, sizeof(APRS_SOFTWARE_NAME) - 1);
        else if (strcmp(key, "APRS_SOFTWARE_VERSION") == 0)
            strncpy(APRS_SOFTWARE_VERSION, value, sizeof(APRS_SOFTWARE_VERSION) - 1);
    }

    fclose(file);
}
