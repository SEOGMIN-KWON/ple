#ifdef KERNEL_MODE

#include <time.h>
#include <limits.h>
#include <linux/vmalloc.h>
#include <stdlib.h>

#else

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#endif

#include "blueftl_gc_page.h"
#include "blueftl_ftl_base.h"
#include "blueftl_mapping_page.h"
#include "blueftl_util.h"
#include "blueftl_user_vdevice.h"
#include "blueftl_wl_dual_pool.h"
#include "blueftl_read_write_mgr.h"
#include "lzrw3.h"

// uint32_t get_block_invalid_pages(struct ftl_context_t *ptr_ftl_context, uint32_t block_no){
// 	struct ftl_page_mapping_context_t *ptr_pg_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
// 	struct chunk_table_t *ptr_chunk_table = ptr_pg_mapping->ptr_chunk_table;
	
// 	uint32_t ppa, page = 0;
// 	uint32_t invalid_count = 0;
// 	uint32_t chunk_index;
// 	while(page < ptr_ftl_context->ptr_ssd->nr_pages_per_block){
// 		ppa = ftl_convert_to_physical_page_address(0, 0, block_no, page);
// 		chunk_index = convert_ppa_to_chunk_index(ppa);
// 		invalid_count += WRITE_BUFFER_LEN - ptr_chunk_table[chunk_index].valid_count;
// 		page += ptr_chunk_table[chunk_index].physical_page_len;
// 	}

// 	return invalid_count;
// }

uint32_t get_block_invalid_page(struct ftl_context_t *ptr_ftl_context, uint32_t no_block){
	struct ftl_page_mapping_context_t *ptr_pg_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct chunk_table_t *ptr_chunk_table = ptr_pg_mapping->ptr_chunk_table;
	uint32_t ppa, chunk_index, i = 0;
	uint32_t invalid_pages = 0;

	while(i < ptr_ftl_context->ptr_ssd->nr_pages_per_block){
		ppa = ftl_convert_to_physical_page_address(0, 0, no_block, i);
		chunk_index = convert_ppa_to_chunk_index(ppa);
		if(ptr_chunk_table[chunk_index].is_compressed){
			invalid_pages += 4 - ptr_chunk_table[chunk_index].valid_count;
			i += ptr_chunk_table[chunk_index].physical_page_len;
		} else {
			invalid_pages += 1 - ptr_chunk_table[chunk_index].valid_count;
			i += 1;
		}
	}
	return invalid_pages;
}

struct flash_block_t *select_victim(struct ftl_context_t *ptr_ftl_context, uint32_t bus, uint32_t chip){
	struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
	uint32_t block;
	uint32_t no_victim_block = -1;
	uint32_t victim_block_invalid_pages;
	uint32_t temp_block_invalid_pages;
	//invalid 가 maximum 인 block 
	// printf("select victim start");
	for(block=0; block < ptr_ssd->nr_blocks_per_chip; block++){
		if(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[block].is_reserved_block) continue;

		temp_block_invalid_pages = get_block_invalid_page(ptr_ftl_context, block);

		if((temp_block_invalid_pages>0 && no_victim_block==-1) || victim_block_invalid_pages < temp_block_invalid_pages){
			// printf(" changed %d to %d(%d)\n", no_victim_block, block, temp_block_invalid_pages);
			no_victim_block = block;
			victim_block_invalid_pages = temp_block_invalid_pages;
		}
	}
	if(no_victim_block == -1){
		printf("no victim block");
		return -1;
	}
	// printf("victim %d invalid %d\n", no_victim_block, victim_block_invalid_pages);
	return &(ptr_ssd->list_buses[bus].list_chips[chip].list_blocks[no_victim_block]);
}

struct wr_buff_t _gc_struct_write_buff;
uint32_t _gc_nr_buff_pages;
#define NEED_BUFFER_FLUSH -2

