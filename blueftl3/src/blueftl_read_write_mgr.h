#ifndef _BLUESSD_FTL_RW_MGR
#define _BLUESSD_FTL_RW_MGR

#define WRITE_BUFFER_LEN 4

struct wr_buff_t {
    uint32_t arr_lpa[WRITE_BUFFER_LEN];
    uint8_t buff[FLASH_PAGE_SIZE * WRITE_BUFFER_LEN];
};

void blueftl_read_write_mgr_init();
void blueftl_read_write_mgr_close();


void serialize(struct wr_buff_t *src, uint8_t *dest);
void deserialize(uint8_t *src, struct wr_buff_t *dest);

void blueftl_page_read(
    struct ftl_base_t ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
);

uint32_t blueftl_page_write(
    struct ftl_base_t ftl_base, 
    struct ftl_context_t *ptr_ftl_context, 
    uint32_t lpa_curr, 
    uint8_t *ptr_lba_buff
);

#endif
