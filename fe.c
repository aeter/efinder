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
#include <string.h>
#include <unistd.h>

#define MAX_DIRS_OPEN 200
#define MAX_LINE_LEN 4096
#define NUM_RE_GROUP_MATCHES 1

#define ANSI_COLOR_YELLOW "\e[33m"
#define ANSI_COLOR_GREEN  "\e[32m"
#define ANSI_COLOR_RESET  "\e[0m"
#define ANSI_COLOR_LIGHT_BLUE_BACKGROUND "\e[104m"

static regex_t SEARCH_RE;

inline static void print_matching_lines(FILE *fd, const char *fpath) {
    unsigned long line_num = 0;
    char line[MAX_LINE_LEN];
    int file_name_printed = 0;
    regmatch_t match_offsets[NUM_RE_GROUP_MATCHES];
    while (fgets(line, sizeof(line), fd)) {
        line_num += 1;
        int matched = !(regexec(&SEARCH_RE, line, NUM_RE_GROUP_MATCHES, match_offsets, 0));

        // if redirected to a pipe, print simpler
        if (matched && !isatty(fileno(stdout))) {
            fprintf(stdout, "%s:%lu:%s", fpath, line_num, line);
        }
        // else print with colors to the terminal.
        else if (matched && isatty(fileno(stdout))) {
            if (!file_name_printed  && isatty(fileno(stdin))) {
               fprintf(stdout, "%s%s%s:\n",
                       ANSI_COLOR_GREEN, fpath, ANSI_COLOR_RESET);
               file_name_printed = 1;
            }
            fprintf(stdout, "%s%lu:%s", ANSI_COLOR_YELLOW, line_num, ANSI_COLOR_RESET);
            // try to match all occurences of the regex pattern on the line
            // (like the flag //g in Perl), cause the regex lib lacks such flag
            char *line_ptr = line;
            while (matched) {
                int line_matches_len = match_offsets[0].rm_eo - match_offsets[0].rm_so;
                fprintf(stdout, "%.*s", (int)match_offsets[0].rm_so, line_ptr);
                fprintf(stdout, "%s", ANSI_COLOR_LIGHT_BLUE_BACKGROUND);
                fprintf(stdout, "%.*s", line_matches_len, line_ptr + match_offsets[0].rm_so);
                fprintf(stdout, "%s", ANSI_COLOR_RESET);
                line_ptr = line_ptr + match_offsets[0].rm_eo;
                matched = !(regexec(&SEARCH_RE, line_ptr,
                            NUM_RE_GROUP_MATCHES, match_offsets, 0));
            }
            line_ptr[strlen(line_ptr) - 1] = '\n'; // shorten lines without \n
            fprintf(stdout, "%s", line_ptr);
        }
    }
}

static int process_file(const char *fpath, const struct stat *sb,
                        int tflag, struct FTW *ftwbuf) {
    int is_file = (tflag == FTW_F);
    if (!is_file)
        return 0;

    // don't search .git dirs
    int is_git_dir = (strstr(fpath, "/.git/") != NULL);
    if (is_git_dir)
        return 0;

    FILE *fd = fopen(fpath, "r");
    if (fd == NULL) {
        perror("Error opening file for reading");
        return 0;
    }
    print_matching_lines(fd, fpath);
    fclose(fd);
    return 0; // To tell nftw() to continue
}

static int walk_dir_recursively(const char *dir) {
    int flags = 0;
    flags |= FTW_PHYS; // don't follow symlinks
    
    if (nftw(dir, process_file, MAX_DIRS_OPEN, flags)
            == -1) {
        perror("nftw");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: expected <re> <dir>, given 0 arguments.\n");
        exit(EXIT_FAILURE);
    }
    // compile re
    char *cmdline_re = argv[1];
    if (regcomp(&SEARCH_RE, cmdline_re, REG_EXTENDED)) {
        perror("Could not compile regular expression: ");
        return EXIT_FAILURE;
    }
    // do the searching
    int exit_code = 0;
    if (isatty(fileno(stdin))) {
        exit_code = walk_dir_recursively((argc < 3) ? "." : argv[2]);
    }
    else { // stdin is piped
        print_matching_lines(stdin, "");
    }
    regfree(&SEARCH_RE);
    return exit_code;
}
