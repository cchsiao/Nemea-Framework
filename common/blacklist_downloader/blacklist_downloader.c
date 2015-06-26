/**
 * \file blacklist_downloader.c
 * \brief Functions for downloading blacklist from website.
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \date 2014
 */

/*
 * Copyright (C) 2013 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <regex.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../include/blacklist_downloader.h"


static pid_t P_ID;
static pid_t C_ID;

static int STOP = 0;

static char COMMAND_INIT[]    = "wget -q -O - ";
static int  COMMAND_INIT_SIZE = sizeof(COMMAND_INIT);


static bl_down_config_t *CONFIG;

static int child_exit;

/**
 * \brief Function for handling incoming signals.
 * \param signal Incoming signal.
 */
void bl_sig_handler(int signal)
{
   if (signal == SIGINT) {
      STOP = 1;
   } else if (signal == SIGUSR1) {
      printf("Force update initiated.\n");
   } else if (signal == SIGCHLD) {
      child_exit = 1;
   }
}

/**
 * \brief Fill lookup table for blacklist flags.
 * \param config Configuration structure for blacklist downloader.
 * \param in     Bitfield of used blacklists.
 * \return Count of used blacklists.
 */
uint8_t fill_lookup(bl_down_config_t *config, uint64_t in)
{
   uint8_t out = 0;
   int index = 0;
   for (int i = 0; i < 64; i++) {
      if ((1ULL << i) & in) {
         config->lut_id[index++] = (in & (1ULL << i));
      }
      out += (in & (1ULL << i)) >> i;
   }
   return out;
}


/**
 * \brief Create array of strings of blacklist sites.
 * \param sites     Bitfield of used blacklists.
 * \param source_ar Array to fill with sources.
 * \param stype_ar  Array to fill with sources type.
 */
void get_sources_info(uint64_t sites, char **source_ar, int *stype_ar)
{
   int index = 0;
   for (int i = 0; i < BL_BLACKLIST_ELEM_COUNT; i++) {
      if ((1ULL << i) & sites) {
         stype_ar[index] = BL_ELEM_AR[i]->source_type;
         source_ar[index++] = BL_ELEM_AR[i]->source;
      }
   }
}

/**
 * \brief Parse path to file string and return string without filename.
 * \param path_to_file Path to file string with filename.
 * \return Path without filename string.
 *
 */
static char *get_dir_str(char *path_to_file)
{
   char *start = path_to_file;
   char *end = start;
   char *return_path;
   while (*path_to_file != 0) {
      if (*path_to_file == '/') {
         end = path_to_file;
      }
      path_to_file++;
   }

   if (start == end) {
      // Only filename was given
      return_path = malloc(strlen("./") + 1);
      memcpy(return_path, "./", 3); // copy with terminating byte
   } else {
      // Copy path
      return_path = malloc(end - start + 2);
      memcpy(return_path, start, end - start + 1);
      return_path[end - start + 1] = 0;
   }

   return return_path;
}


/**
 * \brief Create filename with give name and write warden receiver perl script into it.
 *        Format of filename is concatenation of parameters (`path_str``pid_str``infix_str``source_str``suffix_str`).
 * \param config     Blacklist Downloader config structure.
 * \param path_str   Path to file str (without filename).
 * \param pid_str    PID of current process.
 * \param infix_str  Infix (default is "-BLD-").
 * \param source_str Type of warden source.
 * \param suffix_str Suffix (default is ".pl").
 * \return 1 on success, 0 on fail.
 */
int create_warden_recv_file(bl_down_config_t *config, char *path_str, char *pid_str, char *infix_str, char *source_str, char *suffix_str)
{
   int ret;
   char *filename = malloc(strlen(path_str) + strlen(pid_str) + strlen(infix_str) + strlen(source_str) + strlen(suffix_str) + 1);
   strcat(filename, path_str);
   strcat(filename, pid_str);
   strcat(filename, infix_str);
   strcat(filename, source_str);
   strcat(filename, suffix_str);
   filename[strlen(path_str) + strlen(pid_str) + strlen(infix_str) + strlen(source_str) + strlen(suffix_str)] = 0;

   FILE *fp = fopen(filename, "w");
   if (fp != NULL) {
      fprintf(fp, "%s", BL_WARDEN_RECV_FILE_PL_SOURCE_CODE);
      fclose(fp);
      ret = 1;
      chmod(filename, S_IRWXU);
      config->warden_scripts.fnames[config->warden_scripts.count++] = filename;
   } else {
      fprintf(stderr, "Error: Could not create file: %s!\n", filename);
      ret = 0;
   }

   return ret;
}




