/*
 * ssh-copy-id for Windows
 * Analog of ssh-copy-id utility from Linux
 * 
 * Compile:
 *   gcc -o ssh-copy-id.exe ssh-copy-id.c -lws2_32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>

#define MAX_PATH_LEN 4096
#define MAX_CMD_LEN 8192
#define MAX_KEY_SIZE 65536

/* Options structure */
typedef struct {
    char user[256];
    char host[256];
    int port;
    char identity_file[MAX_PATH_LEN];
    char key_file[MAX_PATH_LEN];
    int force;
    int dry_run;
    int quiet;
    char ssh_options[1024];
    char ssh_config[MAX_PATH_LEN];
} Options;

/* Function prototypes */
void print_help(const char *prog_name);
int parse_target(const char *target, Options *opts);
int get_public_key_path(Options *opts, char *key_path, size_t key_path_size);
int read_public_key(const char *key_path, char *key_content, size_t key_size);
int check_ssh_installed(void);
int run_ssh_command(Options *opts, const char *remote_cmd);
int copy_key_to_server(Options *opts, const char *key_content);
int test_connection(Options *opts);
char* get_home_dir(void);
int file_exists(const char *path);
void trim_string(char *str);

/* Get home directory */
char* get_home_dir(void) {
    static char home[MAX_PATH_LEN];
    const char *home_env = getenv("USERPROFILE");
    if (home_env) {
        strncpy(home, home_env, MAX_PATH_LEN - 1);
        home[MAX_PATH_LEN - 1] = '\0';
        return home;
    }
    return NULL;
}

/* Check if file exists */
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

/* Trim leading and trailing whitespace */
void trim_string(char *str) {
    char *start = str;
    char *end;
    size_t len;
    
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }
    
    len = strlen(start);
    end = start + len - 1;
    
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* Print help message */
void print_help(const char *prog_name) {
    printf("Usage: %s [options] [user@]host\n\n", prog_name);
    printf("Copy your public SSH key to a remote server\n\n");
    printf("Options:\n");
    printf("  -i, --identity_file <file>   Use this public key (default: ~/.ssh/id_rsa.pub)\n");
    printf("  -p, --port <port>            SSH port (default: 22)\n");
    printf("  -f, --force                  Don't check for existing keys\n");
    printf("  -n, --dry_run                Show what would be done, but don't execute\n");
    printf("  -q, --quiet                  Quiet mode\n");
    printf("  -o, --ssh_options <options>  Additional SSH options\n");
    printf("  -F, --ssh_config <file>      SSH configuration file\n");
    printf("  -h, --help                   Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s user@example.com\n", prog_name);
    printf("  %s -i ~/.ssh/id_ed25519.pub user@192.168.1.100\n", prog_name);
    printf("  %s -p 2222 -f root@server.local\n", prog_name);
}

/* Parse target string user@host */
int parse_target(const char *target, Options *opts) {
    const char *at_sign = strchr(target, '@');
    
    if (at_sign) {
        size_t user_len = at_sign - target;
        if (user_len >= sizeof(opts->user)) {
            user_len = sizeof(opts->user) - 1;
        }
        strncpy(opts->user, target, user_len);
        opts->user[user_len] = '\0';
        strncpy(opts->host, at_sign + 1, sizeof(opts->host) - 1);
        opts->host[sizeof(opts->host) - 1] = '\0';
    } else {
        const char *username = getenv("USERNAME");
        if (username) {
            strncpy(opts->user, username, sizeof(opts->user) - 1);
            opts->user[sizeof(opts->user) - 1] = '\0';
        } else {
            strcpy(opts->user, "user");
        }
        strncpy(opts->host, target, sizeof(opts->host) - 1);
        opts->host[sizeof(opts->host) - 1] = '\0';
    }
    
    return 0;
}

/* Get public key file path */
int get_public_key_path(Options *opts, char *key_path, size_t key_path_size) {
    char *home_dir = get_home_dir();
    
    if (opts->key_file[0] != '\0') {
        strncpy(key_path, opts->key_file, key_path_size - 1);
        key_path[key_path_size - 1] = '\0';
    } else if (opts->identity_file[0] != '\0') {
        strncpy(key_path, opts->identity_file, key_path_size - 1);
        key_path[key_path_size - 1] = '\0';
        if (!strstr(key_path, ".pub")) {
            strncat(key_path, ".pub", key_path_size - strlen(key_path) - 1);
        }
    } else {
        snprintf(key_path, key_path_size, "%s\\.ssh\\id_rsa.pub", home_dir ? home_dir : ".");
    }
    
    if (key_path[0] == '~') {
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s%s", home_dir ? home_dir : "", key_path + 1);
        strncpy(key_path, full_path, key_path_size - 1);
        key_path[key_path_size - 1] = '\0';
    }
    
    return 0;
}

