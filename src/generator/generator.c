#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include "sodium/utils.h"
#include "pomelo/token.h"
#include "pomelo/allocator.h"
#include "utils/map.h"
#include "args.h"


#define POMELO_CONNECT_TOKEN_BASE64_CAPACITY \
    sodium_base64_ENCODED_LEN(               \
        POMELO_CONNECT_TOKEN_BYTES,          \
        sodium_base64_VARIANT_URLSAFE        \
    )

#define ARG_OUTPUT_B64 "b64"
#define ARG_OUTPUT_BIN "bin"

#define DEFAULT_EXPIRE_TIME 3600ULL * 1000ULL
#define DATE_TIME_BUFFER_LENGTH 30
#define FIELD_COL_FMT " + %-17s = "


static pomelo_arg_descriptor_t descriptors[] = {
    { "-a", "--address"          },
    { "-i", "--client_id"        },
    { "-k", "--private_key"      },
    { "-p", "--protocol_id"      },
    { "-c", "--create_timestamp" },
    { "-e", "--expire_timestamp" },
    { "-n", "--nonce"            },
    { "-t", "--timeout"          },
    { "-C", "--client_to_server" },
    { "-S", "--server_to_client" },
    { "-u", "--user_data"        },
    { "-o", "--output_format"    },
    { "-f", "--output_file"      },
    { "-s", "--silence"          },
    { "-h", "--help"             }
};


static const char * helps[] = {
    "* Addresses, required, max 32, separate by ';'",
    "* Client ID, required", 
    "* Private Key, 32 bytes, required",
    "Protocol ID, default 0",
    "Create timestamp, in ms, default now",
    "Expire timestamp, in ms, default now + 1 hour",
    "Nonce of connect token, 24 bytes, default zero",
    "Timeout in seconds, default 60",
    "Client to Server key, 32 bytes, default zero",
    "Server to Client key, 32 bytes, default zero",
    "User data, 256 bytes, default zero",
    ( "hex|b64|bin, output format, default hex, bin is only supported with"
    "file output" ),
    "Output file, stdout is used by default",
    "Silence mode, only output is going to be shown",
    "Show help"
};


// Argument code definitions
typedef enum pomelo_generator_arg {
    POMELO_GENERATOR_ARG_ADDRESSES,
    POMELO_GENERATOR_ARG_CLIENT_ID,
    POMELO_GENERATOR_ARG_PRIVATE_KEY,
    POMELO_GENERATOR_ARG_PROTOCOL_ID,
    POMELO_GENERATOR_ARG_CREATE_TIMESTAMP,
    POMELO_GENERATOR_ARG_EXPIRE_TIMESTAMP,
    POMELO_GENERATOR_ARG_NONCE,
    POMELO_GENERATOR_ARG_TIMEOUT,
    POMELO_GENERATOR_ARG_CLIENT_TO_SERVER,
    POMELO_GENERATOR_ARG_SERVER_TO_CLIENT,
    POMELO_GENERATOR_ARG_USER_DATA,
    POMELO_GENERATOR_ARG_OUTPUT_FORMAT,
    POMELO_GENERATOR_ARG_OUTPUT_FILE,
    POMELO_GENERATOR_ARG_SILENCE,
    POMELO_GENERATOR_ARG_HELP,
    POMELO_GENERATOR_ARG_COUNT
} pomelo_generator_arg;

#define POMELO_GENERATOR_ARG_VECTOR_SIZE \
    POMELO_GENERATOR_ARG_COUNT * sizeof(pomelo_arg_vector_t)

/// @brief Output format
typedef enum pomelo_generator_format {
    POMELO_GENERATOR_FORMAT_HEX,
    POMELO_GENERATOR_FORMAT_B64,
    POMELO_GENERATOR_FORMAT_BIN
} pomelo_generator_format;


typedef void (*pomelo_generator_log_fn)(const char * format, ...);
typedef void (*pomelo_generator_log_hex_array_fn)(
    const uint8_t * array,
    size_t length
);


typedef struct {
    pomelo_connect_token_t * token;
    uint8_t * private_key;
    uint8_t * connect_token;
    char * connect_token_b64;
    pomelo_generator_log_fn log;
    pomelo_generator_log_fn log_warn;
    pomelo_generator_log_fn log_error;
    pomelo_generator_log_hex_array_fn log_hex_array;
} pomelo_generator_context_t;