/**
 * \brief Write array of strings to file.
 * \param el_ar     Array of strings.
 * \param el_count  Number of elements in `el_ar`.
 * \param file_name Name of the file to which will strings be printed.
 */
void write_arr_to_file(char **el_ar, uint32_t *blf_ar, int el_count, char *file_name)
{
   FILE *fp = fopen(file_name, "w");
   if (fp == NULL) {
      fprintf(stderr, "Error: Could not open file: %s!\n", file_name);
      return;
   }
   // Write to new file
   for (int i = 0; i < el_count; i++) {
      fprintf(fp, "%s,%u\n", el_ar[i], blf_ar[i]);
   }
   fclose(fp);
}


/**
 * \brief Substract string array `el_ar2` from string array `el_ar1`
 *        and result print to file `fp`.
 * \param el_ar1    Array of strings.
 * \param el_count1 Number of elements in `el_ar1`.
 * \param el_ar2    Array of strings.
 * \param el_count2 Number of elements in `el_ar2`.
 * \param fp        File to which will result be printed.
 */
void update_diff(char **el_ar1, int el_count1, char **el_ar2, int el_count2, uint32_t *blf_ar1, FILE *fp)
{
   int match;

   // Cycle throu every element in `el_ar1`
   for (int i = 0; i < el_count1; i++) {
      match = 0;
      // Cycle throu every element in `el_ar2`
      for (int j = 0; j < el_count2; j++) {
         // Compare elements
         if (strcmp(el_ar1[i], el_ar2[j]) == 0) {
            match = 1; // We have found a match and do not need to continue futher
            break;
         }
      }
      if (!match) {
         // Element was present in `el_ar1` but not in `el_ar2`, print it to file
         fprintf(fp, "%s,%u\n", el_ar1[i], blf_ar1[i]);
      }
   }
}


/**
 * \brief Trim whitespace characters from string.
 * \param str String to modify.
 * \return Pointer to modified.
 */
char *trim_whitespace(char *str)
{
   char *end;

   // Trim leading whitespace characters
   while (isspace(*str)) {
      str++;
   }

   // Check if string was containing only whitespace characters
   if (*str == 0) {
      return str;
   }

   // Trim trailing whitespace characters
   end = str + strlen(str) - 1;
   while (end > str && isspace(*end)) {
      end--;
   }

   // Terminate new string
   *(end+1) = 0;

   return str;
}


/**
 * \brief Find comment in string and remove it.
 * \param str String to parse.
 * \param c   Comment character.
 */
void filter_comments(char **str, char c)
{
   char *pos = strchr(*str, c);
   if (pos) {
      *pos = 0;
   }
}


/**
 * \brief Check if string `str` is in array `ar`.
 * \param ar   Array of strings.
 * \param size Number of elements in array.
 * \param str  String to be checked.
 * \return 1 if string is not in array, otherwise 0.
 */
int uniq_line(char **ar, int size, char *str)
{
   for (int i = 0; i < size; i++) {
      if (strcmp(ar[i], str) == 0) {
         return i;
      }
   }

   return -1;
}


/**
 * \brief Change string based on regular expression. If regular expression
 *        succeed, then string will be changed to contain only matched result.
 *        If regular expression fails, then string will be reduced to zero length.
 * \param str Pointer to string.
 * \return Changed string.
 */
char *filter_regex(char **str)
{
   regmatch_t match[10];
   int res = regexec(&CONFIG->preg, *str, 10, match, 0);

   if (res == REG_NOMATCH) {
      // No match found, erase str
      (*str)[0] = 0;
      return *str;
   }

   // Match found, change string to contain only matched result
   (*str)[match[0].rm_eo] = 0;
   return (*str) + match[0].rm_so;
}


/**
 * \brief Parse output from wget, trim whitespaces and
 *        filter comments.
 * \param fd      File descriptor of wget output.
 * \param line    Line buffer.
 * \param el_ar   Element buffer.
 * \param c       Comment character.
 * \param index   Index of first empty index in element buffer.
 * \param bl_flag Blacklist flag.
 * \return Count of legit IP addresses.
 */
