#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h> //  our new library 
#include <unistd.h>
#include <math.h>

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

struct wr_buff_t _struct_write_buff; 
struct wr_buff_t _struct_read_buff; 

uint32_t _nr_buff_pages;

int32_t get_free_pages(struct ftl_context_t* ptr_ftl_context, 
	uint32_t nr_pages,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page);

void clean_buff(){
    _struct_write_buff.arr_lpa[0] = -1;
    _struct_write_buff.arr_lpa[1] = -1;
    _struct_write_buff.arr_lpa[2] = -1;
    _struct_write_buff.arr_lpa[3] = -1;
    _nr_buff_pages = 0;
}

void blueftl_read_write_mgr_init(){   
    _struct_read_buff.arr_lpa[0] = -1;
    _struct_read_buff.arr_lpa[1] = -1;
    _struct_read_buff.arr_lpa[2] = -1;
    _struct_read_buff.arr_lpa[3] = -1;
    clean_buff();
}

void blueftl_read_write_mgr_close(){
    // free(_write_buff);
    // free(_compressed_buff);
}

void blueftl_page_read(
    struct ftl_base_t ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
){
	uint32_t bus, chip, block, page, i;
    struct ftl_page_mapping_context_t* ptr_pg_mapping=(struct ftl_page_mapping_context_t*)ptr_ftl_context->ptr_mapping;

    // dbg_print(__func__);
    // printf("lpa: %u\n", lpa_curr);
    /**
        1. read buffer/ write buffer에 있는지 확인 -> 있으면 그대로 패스 후 리턴 
        2. pgtable->chunktable에서 압축되어있는지 확인
            압축되어있으면 압축풀고 lpa보고 리턴
            압축 안되어있으면 페이지하나 읽고 리턴 
    **/
    // printf("r%5d   ", lpa_curr);
    // printf("read%5d   ", lpa_curr);
    // if(lpa_curr < 2) printf("rcv read ");
    for(i=0; i<_nr_buff_pages; i++){
        if(_struct_write_buff.arr_lpa[i] == lpa_curr){
            // write buffer의 lpa와 요청lpa가 같으면 buffer의 데이터를 꺼내서 줌
            // printf("writebuffer %u %u\n", lpa_curr, _struct_write_buff.arr_lpa[i]);
            memcpy(ptr_lba_buff,  &(_struct_write_buff.buff[i*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);
            return;
        }
    }

    for(i=0; i<WRITE_BUFFER_LEN; i++){
        if(_struct_read_buff.arr_lpa[i] == lpa_curr){
            // read buffer의 lpa와 요청lpa가 같으면 buffer의 데이터를 꺼내서 줌
            // printf("readbuffer %u %u\n", lpa_curr, _struct_read_buff.arr_lpa[i]);
            memcpy(ptr_lba_buff,  &(_struct_read_buff.buff[i*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);
            return;
        }
    }
    // printf("physical ");
    uint32_t ppa = ptr_pg_mapping->ptr_pg_table[lpa_curr];
    uint32_t chunk_index = convert_ppa_to_chunk_index(ppa);
    // printf("ppa: %u\n", ppa);
    if(ppa == -1) { 
        // printf("no ppa %u\n", lpa_curr);
        return; 
    }
    uint8_t compressed_buff[FLASH_PAGE_SIZE*WRITE_BUFFER_LEN];
    uint8_t read_buff[sizeof(struct wr_buff_t)];
    
    ftl_convert_to_ssd_layout(ppa, &bus, &chip, &block, &page);
    if(ptr_pg_mapping->ptr_chunk_table[chunk_index].is_compressed){
        // printf("comp ");
        for(i=0; i<ptr_pg_mapping->ptr_chunk_table[chunk_index].physical_page_len; i++){
            blueftl_user_vdevice_page_read (_ptr_vdevice, bus, chip, block, page+i, _ptr_vdevice->page_main_size, (char *)compressed_buff + i*FLASH_PAGE_SIZE);    
        }
        decompress(compressed_buff, FLASH_PAGE_SIZE*WRITE_BUFFER_LEN, read_buff);
        deserialize(read_buff, &_struct_read_buff);

        for(i=0; i<WRITE_BUFFER_LEN; i++){
            if(_struct_read_buff.arr_lpa[i] == lpa_curr){
                // printf("b");
                // printf("readbuffer %u %u\n", lpa_curr, _struct_read_buff.arr_lpa[i]);
                memcpy(ptr_lba_buff,  &(_struct_read_buff.buff[i*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);

                // if(lpa_curr == 0 || lpa_curr == 1 || lpa_curr==65){
                //     printf("read  %d]", lpa_curr);
                //     for(i=0; i<2048; i++){
                //         if(ptr_lba_buff[i] != 0){
                //             printf("[%4d,%3d] ", i, ptr_lba_buff[i]);
                //         }
                //     }
                //     printf("\n");
                // }
                
                return;
            }
        }
        printf("there is no physical page mapped to lpa\n");
        exit(-1);
        /**
            physical page len만큼 read -> compressed buff
            decompress -> read_buff 
            deserialize -> _struct_read_buff
        **/
    } else { // 압축안되어있음: vdevice read후 return
        // printf("nocomp %u\n", lpa_curr);
        blueftl_user_vdevice_page_read (_ptr_vdevice, bus, chip, block, page, _ptr_vdevice->page_main_size, (char*)ptr_lba_buff);
        // if(lpa_curr == 0 || lpa_curr == 1 || lpa_curr==65){
        //     printf("read  %d]", lpa_curr);
        //     for(i=0; i<2048; i++){
        //         if(ptr_lba_buff[i] != 0){
        //             printf("[%4d,%3d] ", i, ptr_lba_buff[i]);
        //         }
        //     }
        //     printf("\n");
        // }
    }

    return;
} 
    
/** 
    1. mapping table에서 lpa_curr에 해당하는 ppa를 찾음
    2. chunk table에서 ppa에 해당하는 엔트리에서,
        압축이 되어있으면 압축 풀고 구조체에서 lpa보고 memcpy
        압축이 안되어있으면 single page 하나이므로 vdevice read 
**/

uint32_t blueftl_page_write(
    struct ftl_base_t ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
){
    uint32_t i, bus, chip, block, page;
    uint8_t write_buff[sizeof(struct wr_buff_t)];
    uint8_t compressed_buff[WRITE_BUFFER_LEN*FLASH_PAGE_SIZE];
    /*
        (1) write buff count 증가
        (2) lpa 저장
        (3) write 대상 데이터를 버퍼에 복사
    */
    // printf("write %u .", lpa_curr);

    
    /*
        write 하는 lpa가 이미 write buffer에 존재할때?
        1. 한 lpa에 대응하는 ppa의 valid count가 두번 감소됨 
        2. read할때 writebuffer에서 긁어오면 최신데이터를 못 줄 수도 있음      
            (writebuffer에서 0 1 2 3 순서로 쓰고 0 1 2 3 순서로 읽을때, 3210이면 상관없는듯)
        따라서 writebuffer에 그냥 overwrite하자
    */
    for(i=0; i<_nr_buff_pages; i++){
        if(_struct_write_buff.arr_lpa[i] == lpa_curr){
            memcpy(&(_struct_write_buff.buff[i*FLASH_PAGE_SIZE]), ptr_lba_buff, FLASH_PAGE_SIZE);
            // printf(" overwrite to write buffer _nr_buff_pages %d %d==%d\n", i, _struct_write_buff.arr_lpa[i], lpa_curr);
            return;
        }
    }

    _nr_buff_pages++; // (1)
    _struct_write_buff.arr_lpa[_nr_buff_pages-1] = lpa_curr; // (3)
    memcpy(&(_struct_write_buff.buff[(_nr_buff_pages-1)*FLASH_PAGE_SIZE]), ptr_lba_buff, FLASH_PAGE_SIZE); // (5)
    // printf(" write buffer %d %d\n", _nr_buff_pages, lpa_curr);
    // printf("w%5d  ",lpa_curr);
    // if(lpa_curr == 65 || lpa_curr<2){
    //     printf("write  %d %u]", lpa_curr, timer_get_timestamp_in_us());
    //     for(i=0; i<2048; i++){
    //         if(ptr_lba_buff[i] != 0){
    //             printf("[%4d,%3d] ", i, ptr_lba_buff[i]);
    //         }
    //     }
    //     printf("\n");
    // }
    
    
    /* buff가 꽉찼으므로 compression 후 write */
    if(_nr_buff_pages == WRITE_BUFFER_LEN){
        // printf("flush\n");
        // dbg_print("_nr_buff_pages == WRITE_BUFFER_LEN");
        /*
            압축이 필요한가 - 압축이 필요할 때
                1. 압축
                2. 압축된 페이지 사이즈에 맞게 ftl 페이지 요청
                3. physical write
                4. ftl 등록
            압축이 필요한가 - 압축이 필요하지 않을 때
                각 4개의 페이지에 대해
                    1. ftl 페이지요청
                    2. physical write
                    3. ftl 등록 
            170419: 압축 write를 먼저 구현하자. 압축필요여부는 나중에.
            
            QUESTION
            Write를 했다 -> buffer에 들어가 있음
            buffer에서 꺼내 압축 하여 physical write를 하기 전에 read요청이 들어오면 어떻게 할것인가.
            =>  buffer에 있는 데이터를 그냥 꺼내라

            문제점 page_mapping_get_free_physical_page_address
                현재 block 내에서 연속된 페이지를 찾는데 block fragmentation이 생김. 
                그런 빈 자리를 싱글 페이지가 들어가도록 유도해야 할듯.!!
                
        */
        
        /* 1. serialize 후 압축 */
        serialize(&_struct_write_buff, write_buff);
        UWORD compressed_size = compress(write_buff, sizeof(struct wr_buff_t), compressed_buff);
        // printf("compressd_size %d\n", compressed_size);
        if( compressed_size >= FLASH_PAGE_SIZE * WRITE_BUFFER_LEN // 압축했는데 4페이지보다 크거나
            || compressed_size == -1 // 압축의 리턴값이 -1(실패)일 때 압축하지 않고 physical write
        ){  
            printf("no compressed\n");
            /* 압축 하지 않고 write */
            for(i=0; i<_nr_buff_pages; i++){
                /* 2. 페이지 요청 */
                if(get_free_pages(ptr_ftl_context, 1, &bus, &chip, &block, &page)==-1){
                    goto err;
                }
                /* 3. physical write */
                blueftl_user_vdevice_page_write(
                    _ptr_vdevice,
                    bus, chip, block, page,
                    FLASH_PAGE_SIZE,
                    (char*)&(_struct_write_buff.buff[FLASH_PAGE_SIZE*i])
                );
                /* 4. ftl 등록 */
                if(ftl_base.ftl_map_logical_to_physical(
                    ptr_ftl_context, &(_struct_write_buff.arr_lpa[i]), bus, chip, block, page, 1, 1, 0) == -1)
                {
                    printf ("bluessd: map_logical_to_physical failed\n");
                    goto err;
                }
                // printf("%5u -> %5u\n", _struct_write_buff.arr_lpa[i], ftl_convert_to_physical_page_address(bus, chip, block, page));
            }

        } else { /* 압축된 것을 write */

            // 압축된 페이지 계산
            uint32_t nr_compress_pages = compressed_size/FLASH_PAGE_SIZE + compressed_size%FLASH_PAGE_SIZE?1:0;
            uint32_t bus, chip, block, page;
            /* 2. 압축된 페이지 사이즈에 맞게 ftl 페이지 요청 */
            if(get_free_pages(ptr_ftl_context, nr_compress_pages, &bus, &chip, &block, &page)==-1){
                goto err;
            }

            /* 3. physical write */
            uint32_t *ptr_compress_page;
            for(i=0; i<nr_compress_pages; i++){
                ptr_compress_page = compressed_buff + i*FLASH_PAGE_SIZE;
                blueftl_user_vdevice_page_write(
                    _ptr_vdevice,
                    bus, chip, block, page+i,
                    FLASH_PAGE_SIZE,
                    (char *)ptr_compress_page
                );
                
            }

            /* 4. ftl 등록 */
            if(ftl_base.ftl_map_logical_to_physical(
                ptr_ftl_context, _struct_write_buff.arr_lpa, bus, chip, block, page, WRITE_BUFFER_LEN, nr_compress_pages, 1) == -1)
            {
                printf ("bluessd: map_logical_to_physical failed\n");
                goto err;
            }


            // for(i=0; i<WRITE_BUFFER_LEN; i++){
            //     printf("%5u -> %4u, %2u\n", _struct_write_buff.arr_lpa[i], block, page);
            // }
        }
        
        // printf("%u\n", ftl_convert_to_physical_page_address(bus, chip, block, page));
        clean_buff();
        
    }

    return 0;

err:
    printf("err\n");
    return -1;
}

int32_t get_free_pages(struct ftl_context_t* ptr_ftl_context, 
	uint32_t nr_pages,
	uint32_t *ptr_bus,
	uint32_t *ptr_chip,
	uint32_t *ptr_block,
	uint32_t *ptr_page)
{
    if(_ftl_base.ftl_get_free_physical_page_address(
        ptr_ftl_context, nr_pages, ptr_bus, ptr_chip, ptr_block, ptr_page) == -1
    ){
        /* 구현:: 공간 없음. GC 요청, page_mapping_get_free_physical_page_address 재시도, 안되면 에러*/
        if (_ftl_base.ftl_trigger_gc != NULL) {
            
            /* GC */
            if(_ftl_base.ftl_trigger_gc(ptr_ftl_context, *ptr_bus, *ptr_chip) == -1) {
                printf("bluessd: oops! garbage collection failed.\n");
                goto err;
            }

            /* 페이지요청 재시도 */
            if(_ftl_base.ftl_get_free_physical_page_address(
                ptr_ftl_context, nr_pages, ptr_bus, ptr_chip, ptr_block, ptr_page) == -1
            ){
                printf("bluessd: there is not sufficient space in flash memory.\n");
                goto err;
            }
        } else {
            printf ("bluessd: garbage collection is not registered\n");
            goto err;
        }
    }
    return 0;

err:
    return -1;
}


void serialize(struct wr_buff_t *src, uint8_t *dest){
    uint8_t *dest_pos = dest;
    
    memcpy(dest_pos, &(src->arr_lpa[0]), sizeof(uint32_t));
    dest_pos += sizeof(uint32_t);
    memcpy(dest_pos, &(src->arr_lpa[1]), sizeof(uint32_t));
    dest_pos += sizeof(uint32_t);
    memcpy(dest_pos, &(src->arr_lpa[2]), sizeof(uint32_t));
    dest_pos += sizeof(uint32_t);
    memcpy(dest_pos, &(src->arr_lpa[3]), sizeof(uint32_t));
    dest_pos += sizeof(uint32_t);

    memcpy(dest_pos, &(src->buff[0*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);
    dest_pos += FLASH_PAGE_SIZE;
    memcpy(dest_pos, &(src->buff[1*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);
    dest_pos += FLASH_PAGE_SIZE;
    memcpy(dest_pos, &(src->buff[2*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);
    dest_pos += FLASH_PAGE_SIZE;
    memcpy(dest_pos, &(src->buff[3*FLASH_PAGE_SIZE]), FLASH_PAGE_SIZE);

}

void deserialize(uint8_t *src, struct wr_buff_t *dest){
    uint8_t *src_pos = src;
    
    memcpy(&(dest->arr_lpa[0]), src_pos, sizeof(uint32_t));
    src_pos += sizeof(uint32_t);
    memcpy(&(dest->arr_lpa[1]), src_pos, sizeof(uint32_t));
    src_pos += sizeof(uint32_t);
    memcpy(&(dest->arr_lpa[2]), src_pos, sizeof(uint32_t));
    src_pos += sizeof(uint32_t);
    memcpy(&(dest->arr_lpa[3]), src_pos, sizeof(uint32_t));
    src_pos += sizeof(uint32_t);

    memcpy(&(dest->buff[0*FLASH_PAGE_SIZE]), src_pos, FLASH_PAGE_SIZE);
    src_pos += FLASH_PAGE_SIZE;
    memcpy(&(dest->buff[1*FLASH_PAGE_SIZE]), src_pos, FLASH_PAGE_SIZE);
    src_pos += FLASH_PAGE_SIZE;
    memcpy(&(dest->buff[2*FLASH_PAGE_SIZE]), src_pos, FLASH_PAGE_SIZE);
    src_pos += FLASH_PAGE_SIZE;
    memcpy(&(dest->buff[3*FLASH_PAGE_SIZE]), src_pos, FLASH_PAGE_SIZE);

}
