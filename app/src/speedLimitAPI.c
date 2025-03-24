#include "speedLimitAPI.h"
#include <stdbool.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OVERPASS_API_URL "https://overpass-api.de/api/interpreter"

// Default speed limits based on road types
static int estimate_speed_limit(const char *highway_type) {
    if (strcmp(highway_type, "motorway") == 0) return 90;
    if (strcmp(highway_type, "motorway_link") == 0) return 90;
    if (strcmp(highway_type, "trunk") == 0) return 100;
    if (strcmp(highway_type, "trunk_link") == 0) return 80;
    if (strcmp(highway_type, "primary") == 0) return 80;
    if (strcmp(highway_type, "primary_link") == 0) return 70;
    if (strcmp(highway_type, "secondary") == 0) return 50;
    if (strcmp(highway_type, "secondary_link") == 0) return 50;
    if (strcmp(highway_type, "tertiary") == 0) return 50;
    if (strcmp(highway_type, "tertiary_link") == 0) return 50;
    if (strcmp(highway_type, "unclassified") == 0) return 50;
    if (strcmp(highway_type, "residential") == 0) return 30;
    if (strcmp(highway_type, "living_street") == 0) return 30;

    return 50; // Unknown road type (default 50) was -1
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        fprintf(stderr, "Not enough memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int get_speed_limit(double latitude, double longitude) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    if (!chunk.memory) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "[out:json];way(around:5,%.8f,%.8f)[\"highway\"];out body;", latitude, longitude);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, OVERPASS_API_URL);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Print the full JSON response
            // printf("Response from Overpass API:\n%s\n", chunk.memory);

            cJSON *json = cJSON_Parse(chunk.memory);
            if(json) {
                cJSON *elements = cJSON_GetObjectItem(json, "elements");
                if(cJSON_IsArray(elements)) {
                    cJSON *way;
                    cJSON_ArrayForEach(way, elements) {
                        cJSON *tags = cJSON_GetObjectItem(way, "tags");
                        if(tags) {
                            cJSON *maxspeed = cJSON_GetObjectItem(tags, "maxspeed");
                            if(maxspeed && cJSON_IsString(maxspeed)) {
                                char *endptr;
                                long speed = strtol(maxspeed->valuestring, &endptr, 10);
                                if (*endptr == '\0') { // Valid integer conversion
                                    cJSON_Delete(json);
                                    free(chunk.memory);
                                    curl_easy_cleanup(curl);
                                    curl_global_cleanup();
                                    return (int)speed;
                                }
                            }

                            // If maxspeed not found, check highway type
                            cJSON *highway = cJSON_GetObjectItem(tags, "highway");
                            if (highway && cJSON_IsString(highway)) {
                                int estimated_speed = estimate_speed_limit(highway->valuestring);
                                if (estimated_speed != -1) {
                                    printf("Estimated speed limit based on road type (%s)\n", highway->valuestring);
                                    cJSON_Delete(json);
                                    free(chunk.memory);
                                    curl_easy_cleanup(curl);
                                    curl_global_cleanup();
                                    return estimated_speed;
                                }
                            }
                        }
                    }
                }
                cJSON_Delete(json);
            }
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    free(chunk.memory);
    return 50; // Speed limit not found (default 50) was -1
}
