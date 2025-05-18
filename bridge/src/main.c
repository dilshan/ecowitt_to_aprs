#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "config.h"
#include "mongoose.h"

#define APRS_SOFTWARE_NAME "GWtoAPRS"
#define APRS_SOFTWARE_VERSION "1.0"

/**
 * @brief Constructs and sends an APRS weather report based on the provided
 * WeatherData.
 *
 * This function constructs an APRS weather packet and sends it to the APRS-IS
 * server. It formats various weather parameters such as wind direction, wind
 * speed, temperature, rainfall, solar radiation, humidity, and barometric
 * pressure into the APRS packet. Additionally, it constructs a comment field
 * with information about the weather station model, max daily gust, UV index,
 * battery status, and software name.
 *
 * @param mgr The Mongoose manager instance.
 * @param wd Pointer to the WeatherData struct containing weather information.
 */
static void send_aprs_report(struct mg_mgr *mgr, const WeatherData *wd) {
  if (!wd || !wd->valid_data) {
    printf("APRS: Invalid or incomplete weather data, not sending.\n");
    return;
  }

  char aprs_packet_buffer[1024];
  char post_body[1500];
  char dhm_time_str[8];
  char comment_buffer[256] = "";  // Buffer for the comment field.

  // 1. Format DDHHMMz from dateutc_str.
  int year, month, day, hour, minute, second;
  int parsed_items = sscanf(wd->dateutc_str, "%d-%d-%d+%d:%d:%d", &year, &month,
                            &day, &hour, &minute, &second);
  if (parsed_items != 6) {
    parsed_items = sscanf(wd->dateutc_str, "%d-%d-%d %d:%d:%d", &year, &month,
                          &day, &hour, &minute, &second);
  }
  if (parsed_items == 6) {
    snprintf(dhm_time_str, sizeof(dhm_time_str), "%02d%02d%02d", day, hour,
             minute);
  } else {
    printf("APRS: Failed to parse dateutc: %s. Using placeholder time.\n",
           wd->dateutc_str);
    time_t now = time(NULL);
    struct tm *utc_tm = gmtime(&now);
    if (utc_tm)
      strftime(dhm_time_str, sizeof(dhm_time_str), "%d%H%M", utc_tm);
    else
      strcpy(dhm_time_str, "000000");
  }

  // 2. Construct the APRS weather packet core.
  snprintf(aprs_packet_buffer, sizeof(aprs_packet_buffer), "%s>%s:@%sz%s/%s_",
           APRS_CALLSIGN_SSID, APRS_DESTINATION, dhm_time_str, APRS_LATITUDE,
           APRS_LONGITUDE);

  char temp_buf[64];

  // Wind direction (cXXX).
  snprintf(temp_buf, sizeof(temp_buf), "c%03d",
           wd->wind_dir >= 0 && wd->wind_dir <= 360 ? wd->wind_dir : 0);
  strcat(aprs_packet_buffer, temp_buf);

  // Wind speed (sXXX).
  snprintf(temp_buf, sizeof(temp_buf), "s%03d",
           (int)round(wd->wind_speed_mph < 0 ? 0 : wd->wind_speed_mph));
  strcat(aprs_packet_buffer, temp_buf);

  // Wind gust (gXXX) - this is gust in current observation interval.
  snprintf(temp_buf, sizeof(temp_buf), "g%03d",
           (int)round(wd->wind_gust_mph < 0 ? 0 : wd->wind_gust_mph));
  strcat(aprs_packet_buffer, temp_buf);

  // Temperature (tXXX or t-XX).
  int temp_f_int = (int)round(wd->temp_f);
  if (temp_f_int < 0 && temp_f_int > -100) {  // APRS temp is -99 to 999.
    snprintf(temp_buf, sizeof(temp_buf), "t-%02d",
             (int)round(fabs(wd->temp_f)));
  } else if (temp_f_int >= 0 && temp_f_int <= 999) {
    snprintf(temp_buf, sizeof(temp_buf), "t%03d", temp_f_int);
  } else {  // Out of APRS range, send a boundary or default.
    snprintf(temp_buf, sizeof(temp_buf), "t%03d", temp_f_int < 0 ? -99 : 999);
  }
  strcat(aprs_packet_buffer, temp_buf);

  // Rain last hour (rXXX).
  snprintf(temp_buf, sizeof(temp_buf), "r%03d",
           (int)round(wd->rain_hourly_in * 100.0));
  strcat(aprs_packet_buffer, temp_buf);

  // Rain last 24 hours (PXXX).
  snprintf(temp_buf, sizeof(temp_buf), "P%03d",
           (int)round(wd->rain_daily_in * 100.0));
  strcat(aprs_packet_buffer, temp_buf);

  // Solar Radiation (LXXX for W/m^2, or lXXX for W/m^2, if >999 it's kW/m^2 *
  // 10). We'll use LXXX for W/m^2 up to 999.
  if (wd->solar_radiation >= 0 && wd->solar_radiation < 1000) {
    snprintf(temp_buf, sizeof(temp_buf), "L%03d",
             (int)round(wd->solar_radiation));
    strcat(aprs_packet_buffer, temp_buf);
  } else if (wd->solar_radiation >= 1000) {  // Use l for over 999.
    snprintf(
        temp_buf, sizeof(temp_buf), "l%03d",
        (int)round(wd->solar_radiation / 10.0));  // Example: 1200W/m^2 -> l120.
    strcat(aprs_packet_buffer, temp_buf);
  }
  // If solar_radiation is 0 or negative, it's omitted from this part.

  // Humidity (hXX).
  snprintf(
      temp_buf, sizeof(temp_buf), "h%02d",
      wd->humidity == 100 ? 0 : (wd->humidity < 0 ? 0 : wd->humidity % 100));
  strcat(aprs_packet_buffer, temp_buf);

  // Barometric pressure (bXXXXX).
  snprintf(temp_buf, sizeof(temp_buf), "b%05d",
           (int)round(wd->baro_rel_in * 33.8639 * 10.0));
  strcat(aprs_packet_buffer, temp_buf);

  // 3. Construct the comment Field.
  // Model
  if (strlen(wd->model) > 0) {
    strncat(comment_buffer, wd->model,
            sizeof(comment_buffer) - strlen(comment_buffer) - 1);
    strncat(comment_buffer, " ",
            sizeof(comment_buffer) - strlen(comment_buffer) - 1);
  }

  // Max Daily Gust.
  if (wd->max_daily_gust_mph > 0) {
    snprintf(temp_buf, sizeof(temp_buf), "MaxGust:%.1fmph ",
             wd->max_daily_gust_mph);
    strncat(comment_buffer, temp_buf,
            sizeof(comment_buffer) - strlen(comment_buffer) - 1);
  }

  // UV Index.
  if (wd->uv_index >= 0) {
    snprintf(temp_buf, sizeof(temp_buf), "UVI:%d ", wd->uv_index);
    strncat(comment_buffer, temp_buf,
            sizeof(comment_buffer) - strlen(comment_buffer) - 1);
  }

  // Append software name to comment.
  strncat(comment_buffer, APRS_SOFTWARE_NAME,
          sizeof(comment_buffer) - strlen(comment_buffer) - 1);

  // Remove trailing space if any.
  if (strlen(comment_buffer) > 0 &&
      comment_buffer[strlen(comment_buffer) - 1] == ' ') {
    comment_buffer[strlen(comment_buffer) - 1] = '\0';
  }

  // Append comment to packet if it's not empty.
  if (strlen(comment_buffer) > 0) {
    strncat(aprs_packet_buffer, comment_buffer,
            sizeof(aprs_packet_buffer) - strlen(aprs_packet_buffer) - 1);
  }

  // 4. Construct the full POST body for APRS-IS.
  snprintf(post_body, sizeof(post_body), "user %s pass %s vers %s %s\n%s\n",
           APRS_CALLSIGN_SSID, APRS_PASSCODE, APRS_SOFTWARE_NAME,
           APRS_SOFTWARE_VERSION, aprs_packet_buffer);

  printf("APRS: Preparing to send:\n%s", post_body);

  // 5. Send to APRS-IS server.
  char aprs_server_url[120];
  snprintf(aprs_server_url, sizeof(aprs_server_url), "http://%s:%d/",
           APRS_SERVER_HOST, APRS_SERVER_PORT);

  struct mg_connection *aprs_conn =
      mg_http_connect(mgr, aprs_server_url, NULL, NULL);
  if (aprs_conn == NULL) {
    fprintf(stderr, "APRS: Failed to create connection to %s\n",
            aprs_server_url);
    return;
  }

  char content_len_str[20];
  snprintf(content_len_str, sizeof(content_len_str), "%zu", strlen(post_body));

  mg_printf(aprs_conn,
            "POST / HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: %s\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            APRS_SERVER_HOST, APRS_SERVER_PORT, content_len_str, post_body);

  printf("APRS: Data sent to %s.\n", aprs_server_url);
}

/**
 * @brief Callback function for handling HTTP events.
 *
 * This function processes incoming HTTP messages, extracts weather data from
 * POST requests, and sends the data to the APRS-IS server if valid. It also
 * handles GET requests and unsupported media types.
 *
 * @param c Pointer to the Mongoose connection.
 * @param ev Event type.
 * @param ev_data Pointer to event data.
 */
static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    WeatherData current_wx_data = {0};
    current_wx_data.valid_data = 0;
    current_wx_data.solar_radiation =
        -1;                         // Initialize to indicate not present.
    current_wx_data.uv_index = -1;  // Initialize to indicate not present.
    current_wx_data.max_daily_gust_mph = -1;
    current_wx_data.batt_status = -1;

    if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
      struct mg_str *content_type_hdr = mg_http_get_header(hm, "Content-Type");

      if (content_type_hdr != NULL &&
          mg_strcmp(*content_type_hdr,
                    mg_str("application/x-www-form-urlencoded")) == 0) {
        printf(
            "Received POST request to / with Content-Type: "
            "application/x-www-form-urlencoded\n");

        struct mg_str body_data = hm->body;
        char *current_pos = body_data.buf;
        char *end_of_body = body_data.buf + body_data.len;

        char key_buf[256];
        char value_buf[1024];

        // Parse the URL-encoded body data.
        while (current_pos < end_of_body) {
          char *next_ampersand = current_pos;
          while (next_ampersand < end_of_body && *next_ampersand != '&')
            next_ampersand++;
          char *equals_sign = current_pos;
          while (equals_sign < next_ampersand && *equals_sign != '=')
            equals_sign++;

          struct mg_str key_encoded;
          key_encoded.buf = current_pos;
          key_encoded.len = equals_sign - current_pos;

          struct mg_str value_encoded;
          if (equals_sign < next_ampersand) {
            value_encoded.buf = equals_sign + 1;
            value_encoded.len = next_ampersand - (equals_sign + 1);
          } else {
            value_encoded.buf = next_ampersand;
            value_encoded.len = 0;
          }

          int key_len_decoded = mg_url_decode(key_encoded.buf, key_encoded.len,
                                              key_buf, sizeof(key_buf) - 1, 1);
          if (key_len_decoded >= 0)
            key_buf[key_len_decoded] = '\0';
          else
            strcpy(key_buf, "(KEY_ERR)");

          int value_len_decoded =
              mg_url_decode(value_encoded.buf, value_encoded.len, value_buf,
                            sizeof(value_buf) - 1, 1);
          if (value_len_decoded >= 0)
            value_buf[value_len_decoded] = '\0';
          else
            strcpy(value_buf, "(VAL_ERR)");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"

          // Populate WeatherData struct.
          if (strcmp(key_buf, "dateutc") == 0)
            strncpy(current_wx_data.dateutc_str, value_buf,
                    sizeof(current_wx_data.dateutc_str) - 1);
          else if (strcmp(key_buf, "winddir") == 0)
            current_wx_data.wind_dir = atoi(value_buf);
          else if (strcmp(key_buf, "windspeedmph") == 0)
            current_wx_data.wind_speed_mph = atof(value_buf);
          else if (strcmp(key_buf, "windgustmph") == 0)
            current_wx_data.wind_gust_mph = atof(value_buf);
          else if (strcmp(key_buf, "maxdailygust") == 0)
            current_wx_data.max_daily_gust_mph = atof(value_buf);
          else if (strcmp(key_buf, "tempf") == 0)
            current_wx_data.temp_f = atof(value_buf);
          else if (strcmp(key_buf, "hourlyrainin") == 0)
            current_wx_data.rain_hourly_in = atof(value_buf);
          else if (strcmp(key_buf, "dailyrainin") == 0)
            current_wx_data.rain_daily_in = atof(value_buf);
          else if (strcmp(key_buf, "humidity") == 0)
            current_wx_data.humidity = atoi(value_buf);
          else if (strcmp(key_buf, "baromrelin") == 0)
            current_wx_data.baro_rel_in = atof(value_buf);
          else if (strcmp(key_buf, "solarradiation") == 0)
            current_wx_data.solar_radiation = atof(value_buf);
          else if (strcmp(key_buf, "uv") == 0)
            current_wx_data.uv_index = atoi(value_buf);
          else if (strcmp(key_buf, "wh65batt") == 0)
            current_wx_data.batt_status = atoi(value_buf);
          else if (strcmp(key_buf, "model") == 0)
            strncpy(current_wx_data.model, value_buf,
                    sizeof(current_wx_data.model) - 1);

#pragma GCC diagnostic pop

          if (next_ampersand < end_of_body && *next_ampersand == '&')
            current_pos = next_ampersand + 1;
          else
            break;
        }

        if (strlen(current_wx_data.dateutc_str) > 0 &&
            current_wx_data.baro_rel_in > 0) {
          current_wx_data.valid_data = 1;
        }

        if (current_wx_data.valid_data) {
          printf("Weather data parsed. Forwarding to APRS...\n");
          send_aprs_report(c->mgr, &current_wx_data);
        } else {
          printf("Incomplete weather data. Not sending to APRS.\n");
        }
        mg_http_reply(c, 200, "Content-Type: text/plain\r\n",
                      "Data received.\n");
      } else {
        mg_http_reply(c, 415, "", "Unsupported Media Type\n");
      }
    } else if (mg_strcmp(hm->uri, mg_str("/")) == 0) {
      mg_http_reply(c, 200, "", "GET request to /. POST weather data here.\n");
    } else {
      mg_http_reply(c, 404, "", "Not Found\n");
    }
  }
}

int main(void) {
  struct mg_mgr mgr;
  char addr[64];
  const char *port = "1234";

  const char *config_path = getenv("CONFIG");
  if (!config_path) config_path = "default.cfg";

  mg_mgr_init(&mgr);
  load_config(config_path);

  printf("Starting Ecowitt to APRS Gateway on port %s\n", port);
  printf("APRS Callsign: %s, Lat: %s, Lon: %s\n", APRS_CALLSIGN_SSID,
         APRS_LATITUDE, APRS_LONGITUDE);

  snprintf(addr, sizeof(addr), "http://0.0.0.0:%s", port);

  if (mg_http_listen(&mgr, addr, fn, NULL) == NULL) {
    fprintf(stderr, "Cannot listen on %s.\n", addr);
    mg_mgr_free(&mgr);
    return 1;
  }

  // Main event loop.
  for (;;) mg_mgr_poll(&mgr, 1000);

  mg_mgr_free(&mgr);
  return 0;
}