/// @brief Log message to stdout
static void pomelo_generator_log(const char * format, ...) {
    va_list(args);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


/// @brief Log error to stdout
static void pomelo_generator_log_error(const char * format, ...) {
    printf("Error: ");
    va_list(args);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


/// @brief Log warn to stdout
static void pomelo_generator_log_warn(const char * format, ...) {
    printf("Warn: ");
    va_list(args);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


/// @brief Log hex array
static void pomelo_generator_log_hex_array(
    const uint8_t * array,
    size_t length
) {
    if (length == 0) {
        return;
    }

    // Find the last non-zero index
    size_t last = length - 1;
    while (last > 0 && array[last] == 0) {
        last--;
    }

    if (last == 0 && array[0] == 0) {
        // All elements are zero
        pomelo_generator_log("<%zu zero elements>", length);
        return;
    }

    for (size_t i = 0; i <= last; i++) {
        pomelo_generator_log("%02x ", array[i]);
    }

    size_t remain = length - last - 1;
    if (remain > 0) {
        pomelo_generator_log("... <%zu more zero elements>", remain);
    }
}


/// @brief Log silence
static void pomelo_generator_log_silence(const char * format, ...) {
    (void) format;
}

/// @brief Log silence
static void pomelo_generator_log_hex_array_silence(
    const uint8_t * array,
    size_t length
) {
    (void) array;
    (void) length;
}


/// @brief Cleanup resources
static void pomelo_generator_cleanup(pomelo_generator_context_t * context) {
    if (context->token) {
        free(context->token);
    }

    if (context->private_key) {
        free(context->private_key);
    }

    if (context->connect_token) {
        free(context->connect_token);
    }

    if (context->connect_token_b64) {
        free(context->connect_token_b64);
    }
    memset(context, 0, sizeof(pomelo_generator_context_t));
}


/// @brief Initialize resources
static int pomelo_generator_init(pomelo_generator_context_t * context) {
    context->token = malloc(sizeof(pomelo_connect_token_t));
    if (!context->token) {
        // Failed to allocate new token
        pomelo_generator_log("Failed to allocate connect token.\n");
        return -1;
    }
    memset(context->token, 0, sizeof(pomelo_connect_token_t));

    context->private_key = malloc(POMELO_KEY_BYTES);
    if (!context->private_key) {
        pomelo_generator_log("Failed to allocate private key.\n");
        return -1;
    }

    context->connect_token = malloc(POMELO_CONNECT_TOKEN_BYTES);
    if (!context->connect_token) {
        pomelo_generator_log("Failed to allocate connect token buffer.\n");
        return -1;
    }
    memset(context->connect_token, 0, POMELO_CONNECT_TOKEN_BYTES);

    return 0;
}


/// @brief Scan binary array from argument. Missing fields will be filled by
// zero
static void scan_binary_array(
    char * argv[],
    pomelo_arg_vector_t * vector,
    uint8_t * array,
    size_t length
) {
    size_t index = 0;
    size_t arg_length = (size_t) (vector->end - vector->begin + 1);
    for (int i = vector->begin; i <= vector->end; i++) {
        array[index++] = (uint8_t) strtoul(argv[i], NULL, 0);
    }

    // Fill the rest by zero
    memset(array + arg_length, 0, length - arg_length);
}


/// @brief Scan addresses
static void scan_addresses(
    char * argv[],
    pomelo_arg_vector_t * vector,
    int * naddresses,
    pomelo_address_t * addresses
) {
    int index = 0;
    for (int i = vector->begin; i <= vector->end; i++) {
        char * arg = argv[i];
        int ret = pomelo_address_from_string(addresses + index, arg);
        if (ret == 0) {
            index++;
        }
    }

    *naddresses = index;
}


/// @brief Write array as hex
static void fwrite_hex_array(
    FILE * file,
    const uint8_t * array,
    size_t length
) {
    if (length == 0) {
        return;
    }

    size_t last_index = length - 1;
    for (size_t i = 0; i < length; i++) {
        if (i < last_index) {
            fprintf(file, "%02x ", array[i]);
        } else {
            fprintf(file, "%02x", array[i]);
        }
    }
}


/// @brief Show help
static void pomelo_generator_show_help(void) {
    pomelo_generator_log("Usage: pomelo_generator\n");
    for (int i = 0; i < POMELO_GENERATOR_ARG_COUNT; i++) {
        pomelo_generator_log(
            "%6s, %-20s %s\n",
            descriptors[i].arg_short,
            descriptors[i].arg_long,
            helps[i]
        );
    }
}


/// @brief Update token information from arguments
static int pomelo_generator_update_token_info(
    pomelo_generator_context_t * context,
    pomelo_arg_vector_t * vectors,
    pomelo_connect_token_t * token,
    char * argv[]
) {
    if (vectors[POMELO_GENERATOR_ARG_ADDRESSES].begin) {
        scan_addresses(
            argv,
            &vectors[POMELO_GENERATOR_ARG_ADDRESSES],
            &token->naddresses,
            token->addresses
        );

        if (token->naddresses == 0) {
            context->log_error("No valid addresses are provided\n");
            return -1;
        }
    } else {
        context->log_error("No addresses are provided\n");
        return -1;
    }

    if (vectors[POMELO_GENERATOR_ARG_CLIENT_ID].begin) {
        sscanf(
            argv[vectors[POMELO_GENERATOR_ARG_CLIENT_ID].begin],
            "%" PRIi64,
            &token->client_id
        );
    } else {
        context->log_error("No client ID is provided\n");
        return -1;
    }

    if (vectors[POMELO_GENERATOR_ARG_PRIVATE_KEY].begin) {
        scan_binary_array(
            argv,
            &vectors[POMELO_GENERATOR_ARG_PRIVATE_KEY],
            context->private_key,
            POMELO_KEY_BYTES
        );
    } else {
        context->log_error("No private key is provided\n");
        return -1;
    }

    if (vectors[POMELO_GENERATOR_ARG_PROTOCOL_ID].begin) {
        sscanf(
            argv[vectors[POMELO_GENERATOR_ARG_PROTOCOL_ID].begin],
            "%" PRIu64,
            &token->protocol_id
        );
    }

    if (vectors[POMELO_GENERATOR_ARG_CREATE_TIMESTAMP].begin) {
        sscanf(
            argv[vectors[POMELO_GENERATOR_ARG_CREATE_TIMESTAMP].begin],
            "%" PRIu64,
            &token->create_timestamp
        );
    } else {
        token->create_timestamp = ((uint64_t) time(NULL)) * 1000ULL;
    }

    if (vectors[POMELO_GENERATOR_ARG_EXPIRE_TIMESTAMP].begin) {
        sscanf(
            argv[vectors[POMELO_GENERATOR_ARG_EXPIRE_TIMESTAMP].begin],
            "%" PRIu64,
            &token->expire_timestamp
        );
    } else {
        token->expire_timestamp = token->create_timestamp + DEFAULT_EXPIRE_TIME;
    }

    if (vectors[POMELO_GENERATOR_ARG_NONCE].begin) {
        scan_binary_array(
            argv,
            &vectors[POMELO_GENERATOR_ARG_NONCE],
            token->connect_token_nonce,
            POMELO_CONNECT_TOKEN_NONCE_BYTES
        );
    }

    if (vectors[POMELO_GENERATOR_ARG_TIMEOUT].begin) {
        sscanf(
            argv[vectors[POMELO_GENERATOR_ARG_TIMEOUT].begin],
            "%" PRIi32,
            &token->timeout
        );
    } else {
        token->timeout = -1;
    }

    if (vectors[POMELO_GENERATOR_ARG_CLIENT_TO_SERVER].begin) {
        scan_binary_array(
            argv,
            &vectors[POMELO_GENERATOR_ARG_CLIENT_TO_SERVER],
            token->client_to_server_key,
            POMELO_KEY_BYTES
        );
    }

    if (vectors[POMELO_GENERATOR_ARG_SERVER_TO_CLIENT].begin) {
        scan_binary_array(
            argv,
            &vectors[POMELO_GENERATOR_ARG_SERVER_TO_CLIENT],
            token->server_to_client_key,
            POMELO_KEY_BYTES
        );
    }

    if (vectors[POMELO_GENERATOR_ARG_USER_DATA].begin) {
        scan_binary_array(
            argv,
            &vectors[POMELO_GENERATOR_ARG_USER_DATA],
            token->user_data,
            POMELO_USER_DATA_BYTES
        );
    }

    return 0;
}


/// @brief Show summary
static void pomelo_generator_show_summary(
    pomelo_generator_context_t * context
) {
    pomelo_connect_token_t * token = context->token;

    // Print summary
    context->log("Summary:\n");
    context->log(
        FIELD_COL_FMT "%" PRIu64 "\n",
        "Protocol ID",
        token->protocol_id
    );

    time_t time = (time_t) (token->create_timestamp / 1000ULL);
    char datetime_buffer[DATE_TIME_BUFFER_LENGTH];
    strftime(
        datetime_buffer,
        DATE_TIME_BUFFER_LENGTH,
        "%c GMT",
        gmtime(&time)
    );

    context->log(
        FIELD_COL_FMT "%" PRIu64 " (%s)\n",
        "Create timestamp",
        token->create_timestamp,
        datetime_buffer
    );

    time = (time_t) (token->expire_timestamp / 1000ULL);
    strftime(
        datetime_buffer,
        DATE_TIME_BUFFER_LENGTH,
        "%c GMT",
        gmtime(&time)
    );
    context->log(
        FIELD_COL_FMT "%" PRIu64 " (%s)\n",
        "Expire timestamp",
        token->expire_timestamp,
        datetime_buffer
    );
    context->log(FIELD_COL_FMT "%" PRIi32 "\n", "Timeout", token->timeout);
    context->log(FIELD_COL_FMT "%" PRIi64 "\n", "Client ID", token->client_id);
    context->log(FIELD_COL_FMT, "Nonce");
    context->log_hex_array(
        token->connect_token_nonce,
        POMELO_CONNECT_TOKEN_NONCE_BYTES
    );
    context->log("\n");
    context->log(FIELD_COL_FMT, "Server to client");
    context->log_hex_array(token->server_to_client_key, POMELO_KEY_BYTES);
    context->log("\n");
    context->log(FIELD_COL_FMT, "Client to server");
    context->log_hex_array(token->client_to_server_key, POMELO_KEY_BYTES);
    context->log("\n");
    context->log(FIELD_COL_FMT, "Private key");
    context->log_hex_array(context->private_key, POMELO_KEY_BYTES);
    context->log("\n");
    context->log(FIELD_COL_FMT, "User data");
    context->log_hex_array(token->user_data, POMELO_USER_DATA_BYTES);
    context->log("\n");

    if (token->naddresses != 1) {
        context->log(
            FIELD_COL_FMT " { size = %d }\n",
            "Addresses",
            token->naddresses
        );
    } else {
        context->log(FIELD_COL_FMT, "Address");
    }

    for (int i = 0; i < token->naddresses; i++) {
        pomelo_address_t * address = &token->addresses[i];
        char buffer[POMELO_ADDRESS_STRING_BUFFER_CAPACITY];
        pomelo_address_to_string(
            address,
            buffer,
            POMELO_ADDRESS_STRING_BUFFER_CAPACITY
        );
        if (token->naddresses != 1) {
            context->log("%10s%s\n", "", buffer);
        } else {
            context->log("%s\n", buffer);
        }
    }
}


/// @brief Write output
static int pomelo_generator_write_output(
    pomelo_generator_context_t * context,
    FILE * output,
    pomelo_generator_format format
) {
    if (output == stdout && format == POMELO_GENERATOR_FORMAT_BIN) {
        context->log_warn(
            "Output bin for stdout is not supported. Fallback to hex"
        );
        format = POMELO_GENERATOR_FORMAT_HEX; // Change to hex
    }

    uint8_t * connect_token = context->connect_token;
    if (format == POMELO_GENERATOR_FORMAT_B64) {
        context->connect_token_b64 =
            malloc(POMELO_CONNECT_TOKEN_BASE64_CAPACITY);
        if (!context->connect_token_b64) {
            context->log_error(
                "Failed to allocate base64 connect token buffer\n"
            );
            return -1;
        }

        // Encode base64
        sodium_bin2base64(
            context->connect_token_b64,
            POMELO_CONNECT_TOKEN_BASE64_CAPACITY,
            connect_token,
            POMELO_CONNECT_TOKEN_BYTES,
            sodium_base64_VARIANT_URLSAFE
        );
    }

    if (output == stdout) {
        context->log("\nOutput:\n");
    }
    switch (format) {
        case POMELO_GENERATOR_FORMAT_B64:
            fwrite(
                context->connect_token_b64,
                sizeof(char),
                POMELO_CONNECT_TOKEN_BASE64_CAPACITY - 1, // Exclude last '\0'
                output
            );
            break;

        case POMELO_GENERATOR_FORMAT_BIN:
            fwrite(
                connect_token,
                sizeof(uint8_t),
                POMELO_CONNECT_TOKEN_BYTES,
                output
            );
            break;

        case POMELO_GENERATOR_FORMAT_HEX:
            fwrite_hex_array(output, connect_token, POMELO_CONNECT_TOKEN_BYTES);
            break;
    }

    return 0;
}

/// @brief Entry
static int run(
    int argc,
    char * argv[],
    pomelo_generator_context_t * context,
    pomelo_arg_vector_t * vectors
) {
    // Process arguments
    pomelo_arg_process(
        argc,
        argv,
        descriptors,
        vectors,
        POMELO_GENERATOR_ARG_COUNT
    );

    // Check help
    if (vectors[POMELO_GENERATOR_ARG_HELP].present) {
        pomelo_generator_show_help();
        return 0;
    }

    // Setup log
    if (vectors[POMELO_GENERATOR_ARG_SILENCE].present) {
        context->log = pomelo_generator_log_silence;
        context->log_warn = pomelo_generator_log_silence;
        context->log_error = pomelo_generator_log_silence;
        context->log_hex_array = pomelo_generator_log_hex_array_silence;
    } else {
        context->log = pomelo_generator_log;
        context->log_warn = pomelo_generator_log_warn;
        context->log_error = pomelo_generator_log_error;
        context->log_hex_array = pomelo_generator_log_hex_array;
    }

    // Update token information from arguments
    pomelo_connect_token_t * token = context->token;
    int result = pomelo_generator_update_token_info(
        context,
        vectors,
        token,
        argv
    );
    if (result < 0) {
        return -1;
    }

    // Encode token
    result = pomelo_connect_token_encode(
        context->connect_token,
        token,
        context->private_key
    );
    if (result < 0) {
        context->log_error("Failed to encode connect token.\n");
        return -1;
    }

    // Show summary
    pomelo_generator_show_summary(context);

    pomelo_generator_format format = POMELO_GENERATOR_FORMAT_HEX;
    if (vectors[POMELO_GENERATOR_ARG_OUTPUT_FORMAT].begin) {
        char * arg = argv[vectors[POMELO_GENERATOR_ARG_OUTPUT_FORMAT].begin];
        if (strcmp(arg, ARG_OUTPUT_B64) == 0) {
            format = POMELO_GENERATOR_FORMAT_B64;
        } else if (strcmp(arg, ARG_OUTPUT_BIN) == 0) {
            format = POMELO_GENERATOR_FORMAT_BIN;
        }
    }

    FILE * output = stdout;
    const char * output_file = NULL;
    if (vectors[POMELO_GENERATOR_ARG_OUTPUT_FILE].begin) {
        output_file = argv[vectors[POMELO_GENERATOR_ARG_OUTPUT_FILE].begin];
        output = fopen(output_file, "w");
        if (!output) {
            context->log_error("Failed to open %s\n.", output_file);
            return -1;
        }
    }

    // Write or show the output
    result = pomelo_generator_write_output(context, output, format);
    if (result < 0) {
        return -1;
    }

    if (output == stdout) {
        fprintf(output, "\n"); // End line for stdout
        context->log("\nDone.\n");
    } else {
        fclose(output);
        context->log("\nDone >> \"%s\"\n", output_file);
    }

    return 0;
}

static pomelo_arg_vector_t vectors[POMELO_GENERATOR_ARG_VECTOR_SIZE];
static pomelo_generator_context_t context;


int main(int argc, char * argv[]) {
    if (pomelo_generator_init(&context) < 0) {
        pomelo_generator_cleanup(&context);
        return -1;
    }

    int ret = run(argc, argv, &context, vectors);

    // Finalize resources and return
    pomelo_generator_cleanup(&context);
    return ret;
}
