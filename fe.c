/*
 * Copyright (c) 2015, Adrian Nackov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ftw.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_DIRS_OPEN 200
#define MAX_LINE_LEN 2056

#define ANSI_COLOR_YELLOW "\e[33m"
#define ANSI_COLOR_GREEN  "\e[32m"
#define ANSI_COLOR_RESET  "\e[0m"


static regex_t SEARCH_RE;

inline static void print_info(FILE *fd, const char* fpath) {
    unsigned long line_num = 0;
    char line[MAX_LINE_LEN];
    int file_name_printed = 0;
    while (fgets(line, sizeof(line), fd)) {
        line_num += 1;
        int matched = !(regexec(&SEARCH_RE, line, 0, NULL, 0));
        if (matched) {
            if (!file_name_printed) {
               printf("%s%s%s:\n", ANSI_COLOR_GREEN, fpath, ANSI_COLOR_RESET);
               file_name_printed = 1;
            }
            printf("%s%lu:%s%s", ANSI_COLOR_YELLOW, line_num, 
                    ANSI_COLOR_RESET, line);
        }
    }
}

static int process_file(const char *fpath, const struct stat *sb,
                        int tflag, struct FTW *ftwbuf) {
    int is_file = (tflag == FTW_F);
    if (!is_file)
        return 0;

    FILE* fd = fopen(fpath, "r");
    if (fd == NULL) {
        perror("Error opening file for reading");
        return 0;
    }
    print_info(fd, fpath);
    fclose(fd);
    return 0; // To tell nftw() to continue
}

static int walk_dir_recursively(const char *dir) {
    int flags = 0;
    flags |= FTW_PHYS; // don't follow symlinks
    
    if (nftw(dir, process_file, MAX_DIRS_OPEN, flags)
            == -1) {
        perror("nftw");
        return(EXIT_FAILURE);
    }
    return(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: expected <re> <dir>, given 0 arguments.\n");
        exit(EXIT_FAILURE);
    }
    // compile re
    char *cmdline_re = argv[1];
    regcomp(&SEARCH_RE, cmdline_re, REG_EXTENDED);
    // do the searching
    int result = walk_dir_recursively((argc < 3) ? "." : argv[2]);
    regfree(&SEARCH_RE);
    return result;
}
