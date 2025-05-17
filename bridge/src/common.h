#ifndef __EW_TO_APRS_COMMON__
#define __EW_TO_APRS_COMMON__

// Structure to hold parsed weather data.
typedef struct
{
    char dateutc_str[30];
    int wind_dir;
    double wind_speed_mph;
    double wind_gust_mph;      // Gust in last measurement interval.
    double max_daily_gust_mph; // Max gust for the day.
    double temp_f;
    double rain_hourly_in;
    double rain_daily_in;
    int humidity;
    double baro_rel_in;
    double solar_radiation; // W/m^2
    int uv_index;
    int batt_status; // Decoding information is not available for this!
    char model[50];

    int valid_data;
} WeatherData;

#endif /*__EW_TO_APRS_COMMON__*/