int bl_down_parse(FILE *fd, char *line, char **el_ar, uint32_t *blf_ar, char c, int index, int bl_flag)
{
   int i = index;
   char *trim_line;
   int uniq_index;
   while (fgets(line, CONFIG->buf.line_max_length, fd) != NULL) {
      trim_line = trim_whitespace(line);
      filter_comments(&trim_line, c);

      if (CONFIG->use_regex) {
         trim_line = filter_regex(&trim_line);
      }

      if (strlen(trim_line) > 0) {
         trim_line[CONFIG->buf.el_max_length] = 0; // Cut line to max element length
         if ((uniq_index = uniq_line(el_ar, i, trim_line)) == -1) {
            // Line is unique, add it to element array
            blf_ar[i] = bl_flag;
            memcpy(el_ar[i], trim_line, strlen(trim_line) + 1); // Copy with terminating byte
            i++;
         } else {
            // Line is already in element array, update blaklist flag
            blf_ar[uniq_index] |= bl_flag;
         }
      }
   }

   return i;
}


/**
 * \brief Free config structure. Because config structure is global
 *        there is no need for any arguments.
 */
void bl_down_destroy_config()
{
   // Free warden script names a delete files
   for (int i = 0; i < CONFIG->warden_scripts.count; i++) {
      remove(CONFIG->warden_scripts.fnames[i]);
      free(CONFIG->warden_scripts.fnames[i]);
   }
   // Free command strings and file names
   for (int i = 0; i < CONFIG->cmd.cnt; i++) {
      free(CONFIG->cmd.ar[i]);
   }
   // Free element buffers
   for (int i = 0; i < CONFIG->buf.el_max_count; i++) {
      free(CONFIG->buf.el_ar[0][i]);
      free(CONFIG->buf.el_ar[1][i]);
   }
   free(CONFIG->buf.el_ar[0]);
   free(CONFIG->buf.el_ar[1]);
   free(CONFIG->buf.line);
   free(CONFIG->comment_ar);
   free(CONFIG->buf.file);
   free(CONFIG->cmd.ar);
   free(CONFIG);
}


/**
 * \brief Function represents process which will download
 *        website content and store it to a file. When download
 *        is complete, process will send signal (SIGUSR1) to
 *        its parent.
 */
void bl_down_process(void)
{
   // Register signal handler
   struct sigaction sa;
   sa.sa_handler = bl_sig_handler;
   sa.sa_flags = 0;
   sigemptyset(&sa.sa_mask);
   sigaction(SIGINT,  &sa, NULL);
   sigaction(SIGUSR1, &sa, NULL);
   sigaction(SIGCHLD, &sa, NULL);

   int valid_flag;
   int first_run = 1;
   int swap_flag = 0;
   int el_count_ar[2] = {0};


   // ***** Cycle until parent process sends SIGINT *****
   while (!STOP) {
       valid_flag = 0;
       el_count_ar[swap_flag] = 0;
      // Loop throu all blacklist sites
      for (int i = 0; i < CONFIG->cmd.cnt; i++) {
         // Execute command
         child_exit = 0;
         FILE *fd = popen(CONFIG->cmd.ar[i], "r");
         if (fd == NULL) {
            fprintf(stderr, "Error: popen failed!\n");
         }
         // Wait for child to execute
         for (int c = 0; !child_exit && c < 50; c++) {
            usleep(100);
         }
         el_count_ar[swap_flag] = bl_down_parse(fd, CONFIG->buf.line, CONFIG->buf.el_ar[swap_flag], CONFIG->buf.blf_ar[swap_flag],
                                                CONFIG->comment_ar[i], el_count_ar[swap_flag], CONFIG->lut_id[i]);
         int ret = pclose(fd);
         ret = WEXITSTATUS(ret);

         // Check command exit code
         if (ret != 0) {
            fprintf(stderr, "Warning: Command \"%s\" exited with code: %d...Skipping\n", CONFIG->cmd.ar[i], ret);
         } else {
            valid_flag = 1;
         }
      }

      if (valid_flag) {
         if (first_run || CONFIG->update_mode == DEFAULT_UPDATE_MODE) {
            // Store elements to file
            write_arr_to_file(CONFIG->buf.el_ar[swap_flag], CONFIG->buf.blf_ar[swap_flag], el_count_ar[swap_flag], CONFIG->buf.file);
            first_run = 0;
         } else {
            FILE *fu = fopen(CONFIG->buf.file, "w");
            if (fu == NULL) {
               fprintf(stderr, "Error: Could not open file %s!\n", CONFIG->buf.file);
               continue;
            }
            // Substract new update from old update to get new elements
            update_diff(CONFIG->buf.el_ar[!swap_flag], el_count_ar[!swap_flag], CONFIG->buf.el_ar[swap_flag], el_count_ar[swap_flag], CONFIG->buf.blf_ar[!swap_flag], fu);

            // Add delimeter string to file
            fprintf(fu, "#remove\n");

            // Substract old update from new update to get removed elements
            update_diff(CONFIG->buf.el_ar[swap_flag], el_count_ar[swap_flag], CONFIG->buf.el_ar[!swap_flag], el_count_ar[!swap_flag], CONFIG->buf.blf_ar[swap_flag], fu);

            fclose(fu);
         }
         swap_flag = !swap_flag;
      }

      // Send update signal to parent process
      kill(P_ID, SIGUSR1);

      sleep(CONFIG->delay);
   }
   // ***************************************************


   printf("Downloader exiting...\n");

   bl_down_destroy_config();

   exit(0);
}



