/*
 * Copyright (C) 2013,2014 CESNET
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
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <libtrap/trap.h>
#include <trap_internal.h>

// Struct with information about module
trap_module_info_t module_info = {
   "TCPIP Example client module", // Module name
   // Module description
   "",
   1, // Number of input interfaces
   1, // Number of output interfaces
};

static char stop = 0;

#ifndef TESTBUFFERING
#define TESTBUFFERING
#endif

int main(int argc, char **argv)
{
   int ret;

   trap_ifc_spec_t ifc_spec;
   //char *ifc_params[] = {"traptestshm,650000;traptestshm,650000"};
   //ifc_spec.types = "mm";
   char *ifc_params[3] = { argv[0], "-i", "tt;localhost,11111,65533;11111,5,65533" };
   int paramno = 3;

   uint64_t counter = 0;
   uint64_t iteration = 0;
   time_t duration;
   uint16_t payload_size;

   char *payload = NULL;

   //verbose = CL_VERBOSE_LIBRARY;
   trap_verbose = CL_VERBOSE_OFF;
   VERBOSE(CL_VERBOSE_OFF, "%s [number]\nnumber - size of data to send for testing", argv[0]);

   ret = trap_parse_params(&paramno, ifc_params, &ifc_spec);
   if (ret != TRAP_E_OK) {
      if (ret == TRAP_E_HELP) { // "-h" was found
         trap_print_help(&module_info);
         return 0;
      }
      fprintf(stderr, "ERROR in parsing of parameters for TRAP: %s\n", trap_last_error_msg);
      return 1;
   }
   // Initialize TRAP library (create and init all interfaces)
   ret = trap_init(&module_info, ifc_spec);
   if (ret != TRAP_E_OK) {
      fprintf(stderr, "ERROR in TRAP initialization %s\n", trap_last_error_msg);
      return 1;
   }

   duration = time(NULL);

   // Read data from input, process them and write to output
   while(!stop) {
      ret = trap_get_data(0, (const void **) &payload, &payload_size, TRAP_WAIT);
      if (ret != TRAP_E_OK) {
         VERBOSE(CL_ERROR, "error: %s", trap_last_error_msg);
      }
      ret = trap_send_data(0, (void *) payload, payload_size, TRAP_WAIT);
      if (ret == TRAP_E_OK) {
         //printf("\r%llu", (unsigned long long int) counter);
         counter++;
      } else {
         VERBOSE(CL_ERROR, "error: %s", trap_last_error_msg);
      }
      iteration++;
      if (counter == 10) {
         break;
      }
   }
   duration = time(NULL) - duration;

   printf("Number of iterations: %llu\nLast sent: %llu\nTime: %llus\n",
      (unsigned long long int) iteration,
      (unsigned long long int) counter-1,
      (unsigned long long int) duration);
   sleep(2);

   // Do all necessary cleanup before exiting
   // (close interfaces and free allocated memory)
   trap_finalize();
   //free(payload);

   return 0;
}

