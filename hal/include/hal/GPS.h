void GPS_init();
char* GPS_read();
void parse_GNGGA(char* gngga_sentence, double* latitude, double* longitude);