/* Read public key from file */
int read_public_key(const char *key_path, char *key_content, size_t key_size) {
    FILE *fp = fopen(key_path, "r");
    if (!fp) {
        return -1;
    }
    
    size_t bytes_read = fread(key_content, 1, key_size - 1, fp);
    fclose(fp);
    
    if (bytes_read == 0) {
        return -1;
    }
    
    key_content[bytes_read] = '\0';
    trim_string(key_content);
    
    return 0;
}

/* Check if SSH client is installed */
int check_ssh_installed(void) {
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "where ssh >nul 2>&1");
    return system(cmd) == 0;
}

/* Execute SSH command */
int run_ssh_command(Options *opts, const char *remote_cmd) {
    char cmd[MAX_CMD_LEN];
    char port_str[32] = "";
    char config_str[MAX_CMD_LEN] = "";
    char opts_str[256] = "";
    
    if (opts->port > 0 && opts->port != 22) {
        snprintf(port_str, sizeof(port_str), "-p %d ", opts->port);
    }
    
    if (opts->ssh_config[0] != '\0') {
        snprintf(config_str, sizeof(config_str), "-F \"%s\" ", opts->ssh_config);
    }
    
    if (opts->ssh_options[0] != '\0') {
        snprintf(opts_str, sizeof(opts_str), "-o %s ", opts->ssh_options);
    }
    
    snprintf(cmd, sizeof(cmd), "ssh %s%s%s-o StrictHostKeyChecking=accept-new %s@%s \"%s\"",
             config_str, port_str, opts_str, opts->user, opts->host, remote_cmd);
    
    return system(cmd);
}

/* Copy key to server */
int copy_key_to_server(Options *opts, const char *key_content) {
    char cmd[MAX_CMD_LEN];
    char port_str[32] = "";
    char config_str[MAX_CMD_LEN] = "";
    char escaped_key[MAX_KEY_SIZE];
    
    /* Escape key for shell */
    const char *src = key_content;
    char *dst = escaped_key;
    size_t remaining = sizeof(escaped_key) - 1;
    
    while (*src && remaining > 1) {
        if (*src == '\'') {
            if (remaining >= 5) {
                memcpy(dst, "'\\''", 4);
                dst += 4;
                remaining -= 4;
            }
        } else {
            *dst++ = *src;
            remaining--;
        }
        src++;
    }
    *dst = '\0';
    
    if (opts->port > 0 && opts->port != 22) {
        snprintf(port_str, sizeof(port_str), "-p %d ", opts->port);
    }
    
    if (opts->ssh_config[0] != '\0') {
        snprintf(config_str, sizeof(config_str), "-F \"%s\" ", opts->ssh_config);
    }
    
    /* Create .ssh directory */
    printf("Creating ~/.ssh directory...\n");
    snprintf(cmd, sizeof(cmd), "ssh %s%s%s@%s \"mkdir -p ~/.ssh && chmod 700 ~/.ssh\"",
             config_str, port_str, opts->user, opts->host);
    system(cmd);
    
    if (opts->force) {
        printf("Adding key to authorized_keys...\n");
        snprintf(cmd, sizeof(cmd), "ssh %s%s%s@%s \"echo '%s' >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys\"",
                 config_str, port_str, opts->user, opts->host, escaped_key);
        return system(cmd);
    } else {
        /* Check for existing key */
        char check_cmd[MAX_CMD_LEN];
        snprintf(check_cmd, sizeof(check_cmd), "ssh %s%s%s@%s \"cat ~/.ssh/authorized_keys 2>/dev/null\"",
                 config_str, port_str, opts->user, opts->host);
        
        FILE *fp = _popen(check_cmd, "r");
        if (fp) {
            char existing_keys[MAX_KEY_SIZE] = "";
            size_t bytes_read = fread(existing_keys, 1, sizeof(existing_keys) - 1, fp);
            existing_keys[bytes_read] = '\0';
            _pclose(fp);
            
            if (strstr(existing_keys, key_content)) {
                printf("Key already exists on server\n");
                return 0;
            }
        }
        
        printf("Adding key to authorized_keys...\n");
        snprintf(cmd, sizeof(cmd), "ssh %s%s%s@%s \"echo '%s' >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys\"",
                 config_str, port_str, opts->user, opts->host, escaped_key);
        return system(cmd);
    }
}

