#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <regex.h>


#define MAX_FILE_NUM 4096
#define BUF_SIZE 4096

// GLOBALS
bool recursive = false;
bool print_line_numbers = false;
char *pattern = NULL;
char *usage = "Usage:\n"
              "./grep [-rh]\n"
              "-h     Show help message\n"
              "-r     Recursively search through directory structure\n"
              "-n     Include line numbers\n";

char *parse_args(int argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, "rn")) != -1) {
        switch (opt) {
            case 'r':
                recursive = true;
                break;
            
            case 'h':
                printf("%s", usage);
                break;
            
            case 'n':
                print_line_numbers = true;
                break;

            case '?':
                printf("Error parsing command line arguments\n%s", usage);
                exit(1);
        }
    }

    // If there are not two arguments left (pattern and file name)
    if (argc - optind != 2) {
        fprintf(stderr, "Missing either pattern or file name in parsing command line arguments\n%s", usage);
        exit(1);
    }

    pattern = argv[optind];
    return argv[optind + 1];
}

void grep_file(const char *file_name) {
    FILE *file;
    char buf[BUF_SIZE]; 
    char error_msg[100];   
    int regex_errorno; // value returned from regex library functions, zero indicates success
    regex_t regex; // tmp variable to conduct regex operations
    const char *file_display_name = file_name + 2; // Remove proceeding "./" in directory string

    if (regcomp(&regex, pattern, 0)) {
        fprintf(stderr, "Regex compile failed\n");
        exit(1);
    }
    if ((file = fopen(file_name, "r")) == NULL) {
        perror("Error Opening File");        
        return;
    }

    int line_number = 1;    
    while (fgets(buf, BUF_SIZE, file)) {
        regex_errorno = regexec(&regex, buf, 0, NULL, 0);
        if (!regex_errorno) {
            if (recursive) {
                if (print_line_numbers)
                    printf("%s:%d:%s\n", file_display_name, line_number, buf);
                else 
                    printf("%s:%s\n", file_display_name, buf);
            } else {
                if (print_line_numbers)
                    printf("%d:%s\n", line_number, buf);
                else
                    printf("%s\n", buf);
            }
        } else if (regex_errorno == REG_NOMATCH) {
            continue;
        } else {
            regerror(regex_errorno, &regex, error_msg, sizeof(error_msg));
            fprintf(stderr, "match failed: %s\n", error_msg);
            exit(1);
        }
        line_number++;
    }
}

int grep_file_wrapper(const char *filename, const struct stat *statptr,
    int fileflags, struct FTW *pfwt) 
{
    if (fileflags == FTW_F)
        grep_file(filename);

    return 0; // Tells nftw to continue
}

void grep_dir(char *path) {
    /* Iterates over the directory structure starting at path and
    calls grep_file_wrapper on each file */
    nftw(path, grep_file_wrapper, MAX_FILE_NUM, 0);
}

int main(int argc, char *argv[]) {
    struct stat sb;

    char *file_name = parse_args(argc, argv);

    if (stat(file_name, &sb) == -1) {
        perror("stat");
        exit(1);
    }

    if (recursive && S_ISREG(sb.st_mode)) {
        fprintf(stderr, "%s is not a directory\n%s", file_name, usage);
        exit(1);
    }

    if (!recursive && S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "%s is not a file\n%s", file_name, usage);
        exit(1);
    }

    if (!recursive)
        grep_file(file_name);
    else
        grep_dir(file_name);
}