uint32_t gc_buffered_write(struct flash_block_t *src, uint32_t lpa, uint8_t *data){
	_gc_nr_buff_pages++;
	_gc_struct_write_buff.arr_lpa[_gc_nr_buff_pages-1] = lpa;
	memcpy(&(_gc_struct_write_buff.buff[(_gc_nr_buff_pages-1)*FLASH_PAGE_SIZE]), data, FLASH_PAGE_SIZE);
	
	if(_gc_nr_buff_pages == 4){
		return NEED_BUFFER_FLUSH;
	}
	return 0;
}

void gc_buffer_flush(struct ftl_context_t *context, struct flash_block_t *dest, uint32_t *dest_pg){
	if(_gc_nr_buff_pages == 0){
		goto ret;
	}
	
	uint8_t *serialized_write_buff = (uint8_t *)malloc(sizeof(struct wr_buff_t));
	uint8_t *compressed_buff = (uint8_t *)malloc(sizeof(struct wr_buff_t));
	serialize(&_gc_struct_write_buff, serialized_write_buff);
	uint32_t compressed_len = compress(serialized_write_buff, sizeof(struct wr_buff_t), compressed_buff);
	uint32_t i,ppa,chunk_index;
	struct ftl_page_mapping_context_t *ptr_pg_mapping = (struct ftl_page_mapping_context_t *)context->ptr_mapping;

	if(	_gc_nr_buff_pages == 4
		&&( 0 <= compressed_len && compressed_len < FLASH_PAGE_SIZE * WRITE_BUFFER_LEN )
	){ //압축 write
		/* 압축된 페이지를 write + 청크테이블, 페이지정보, 블록정보 업데이트 */
		uint32_t nr_compress_pages = compressed_len/FLASH_PAGE_SIZE + compressed_len%FLASH_PAGE_SIZE?1:0;
		uint8_t *ptr_compress_page;
		ppa = ftl_convert_to_physical_page_address(dest->no_bus, dest->no_chip, dest->no_block, *dest_pg);
		
		/* 페이지테이블 정보 업데이트 */
		for(i=0; i<WRITE_BUFFER_LEN; i++){
			ptr_pg_mapping->ptr_pg_table[_gc_struct_write_buff.arr_lpa[i]] = ppa;
		}

		for(i=0; i<nr_compress_pages; i++){

			// 페이지 write
			ptr_compress_page = &(compressed_buff[i*FLASH_PAGE_SIZE]);
			blueftl_user_vdevice_page_write(
				context->ptr_vdevice, dest->no_bus, dest->no_chip, dest->no_block, *dest_pg + i,
				FLASH_PAGE_SIZE, (char *)ptr_compress_page
			);

			ppa = ftl_convert_to_physical_page_address(dest->no_bus, dest->no_chip, dest->no_block, *dest_pg + i);
			chunk_index = convert_ppa_to_chunk_index(ppa);
			// 청크테이블 업데이트
			ptr_pg_mapping->ptr_chunk_table[chunk_index].valid_count = WRITE_BUFFER_LEN;
			ptr_pg_mapping->ptr_chunk_table[chunk_index].physical_page_len = nr_compress_pages;
			ptr_pg_mapping->ptr_chunk_table[chunk_index].is_compressed = 1;
			

			// 페이지 정보 업데이트
			dest->list_pages[*dest_pg + i].no_logical_page_addr = _gc_struct_write_buff.arr_lpa[0]; //1st lpa
			dest->list_pages[*dest_pg + i].page_status = PAGE_STATUS_VALID;

			// 블록 정보 업데이트
			dest->nr_valid_pages++;
			dest->nr_free_pages--;
		}

		// dest pg 증가
		*dest_pg += nr_compress_pages;
		
		_gc_nr_buff_pages = 0;
	} else {
		// 각 페이지에 대해 write
		for(i=0; i<_gc_nr_buff_pages; i++){
			
			// 페이지 write
			blueftl_user_vdevice_page_write(
				context->ptr_vdevice, dest->no_bus, dest->no_chip, dest->no_block, *dest_pg,
				FLASH_PAGE_SIZE, (char *)&(_gc_struct_write_buff.buff[FLASH_PAGE_SIZE*i])
			);
			
			ppa = ftl_convert_to_physical_page_address(dest->no_bus, dest->no_chip, dest->no_block, *dest_pg);
			chunk_index = convert_ppa_to_chunk_index(ppa);
			// 청크테이블 업데이트
			ptr_pg_mapping->ptr_chunk_table[chunk_index].is_compressed = 0;
			ptr_pg_mapping->ptr_chunk_table[chunk_index].valid_count = 1;
			ptr_pg_mapping->ptr_chunk_table[chunk_index].physical_page_len = 1;

			// 페이지 테이블 업데이트
			ptr_pg_mapping->ptr_pg_table[_gc_struct_write_buff.arr_lpa[i]] = ppa;

			// 페이지 정보 업데이트
			dest->list_pages[*dest_pg].page_status = PAGE_STATUS_VALID;
			dest->list_pages[*dest_pg].no_logical_page_addr = _gc_struct_write_buff.arr_lpa[i];

			// 블록 정보 업데이트 
			dest->nr_free_pages--;
			dest->nr_valid_pages++;

			// dest pg 증가
			(*dest_pg)++;
		}
		_gc_nr_buff_pages = 0;
	}

	free(serialized_write_buff);
	free(compressed_buff);
	return;
ret:
	return;
}

