#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <assert.h>
#include "streetAPI.h"
#include <json-c/json.h>
#include "hal/GPS.h"

#define API_URL_ADRESS "https://nominatim.openstreetmap.org/search?format=json&q="
#define API_URL_LAT_LON "https://nominatim.openstreetmap.org/reverse?format=json&lat=%f&lon=%f"

static bool isInitialize = false;
void StreetAPI_init(){
    assert(!isInitialize);
    isInitialize = true;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, struct Response *res) {
    size_t new_size = size * nmemb;
    res->data = realloc(res->data, res->size + new_size + 1);
    if (res->data == NULL) return 0;
    memcpy(res->data + res->size, ptr, new_size);
    res->size += new_size;
    res->data[res->size] = '\0';
    return new_size;
}

// Function to replace spaces with %20
static void url_encode(const char *input, char *output, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0' && j < max_len - 3; i++) {
        if (input[i] == ' ') {
            strcpy(output + j, "%20");
            j += 3;
        } else {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

struct location StreetAPI_get_lat_long(char *address) {
    assert(isInitialize);
    CURL *curl;
    CURLcode res;
    struct Response response = {NULL, 0};
    struct location loc = {-1000, -1000, -1}; // Default invalid values

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return loc;
    }

    char encoded_address[512];
    url_encode(address, encoded_address, sizeof(encoded_address));

    char url[1024]; // Increase buffer size to handle longer URLs
    snprintf(url, sizeof(url), "%s%s", API_URL_ADRESS, encoded_address);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
    } else {
        // Manual JSON parsing using strstr() and sscanf()
        char *lat_ptr = strstr(response.data, "\"lat\":\"");
        char *lon_ptr = strstr(response.data, "\"lon\":\"");

        if (lat_ptr && lon_ptr) {
            sscanf(lat_ptr, "\"lat\":\"%lf\"", &loc.latitude);
            sscanf(lon_ptr, "\"lon\":\"%lf\"", &loc.longitude);
        } else {
            printf("No results found for the given address.\n");
        }
    }

    free(response.data);
    curl_easy_cleanup(curl);
    return loc;
}

char* StreetAPI_get_address_from_lat_lon(double lat, double lon) {
    assert(isInitialize);
    CURL *curl;
    CURLcode res;
    struct Response response = {NULL, 0};
    char *address = NULL;  // Default null value for address

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return NULL;
    }

    char url[1024];
    snprintf(url, sizeof(url), API_URL_LAT_LON, lat, lon);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
    } else {
        // Manual JSON parsing using strstr() and extracting the display_name
        char *address_ptr = strstr(response.data, "\"display_name\":\"");
        if (address_ptr) {
            address_ptr += strlen("\"display_name\":\"");
            char *end_ptr = strchr(address_ptr, '"');
            if (end_ptr) {
                size_t len = end_ptr - address_ptr;
                address = malloc(len + 1);
                if (address) {
                    strncpy(address, address_ptr, len);
                    address[len] = '\0';
                }
            }
        } else {
            printf("No address found for the given coordinates.\n");
        }
    }

    free(response.data);
    curl_easy_cleanup(curl);
    return address;
}

void StreetAPI_cleanup() {
    assert(isInitialize);
    isInitialize = false;
}