/**
 * \brief Allocate memory and setup config structure.
 * \param args Arguments passed in by module.
 * \return Pointer to config structure on success, NULL otherwise.
 */
bl_down_config_t *bl_down_setup_config(bl_down_args_t *args)
{
   // Create array of blacklist sites
   char *source_ar[64];
   int stype_ar[64];
   get_sources_info(args->sites, (char**)source_ar, (int *)stype_ar);

   // Convert process id to string
   char pid_str[16] = {0};
   snprintf(pid_str, 15, "%u", P_ID);

   // Allocate memory for config structure
   bl_down_config_t *config = malloc(sizeof(bl_down_config_t));
   if (config == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for config structure!\n");
      goto setup_malloc_fail_config;
   }

   // Fill lookup table for blacklist id
   uint8_t num = fill_lookup(config, args->sites);


   // Allocate memory for command array
   config->cmd.ar = malloc(sizeof(char*) * num);
   if (config->cmd.ar == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for command array!\n");
      goto setup_malloc_fail_cmd;
   }

   // Allocate memory for file name array
   config->buf.file = malloc(sizeof(char) * (strlen(args->file) + 1));
   if (config->buf.file == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for file name array!\n");
      goto setup_malloc_fail_file;
   }

   // Allocate memory for comment array
   config->comment_ar = malloc(sizeof(char) * num);
   if (config->comment_ar == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for comment array!\n");
      goto setup_malloc_fail_comment;
   }

   // Allocate memory for line buffer
   config->buf.line = malloc(sizeof(char) * args->line_max_length);
   if (config->buf.line == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for line buffer!\n");
      goto setup_malloc_fail_linebuf;
   }

   // Allocate memory for 2 blacklist element arrays
   config->buf.el_ar[0] = malloc(sizeof(char*) * args->el_max_count);
   config->buf.el_ar[1] = malloc(sizeof(char*) * args->el_max_count);
   config->buf.blf_ar[0] = malloc(sizeof(uint32_t) * args->el_max_count);
   config->buf.blf_ar[1] = malloc(sizeof(uint32_t) * args->el_max_count);
   if (config->buf.el_ar[0] == NULL || config->buf.el_ar[1] == NULL ||
       config->buf.blf_ar[0] == NULL || config->buf.blf_ar[1] == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for element buffers!\n");
      goto setup_malloc_fail_elbufitem;
   }


   // Allocate memory for each element
   for (int i = 0; i < args->el_max_count; i++) {
      config->buf.el_ar[0][i] = malloc(sizeof(char) * (args->el_max_length + 1));
      config->buf.el_ar[1][i] = malloc(sizeof(char) * (args->el_max_length + 1));
      // Check allocation
      if (config->buf.el_ar[0][i] == NULL || config->buf.el_ar[1][i] == NULL) {
         // Allocation failed, free memory
         for (int j = 0; j <= i; j++) {
            free(config->buf.el_ar[0][j]);
            free(config->buf.el_ar[1][j]);
         }
         fprintf(stderr, "Error: Could not allocate memory for element buffers!\n");
         goto setup_malloc_fail_elbufitem;
      }
   }


   // Allocate memory for command strings and file names
   for (int i = 0; i < num; i++) {
      // Determine type of source and compute command string size
      int command_size;
      int bad_type_flag = 0;
      int file_error_flag = 0;
      char *dir_str;
      switch (stype_ar[i]) {
         case BL_STYPE_WEB: // command string in format: wget -q -O - $SOURCE
                            command_size = COMMAND_INIT_SIZE + strlen(source_ar[i]) + 1;
                            break;
         case BL_STYPE_WARDEN: dir_str = get_dir_str(args->file);
                               if (!create_warden_recv_file(config, dir_str, pid_str, "-BLD-", source_ar[i], ".pl")) {
                                  file_error_flag = 1;
                               }
                               // command string in format: $PATH$PID-BLD-$SOURCE.pl $SOURCE
                               command_size = strlen(dir_str) + strlen(pid_str) + strlen("-BLD-") + strlen(source_ar[i]) + strlen(".pl ") + strlen(source_ar[i]) + 1;
                               break;
         default: fprintf(stderr, "Error: Wrong source type: %u!\n", stype_ar[i]);
                  config->cmd.ar[i] = NULL;
                  bad_type_flag = 1;
      }

      // Allocate memory for command string
      if (!bad_type_flag && !file_error_flag) {
         config->cmd.ar[i]  = malloc(sizeof(char) * command_size);
         memset(config->cmd.ar[i], 0, command_size);
      }

      // Check memory allocation
      if (config->cmd.ar[i] == NULL) {
         // malloc failed, free memory and return error
         for (int j = 0; j <= i; j++) {
            free(config->cmd.ar[i]);
         }
         fprintf(stderr, "Error: Could not allocate memory for command!\n");
         goto setup_malloc_fail_cmdstr;
      }

      // Copy command string
      switch(stype_ar[i]) {
         case BL_STYPE_WEB: memcpy(config->cmd.ar[i], COMMAND_INIT, COMMAND_INIT_SIZE);
                            strcat(config->cmd.ar[i], source_ar[i]);
                            break;
         case BL_STYPE_WARDEN: strcat(config->cmd.ar[i], dir_str);
                               strcat(config->cmd.ar[i], pid_str);
                               strcat(config->cmd.ar[i], "-BLD-");
                               strcat(config->cmd.ar[i], source_ar[i]);
                               strcat(config->cmd.ar[i], ".pl ");
                               strcat(config->cmd.ar[i], source_ar[i]);
                               free(dir_str);
                               break;
      }
      // Copy comment
      config->comment_ar[i] = args->comment_ar[i];
   }

   // Copy file name
   memcpy(config->buf.file, args->file, strlen(args->file) + 1);


   // Compile regex
   if (args->use_regex) {
      int res;
      char reg_err_buf[100];
      if ((res = regcomp(&config->preg, args->reg_pattern, REG_EXTENDED)) != 0) {
         regerror(res, &config->preg, reg_err_buf, 99);
         fprintf(stderr, "Error: %s\n", reg_err_buf);
         goto setup_malloc_fail_cmdstr;
      }
   }

   // Set remaining parameters
   config->delay = args->delay;
   config->update_mode = args->update_mode;
   config->use_regex = args->use_regex;
   config->cmd.cnt = num;
   config->buf.line_max_length = args->line_max_length;
   config->buf.el_max_length = args->el_max_length;
   config->buf.el_max_count = args->el_max_count;

   // Line length cannot be shorter than element length
   if (config->buf.el_max_length > config->buf.line_max_length) {
      // Set element length to line length
      config->buf.el_max_length = config->buf.line_max_length;
   }

   return config;

// Memory deallocation if malloc failed
setup_malloc_fail_cmdstr:
   for (int j = 0; j < args->el_max_count; j++) {
      free(config->buf.el_ar[0][j]);
      free(config->buf.el_ar[1][j]);
   }
   for (int j = 0; j < config->warden_scripts.count; j++) {
      free(config->warden_scripts.fnames[j]);
   }
setup_malloc_fail_elbufitem:
   free(config->buf.el_ar[0]);
   free(config->buf.el_ar[1]);
   free(config->buf.blf_ar[0]);
   free(config->buf.blf_ar[1]);
setup_malloc_fail_elbuf:
   free(config->buf.line);
setup_malloc_fail_linebuf:
   free(config->comment_ar);
setup_malloc_fail_comment:
   free(config->buf.file);
setup_malloc_fail_file:
   free(config->cmd.ar);
setup_malloc_fail_cmd:
   free(config);
setup_malloc_fail_config:
   return NULL;
}

/**
 * \brief Function downloads content from provided website by creating
 *        new process and array of command strings that will be executed
 *        by this new process. Content of each website will be stored in
 *        provided file on same index, i.e. content of website on index 0
 *        in `page` will be stored to file on index 0 in `file`.
 * \param page  Array of websites from which will created process download blacklists.
 * \param file  Array of files names to which will creted process stores downloaded blacklists.
 * \param num   Number of websites/files.
 * \param delay Time to wait between updates (in seconds).
 * \return ID of created process on success, -1 otherwise.
 */
int bl_down_init(bl_down_args_t *args)
{
   P_ID = getpid();

   // Setup config structure
   CONFIG = bl_down_setup_config(args);

   if (CONFIG == NULL) {
      return -1; // Error occured
   }


   C_ID = fork();

   if (C_ID >= 0) {
      // For was successful
      if (C_ID == 0) {
         // Child process
         bl_down_process();
      } else {
         // Parent process
         return C_ID;
      }
   } else {
      // Fork failed
      return -1;
   }

}