void move_block(struct ftl_context_t *context, struct flash_block_t *src, struct flash_block_t *dest){
	struct flash_ssd_t *ptr_ssd = context->ptr_ssd;
	char *compressed_buff = (char *)malloc(context->ptr_vdevice->page_main_size);
	struct ftl_page_mapping_context_t* ptr_pg_mapping = (struct ftl_page_mapping_context_t*)context->ptr_mapping;
	struct chunk_table_t *ptr_chunk_table = ptr_pg_mapping->ptr_chunk_table;
	uint32_t src_pg, dest_pg;
	uint32_t src_ppa, temp_physical_len, src_chunk_index;
	uint32_t i;
	// printf("start %s", __func__);
	/**
		1. src 블록의 시작 ppa를 가져옴
		2. chunk table 에서 시작 ppa ~ 끝 ppa를 돌면서 압축된 페이지인지 확인
			압축이 되어있을 때
				valid count == 4 일때 page to page copy
				valud count < 4 일때
					1. 압축된 모든 페이지 read
					2. decompress
					3. invalid page 제거 -> 무엇이 invalid인지 확인:: lpa를보고 mapping table에서 현재 ppa와 일치하는지 보면됨
					4. 버퍼에 잠시 담아두고 4개가 채워지면 write
					5. 끝 ppa까지 돌았는데 남아있는 page가 있으면 compress안하고 write
			압축이 안되어있을 때
				valid page 이면 page to page copy

		***************************** FREEPAGE인지 확인해 *****************************
	**/

	uint8_t *c_buff = (uint8_t *)malloc(FLASH_PAGE_SIZE*WRITE_BUFFER_LEN);
	uint8_t *buff1 = (uint8_t *)malloc(FLASH_PAGE_SIZE*WRITE_BUFFER_LEN);
	uint8_t *buff2 = (uint8_t *)malloc(FLASH_PAGE_SIZE);
	src_pg = 0;
	dest_pg = 0;
	while(src_pg < ptr_ssd->nr_pages_per_block){
		src_ppa = ftl_convert_to_physical_page_address(src->no_bus, src->no_chip, src->no_block, src_pg);
		src_chunk_index = convert_ppa_to_chunk_index(src_ppa);
		temp_physical_len = ptr_chunk_table[src_chunk_index].physical_page_len;

		// printf("src_pg %d ", src_pg);
		// valid page가 있으면 copy대상
		if(ptr_chunk_table[src_chunk_index].valid_count>0){
			// printf("validcount>0 ");
			// 압축된 청크
			if(ptr_chunk_table[src_chunk_index].is_compressed){
				// printf("compressed ");
				/** 
					압축해제,deserial
					invalid인지 확인.
					valid한것을 buff로 옮김.
					valid한것 4개를 모아서 한꺼번에 압축후 write
				**/
				struct wr_buff_t struct_wr_buff;
				uint32_t ppa = ftl_convert_to_physical_page_address(src->no_bus, src->no_chip, src->no_block, src_pg);
				// printf("ppl%d", ptr_chunk_table[src_chunk_index].physical_page_len);
				for(i=0; i<ptr_chunk_table[src_chunk_index].physical_page_len; i++){ // plen
					blueftl_user_vdevice_page_read(
						context->ptr_vdevice, src->no_bus, src->no_chip, src->no_block, 
						src_pg+i, context->ptr_vdevice->page_main_size, (char*)c_buff+(i*FLASH_PAGE_SIZE)
					);
				}
				// printf("read ");
				decompress(c_buff, WRITE_BUFFER_LEN*FLASH_PAGE_SIZE, buff1); // decompress size ???????. 
				deserialize(buff1, &struct_wr_buff);
				
				// 현재 src_pg의 ppa와, 압축을 푼 데이터 lpa들의 mapping table에 존재하는 ppa가 같으면 T, 다르면 F
				for(i=0; i<WRITE_BUFFER_LEN; i++){
					if(ppa == ptr_pg_mapping->ptr_pg_table[struct_wr_buff.arr_lpa[i]]){
						// memcpy(buff+(valid_pg*FLASH_PAGE_SIZE), &struct_wr_buff.buff[i*FLASH_PAGE_SIZE], FLASH_PAGE_SIZE);
						if(gc_buffered_write(src, struct_wr_buff.arr_lpa[i], &(struct_wr_buff.buff[i*FLASH_PAGE_SIZE]))
							== NEED_BUFFER_FLUSH
						){
							gc_buffer_flush(context, dest, &dest_pg);
						}
					}
				}
				// printf("flush ");
			} else { // 압축되지않은 페이지. page to page copy -->가 아니고 bufferd write 
				// printf("!compressed ");
				src_ppa = ftl_convert_to_physical_page_address(src->no_bus, src->no_chip, src->no_block, src_pg);
				src_chunk_index = convert_ppa_to_chunk_index(src_ppa);
				blueftl_user_vdevice_page_read(
					context->ptr_vdevice, src->no_bus, src->no_chip, src->no_block, 
					src_pg, context->ptr_vdevice->page_main_size, buff2
				);
				// printf("read ");
				if(gc_buffered_write(src, src->list_pages[src_pg].no_logical_page_addr, (uint8_t *)buff2) 
					== NEED_BUFFER_FLUSH
				){
					gc_buffer_flush(context, dest, &dest_pg);
				}
				// printf("flush ");
			}
			// printf("\n");
		}
		src_pg += ptr_chunk_table[src_chunk_index].physical_page_len;
	}

	for(src_pg=0; src_pg< ptr_ssd->nr_pages_per_block; src_pg++){
		src_ppa = ftl_convert_to_physical_page_address(src->no_bus, src->no_chip, src->no_block, src_pg);
		src_chunk_index = convert_ppa_to_chunk_index(src_ppa);
		ptr_chunk_table[src_chunk_index].valid_count=0;
		ptr_chunk_table[src_chunk_index].physical_page_len=0;
		ptr_chunk_table[src_chunk_index].is_compressed=0;
	}

	free(compressed_buff);
	free(c_buff);
	free(buff1);
	free(buff2);
	gc_buffer_flush(context, dest, &dest_pg);
	// printf("  end %s\n", __func__);
}

