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
#define SMALL_BUFFER_SIZE 512
#define LARGE_BUFFER_SIZE 1024

static bool isInitialize = false;
void StreetAPI_init(){
    assert(!isInitialize);
    isInitialize = true;
}

// Callback function for libcurl to write incoming data to a buffer.
static size_t write_callback(void *ptr, size_t size, size_t nmemb, struct Response *res) {
    size_t new_size = size * nmemb;
    res->data = realloc(res->data, res->size + new_size + 1);
    if (res->data == NULL) return 0;
    memcpy(res->data + res->size, ptr, new_size);
    res->size += new_size;
    res->data[res->size] = '\0';
    return new_size;
}

// It seems necessary to use a custom URL encoding function since the API may not handle spaces correctly.
// It basically replace every space in the sentence to %20
static void apply_url_encode(const char *input, char *output, size_t max_len) {
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

// Uses the OpenStreetMap Nominatim API to convert a address into latitude and longitude.
struct location StreetAPI_get_lat_long(char *address) {
    assert(isInitialize);
    CURL *curl;
    CURLcode res;
    struct Response response = {NULL, 0};
    struct location loc = {INVALID_LATITUDE, INVALID_LONGITUDE, INVALID_SPEED}; // Default invalid values

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return loc;
    }

    char encoded_address[SMALL_BUFFER_SIZE];
    // Replace every space to %20 
    apply_url_encode(address, encoded_address, sizeof(encoded_address));

    char url[LARGE_BUFFER_SIZE]; // Increase buffer size to handle longer URLs
    snprintf(url, sizeof(url), "%s%s", API_URL_ADRESS, encoded_address);

    // Set the target URL for the HTTP GET request.
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Set the callback function that will handle the incoming data.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    // Provide the response buffer to the callback function so it knows where to store data.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // Set the header to identify the client. It semms required by the API. (not working without it)
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
    } else {
        // Searching the JSON response for latitude and longitude
        char *lat_ptr = strstr(response.data, "\"lat\":\"");
        char *lon_ptr = strstr(response.data, "\"lon\":\"");

        if (lat_ptr && lon_ptr) {
            sscanf(lat_ptr, "\"lat\":\"%lf\"", &loc.latitude);
            sscanf(lon_ptr, "\"lon\":\"%lf\"", &loc.longitude);
        } else {
            printf("No results found for the given address.\n");
        }
    }
    // Free the response data and clean up CURL
    free(response.data);
    curl_easy_cleanup(curl);
    return loc;
}

// Uses the OpenStreetMap Nominatim API to convert latitude and longitude into address.
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

    char url[LARGE_BUFFER_SIZE];
    snprintf(url, sizeof(url), API_URL_LAT_LON, lat, lon);
    // Set the target URL for the HTTP GET request.
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Set the callback function that will handle the incoming data.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    // Provide the response buffer to the callback function so it knows where to store data.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // Set the header to identify the client. It semms required by the API. (not working without it)
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