/* Test connection */
int test_connection(Options *opts) {
    char cmd[MAX_CMD_LEN];
    char port_str[32] = "";
    char config_str[MAX_CMD_LEN] = "";
    char private_key[MAX_PATH_LEN];
    
    if (opts->port > 0 && opts->port != 22) {
        snprintf(port_str, sizeof(port_str), "-p %d ", opts->port);
    }
    
    if (opts->ssh_config[0] != '\0') {
        snprintf(config_str, sizeof(config_str), "-F \"%s\" ", opts->ssh_config);
    }
    
    get_public_key_path(opts, private_key, sizeof(private_key));
    char *pub_pos = strstr(private_key, ".pub");
    if (pub_pos) {
        *pub_pos = '\0';
    }
    
    printf("Testing connection with key...\n");
    snprintf(cmd, sizeof(cmd), "ssh %s%s-i \"%s\" -o BatchMode=yes %s@%s \"exit 0\"",
             config_str, port_str, private_key, opts->user, opts->host);
    
    return system(cmd);
}

/* Parse command line arguments */
int parse_args(int argc, char *argv[], Options *opts) {
    int i;
    int target_found = 0;
    
    memset(opts, 0, sizeof(Options));
    opts->port = 22;
    strcpy(opts->ssh_options, "");
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--identity_file") == 0) {
            if (i + 1 < argc) {
                strncpy(opts->identity_file, argv[++i], sizeof(opts->identity_file) - 1);
            }
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                opts->port = atoi(argv[++i]);
            }
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            opts->force = 1;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--dry_run") == 0) {
            opts->dry_run = 1;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            opts->quiet = 1;
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--ssh_options") == 0) {
            if (i + 1 < argc) {
                strncpy(opts->ssh_options, argv[++i], sizeof(opts->ssh_options) - 1);
            }
        }
        else if (strcmp(argv[i], "-F") == 0 || strcmp(argv[i], "--ssh_config") == 0) {
            if (i + 1 < argc) {
                strncpy(opts->ssh_config, argv[++i], sizeof(opts->ssh_config) - 1);
            }
        }
        else if (argv[i][0] != '-' && !target_found) {
            parse_target(argv[i], opts);
            target_found = 1;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help(argv[0]);
            return -1;
        }
    }
    
    if (!target_found && opts->host[0] == '\0') {
        fprintf(stderr, "No host specified. Usage: %s user@host\n", argv[0]);
        print_help(argv[0]);
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    Options opts;
    char key_path[MAX_PATH_LEN];
    char key_content[MAX_KEY_SIZE];
    int result;
    
    /* Initialize Winsock */
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    /* Parse arguments */
    if (parse_args(argc, argv, &opts) != 0) {
        WSACleanup();
        return 1;
    }
    
    /* Check SSH client */
    if (!check_ssh_installed()) {
        fprintf(stderr, "SSH client not found. Please install OpenSSH for Windows.\n");
        WSACleanup();
        return 1;
    }
    
    /* Get key path */
    get_public_key_path(&opts, key_path, sizeof(key_path));
    
    if (!opts.quiet) {
        printf("Copying key: %s\n", key_path);
        printf("To server: %s@%s", opts.user, opts.host);
        if (opts.port > 0 && opts.port != 22) {
            printf(":%d", opts.port);
        }
        printf("\n");
    }
    
    /* Dry run */
    if (opts.dry_run) {
        printf("[DRY RUN] Key would be added to ~/.ssh/authorized_keys\n");
        WSACleanup();
        return 0;
    }
    
    /* Read public key */
    if (read_public_key(key_path, key_content, sizeof(key_content)) != 0) {
        fprintf(stderr, "Public key not found: %s\n", key_path);
        
        char private_key[MAX_PATH_LEN];
        strncpy(private_key, key_path, sizeof(private_key) - 1);
        char *pub_pos = strstr(private_key, ".pub");
        if (pub_pos) {
            *pub_pos = '\0';
        }
        
        if (file_exists(private_key)) {
            fprintf(stderr, "Private key found: %s\n", private_key);
            fprintf(stderr, "Generate public key: ssh-keygen -y -f %s > %s\n", 
                    private_key, key_path);
        }
        
        WSACleanup();
        return 1;
    }
    
    /* Test connection */
    if (!opts.quiet) {
        printf("Testing connection...\n");
    }
    
    result = run_ssh_command(&opts, "exit 0");
    if (result != 0) {
        fprintf(stderr, "Failed to connect to server. Check login credentials.\n");
        WSACleanup();
        return 1;
    }
    
    /* Copy key */
    result = copy_key_to_server(&opts, key_content);
    
    if (result == 0) {
        if (!opts.quiet) {
            printf("Key copied successfully!\n");
            
            if (test_connection(&opts) == 0) {
                printf("Connection with key works!\n");
            } else {
                printf("Connection with key failed.\n");
            }
        }
    } else {
        fprintf(stderr, "Error copying key\n");
        WSACleanup();
        return 1;
    }
    
    WSACleanup();
    return 0;
}