uint32_t num_physical_page_inblock(struct ftl_context_t *ptr_ftl_context, struct flash_block_t *block){
	struct ftl_page_mapping_context_t *ptr_pg_mapping = (struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping;
	struct chunk_table_t *ptr_chunk_table = ptr_pg_mapping->ptr_chunk_table;
	uint32_t pg=0, chunk_index, pl=0;

	while(pg<64){
		chunk_index = convert_layout_to_chunk_index(block->no_block,pg);
		pl += ptr_chunk_table[chunk_index].valid_count;
		pg += ptr_chunk_table[chunk_index].is_compressed?ptr_chunk_table[chunk_index].physical_page_len : 1;
		// pl+=block->list_pages[pg].page_status==PAGE_STATUS_VALID?1:0;
		// pg++;
	}
	return pl;
}

void gc_page_trigger_init(struct ftl_context_t *ptr_ftl_context){
	struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
	// struct flash_block_t *reserved = ((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved;

	if(ptr_ssd->list_buses[0].list_chips[0].nr_free_blocks>0){
		((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved = ssdmgmt_get_free_block(ptr_ssd, 0, 0);
		ptr_ssd->list_buses[0].list_chips[0].nr_free_blocks--;
		ptr_ssd->list_buses[0].list_chips[0].nr_dirty_blocks++;
	}
	((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved->is_reserved_block=1;
	//printf("init reserved %p\n", reserved);
}


int32_t gc_page_trigger_gc(struct ftl_context_t *ptr_ftl_context, int32_t bus, int32_t chip)
{
	struct flash_ssd_t *ptr_ssd = ptr_ftl_context->ptr_ssd;
	struct virtual_device_t *pvdevice = ptr_ftl_context->ptr_vdevice;
	uint32_t page_offset;
	bus=0;chip=0;
	struct flash_block_t *victim = select_victim(ptr_ftl_context, bus, chip);

	struct flash_block_t *temp;
	struct flash_block_t *reserved = ((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved;
	
	_gc_nr_buff_pages = 0;

	// printf("before %d %d %d\n", victim->no_block, num_physical_page_inblock(ptr_ftl_context, victim), get_block_invalid_page(ptr_ftl_context, victim->no_block));
	move_block(ptr_ftl_context, victim, reserved);
	// printf("after  %d %d %d\n", reserved->no_block, num_physical_page_inblock(ptr_ftl_context, reserved), get_block_invalid_page(ptr_ftl_context, reserved->no_block));

	/* erase victim block */
	blueftl_user_vdevice_block_erase(pvdevice, victim->no_bus, victim->no_chip, victim->no_block);
	for (page_offset = 0; page_offset < ptr_ssd->nr_pages_per_block; page_offset++) {
		victim->list_pages[page_offset].page_status = PAGE_STATUS_FREE;
	}
	// ptr_ssd->list_buses[victim->no_bus].list_chips[victim->no_chip].nr_free_blocks++;
	// ptr_ssd->list_buses[reserved->no_bus].list_chips[reserved->no_chip].nr_dirty_blocks--;
	
	
	reserved->is_reserved_block = 0;
	
	//swap
	temp = reserved;
	((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved = victim;
	reserved = ((struct ftl_page_mapping_context_t *)ptr_ftl_context->ptr_mapping)->reserved;
	victim = temp;
	
	reserved->is_reserved_block = 1; // victim이 reserved가 됨 (비어있는), 따라서 reserved에 valid가 

	reserved->nr_erase_cnt++;
	reserved->nr_recent_erase_cnt++;

	reserved->nr_valid_pages = 0;
	reserved->nr_invalid_pages = 0;
	reserved->nr_free_pages = ptr_ssd->nr_pages_per_block;
	reserved->last_modified_time = timer_get_timestamp_in_us();

#if WL_ON
	update_max_min_nr_erase_cnt_in_pool(ptr_ftl_context);
	cold_data_migration(ptr_ftl_context);
	cold_pool_adjustment(ptr_ftl_context);
	hot_pool_adjustment(ptr_ftl_context);
#endif
	
	return 0;
}
