#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h> //  our new library 
#include <unistd.h>
#include <math.h>
////
#include "blueftl_user.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_util.h"
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_mapping_block.h"
#include "blueftl_char.h"
#include "blueftl_wl_dual_pool.h"
#include "blueftl_gc_page.h"
#include "blueftl_read_write_mgr.h"
#include "lzrw3.h"
#include "blueftl_user_ftl_main.h"

struct ftl_context_t* _ptr_ftl_context = NULL;

extern int8_t _is_debugging_mode;

struct sigaction act_new;
struct sigaction act_old;

void dbg_print(char *s)
{
#if __DBG__
	printf("%s\n",s);
#endif
}

void printall(int sig){ // can be called asynchronously
	struct flash_ssd_t *ptr_ssd = _ptr_ftl_context->ptr_ssd;
	uint32_t i,bus=0;
	uint32_t chip=0;
	uint32_t block=0, page=0;;
	uint32_t max_count=0;
	// double d[1024];

#if WL_ON
	printf("DUALPOOL THRESH: %d\n", WEAR_LEVELING_THRESHOLD);
#else
	printf("NO WEAR LEVELING: %d\n", WL_ON);
#endif

	signal(SIGTSTP, SIG_IGN);
	struct ftl_page_mapping_context_t *context = (struct ftl_page_mapping_context_t *)_ptr_ftl_context->ptr_mapping;
	uint32_t prev_pg,prev_block;
	uint32_t temp[4], j;
	
	for(i=0; i<65536; i++){
		if(context->ptr_pg_table[i] == -1){
			printf("%d ", i);
		}
	}
	printf("\n");
	// ftl_convert_to_ssd_layout(context->ptr_pg_table[0], &bus, &chip, &block, &page);
	// prev_block = block;
	// prev_pg = page;
	// temp[0] = 0;
	// j=1;
	
	// for(i=1; i<1024*64; i++){
	// 	if(context->ptr_pg_table[i] == -1) continue;
	// 	ftl_convert_to_ssd_layout(context->ptr_pg_table[i], &bus, &chip, &block, &page);
	// 	if(page == prev_pg && block == prev_block) {
	// 		temp[j++] = i;
	// 	} else {
	// 		printf("%5u, %5u, %5u, %5u, ,%4u, %2u\n", temp[0], temp[1], temp[2] ,temp[3], prev_block, prev_pg);
	// 		prev_pg = page;
	// 		prev_block = block;
	// 		temp[0] = i;
	// 		j=1;
	// 	}
	// }

	// for(bus=0; bus<ptr_ssd->nr_buses; bus++){
	// 	for(chip=0; chip<ptr_ssd->nr_chips_per_bus; chip++){
	// 		printf("bus,chip:: %d, %d\n", bus, chip);
	// 		for(block=0; block<ptr_ssd->nr_blocks_per_chip; block++){
	// 			// if(block%30==0) printf("\n");
	// 			if(max_count < ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt){
	// 				max_count = ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt;
	// 			}
	// 			// d[block] = (double)(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt);
	// 			printf("%d ", ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].nr_erase_cnt);
	// 		}
	// 		printf("\n");
	// 	}
	// 	printf("-----------------------");
	// }
	// printf("max ec: %d\n", max_count);
	// printf("standard deviation: %lf\n", sqrt(sd(d)));
	kill(getpid(), SIGINT);
}
// const int pg_no[50] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 30, 31, 32, 33, 64, 65, 66, 67, 68, 69, 128, 129, 192, 193, 194, 195, 196, 197, 512, 513, 1024, 1025, 65470, 65471, 65472, 65473, 65504, 65505, 65520, 65521, 65532, 65533, 65534, 65535, 65436, 65437};
// char pg_data[50][2048];

int32_t blueftl_user_ftl_create (struct ssd_params_t* ptr_ssd_params)
{
	/* open the character device driver for blueSSD */
	if ((_ptr_vdevice = blueftl_user_vdevice_open (ptr_ssd_params)) == NULL) {
		return -1;
	}
	int i,j;
	// for(i=0;i<50;i++){
	// 	for(j=0;j<2048;j++){
	// 		pg_data[i][j]=-1;
	// 	}
	// }

	//set unbuffered
	setvbuf(stdout, NULL, _IONBF, 0);

	act_new.sa_handler = printall; // 시그널 핸들러 지정
	sigemptyset(&act_new.sa_mask);      // 시그널 처리 중 블록될 시그널은 없음
	sigaction(SIGTSTP, &act_new, &act_old); 
	sigaction(SIGSEGV, &act_new, &act_old); 

	/* map the block mapping functions to _ftl_base */
	// _ftl_base = ftl_base_block_mapping;
	_ftl_base = ftl_base_page_mapping; //* mskim: page mapping */
	

	/* initialize the user-level FTL */
	if ((_ptr_ftl_context = _ftl_base.ftl_create_ftl_context (_ptr_vdevice)) == NULL) {
		printf ("blueftl_user_ftl_main: error occurs when creating a ftl context\n");
		blueftl_user_vdevice_close (_ptr_vdevice);
		return -1;
	}

	blueftl_read_write_mgr_init(_ptr_vdevice->page_main_size);


	printf("blueftl user create end\n");
	return 0;
}

