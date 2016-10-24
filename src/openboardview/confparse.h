
#define CONFPARSE_MAX_VALUE_SIZE 10240
struct Confparse {

	char value[CONFPARSE_MAX_VALUE_SIZE];
	char *conf, *limit;
	size_t buffer_size;

	int Load( const char *utf8_filename );
	char *Parse( const char *key );
	char *Parse( const char *key, char *defaultv );
	double ParseDouble(const char *key, double defaultv );
	int ParseInt( const char *key, int defaultv );

};
