#ifndef BLUEFTL_USER_FTL_MAIN_H
#define BLUEFTL_USER_FTL_MAIN_H

#include "blueftl_user.h"
#define __DBG__ 	1

int32_t blueftl_user_ftl_create (struct ssd_params_t* ptr_ssd_params);

void blueftl_user_ftl_destroy ();

struct ftl_base_t _ftl_base;

int32_t blueftl_user_ftl_main (
	uint8_t req_dir, 
	uint32_t lpa_begin, 
	uint32_t lpa_end, 
	uint8_t* ptr_buffer);

void dbg_print(char *s);
#endif