void blueftl_user_ftl_destroy (struct virtual_device_t* _ptr_vdevice)
{
	/* close the character device driver */
	blueftl_user_vdevice_close (_ptr_vdevice);

	/* destroy the user-level FTL */
	_ftl_base.ftl_destroy_ftl_context (_ptr_ftl_context);
	
	blueftl_read_write_mgr_close();
}


/* ftl entry point */
int32_t blueftl_user_ftl_main (
	uint8_t req_dir, 
	uint32_t lba_begin, 
	uint32_t lba_end, 
	uint8_t* ptr_buffer)
{
	uint32_t lpa_begin;
	uint32_t lpa_end;
	uint32_t lpa_curr;
	int32_t ret;
	uint32_t i,j,b;


	if (_ptr_vdevice == NULL || _ptr_vdevice->blueftl_char_h == -1) {
		printf ("blueftl_user_ftl_main: the character device driver is not initialized\n");
		return -1;
	}

	lpa_begin = lba_begin / _ptr_vdevice->page_main_size;
	lpa_end = lpa_begin + (lba_end / _ptr_vdevice->page_main_size);
	
	switch (req_dir) {
		case NETLINK_READA:
		case NETLINK_READ:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				/* find a physical page address corresponding to a given lpa */
				uint8_t* ptr_lba_buff = ptr_buffer + ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
				// if(lpa_curr <2) printf("read req  %d   ",lpa_curr);
				// printf("%d ",lpa_curr);
				
				blueftl_page_read(_ftl_base, _ptr_ftl_context, lpa_curr, ptr_lba_buff);
				// for(i=0; i<50; i++){
				// 	if(pg_no[i] == lpa_curr){
				// 		b=1;
				// 		for(j=0; j<2048; j++){
				// 			if(ptr_lba_buff[j] != pg_data[i][j]){
				// 				b=0;
				// 			}
				// 		}
				// 		printf("%d %c\n", pg_no[i], b?'T':'F');
				// 	}
				// }

				// printf(":r%d: ",lpa_curr);
				// if(lpa_curr < 2 || lpa_curr == 65){
					
				// 	for(i=0; i<2048; i++){
                //         if(ptr_lba_buff[i] != 0){
                //             printf("[%4d,%3d] ", i, ptr_lba_buff[i]);
                //         } 
                //     }
                //     printf("\n");
				// }
				
			}
			break;
		case NETLINK_WRITE:
			for (lpa_curr = lpa_begin; lpa_curr < lpa_end; lpa_curr++) {
				
				uint8_t* ptr_lba_buff = ptr_buffer + ((lpa_curr - lpa_begin) * _ptr_vdevice->page_main_size);
				// if(lpa_curr <2) printf("write req  ");
				if((ret=blueftl_page_write(_ftl_base, _ptr_ftl_context, lpa_curr, ptr_lba_buff)) == -1){
					goto failed;
				}
				// if(lpa_curr < 10000) printf(":w%d: ",lpa_curr);
				// if(lpa_curr < 2 || lpa_curr == 65){
					
				// 	for(i=0; i<2048; i++){
                //         if(ptr_lba_buff[i] != 0){
                //             printf("[%4d,%3d] ", i, ptr_lba_buff[i]);
                //         } 
                //     }
                //     printf("\n");
				// }

				// for(i=0; i<50; i++){
				// 	if(pg_no[i] == lpa_curr){
				// 		memcpy(pg_data[i], ptr_lba_buff, 2048);
				// 		printf("pg_no %d is written\n", pg_no[i]);
				// 	}
				// }
			}
			break;

		default:
			printf ("bluessd: invalid I/O type (%u)\n", req_dir);
			ret = -1;
			goto failed;
	}

	ret = 0;

failed:
	// blueftl_user_vdevice_req_done (_ptr_vdevice);
	return ret;
}
