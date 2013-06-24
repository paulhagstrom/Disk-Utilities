/*
 * disk/appleii_16.c
 * 
 * 16-sector Apple II format.
 * 
 * Written in 2013 by balr0g
 */

#include <libdisk/util.h>
#include "../private.h"

struct appleII_extra_data {
    uint32_t address_mark;
    uint32_t data_mark;
    uint32_t data_raw_length;
    uint32_t postamble;
    int (*decode_bytes)(
    uint8_t *in, uint8_t *out, int size, int sec_size);
};

struct appleII_address_field {
    uint32_t address_mark;
    uint8_t volume,
            track,
            sector,
            checksum;
    uint32_t postamble;
};

const uint8_t gcr6bw_tb[0x100] =
{
    // 0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x03, 0x00, 0x04, 0x05, 0x06,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x08, 0x00, 0x00, 0xFE, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x00, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x1c, 0x1d, 0x1e,
    0x00, 0x00, 0x00, 0x1f, 0x00, 0xFF, 0x20, 0x21, 0x00, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x2a, 0x2b, 0x00, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32,
    0x00, 0x00, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x00, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
};


uint8_t appleII_gcr4_decode(uint8_t e0, uint8_t e1)
{
    return ((e0 << 1) & 0xaa) | (e1 & 0x55);
}

int appleII_16sector_decode_bytes(uint8_t *in, uint8_t *out, int size, int sec_size)
{
    // bail out if this isn't a 16-sector A2 sector
    if(size != 342 && sec_size != 256) {
        errx(1, "Invalid sector being decoded -- wrong sizes!");
    }
    
    uint8_t buf[size];
    uint8_t c = 0;
    // convert the data into 6-byte GCR and calculate the checksum
    for(int i = 0; i < size; i++) {
        buf[i] = gcr6bw_tb[in[i]] ^ c;
        c = buf[i];
   //     printf("%02x", in[i]);
    }
    
    memset(out, 0, sec_size);
    
    // buffer broken into the following sections:
    // bytes 0-86:
    //   "2" section: bits: xx 01c 01b 01a
    //   a is for first 86 bytes, b is for next 86, c is for last 84
    // bytes 87-173:
    // remaining "6" sections, bits 00 76 54 32
    
    
    // first block - decode "2" section of "6-and-2" -- first 86 bytes of 342-byte buffer
    // first block
    for(int i=0; i<86; i++) {
        // set a
        out[i] |= ((buf[i]&0x2) == 0x2);
        out[i] |= ((buf[i]&0x1) == 0x1) << 1;
        // set b
        out[i+86] |= ((buf[i]&0x8) == 0x8);
        out[i+86] |= ((buf[i]&0x4) == 0x4) << 1;
        // set c (warning, only 84 bytes!)
        if(i<84) {
            out[i+86+86] |= ((buf[i]&0x20) == 0x20);
            out[i+86+86] |= ((buf[i]&0x10) == 0x10) << 1;
        }
    }
   
    // remaining blocks - decode "6" sections
    for(int i=86; i<86+86+86+84; i++) {
        out[i-86] |= buf[i] << 2;
      //  printf("%c ", out[i-86]);
    }


  //  printf("\n");
    return c;
}

int appleII_13sector_decode_bytes(uint8_t *in, uint8_t *out, int size, int sec_size)
{
    return 0;
}

int16_t appleII_get_nybble(struct stream *s, unsigned int max_scan)
{
    while((stream_next_bit(s) != -1) && --max_scan) {
        if((s->word & 0x80) == 0x80) { // valid nybble
            int16_t val = (s->word) & 0xFF;
            stream_next_bits(s,7);
            return val;
        }

    }
    return -1; // didn't find any!
}


int appleII_read_block(struct stream *s, uint8_t *buf, unsigned int length)
{
    int i;
    int16_t tempnybble;
    for (i = 0; i < length; i++) {
        tempnybble = appleII_get_nybble(s, 0);
        if (tempnybble == -1) return -1; // bail out of we ran out of bits
        buf[i] = tempnybble;
    }
    return 0;
}

/**
 @return 0 if found, -1 if no mark found before end of stream
**/
int appleII_scan_mark(struct stream *s, uint32_t mark, uint32_t mask, unsigned int max_scan)
{
    int16_t tempnybble; // needs to be signed since get_nybble can return -1 if the stream ran out
    uint32_t lastfour = 0; // last four nybbles read
    while (1) {
        tempnybble = appleII_get_nybble(s, max_scan); // grab one nybble
        if (tempnybble == -1) return -1; // if end of stream, return that
        // otherwise stash the nybble we just got
        lastfour <<= 8;
        lastfour |= (tempnybble&0xFF);
        if ((lastfour&mask)==(mark&mask)) { // found mark
            printf("%08X ", lastfour);
            // describe what was found, for the viewers at home
            switch(lastfour&mask) {
                case 0x00D5AA96:
                    printf("Address mark header (16sector)\n");
                    break;
                case 0x00D5AAAB:
                    printf("Address mark header (13sector)\n");
                    break;
                case 0x00D5AAAD:
                    printf("Data mark header\n");
                    break;
                case 0x00DEAA00:
                    printf("Postamble\n");
                    break;
                default:
                    printf("Unknown mark\n");
                    break;
            }
        return 0;
        }
    }
}

int appleII_scan_address_field(struct stream *s, uint32_t addrmark, struct appleII_address_field *address_field)
{
    int mark_status = appleII_scan_mark(s, addrmark, 0x00FFFFFF, ~0u);
    int i;
    int16_t tempnybble; // needs to be signed since get_nybble can return -1 if the stream ran out
    uint32_t lastfour; // last four nybbles read
    if(mark_status == -1) return -2; // bail out if we ran out of bits before even starting, and return -2 instead of -1
    address_field->address_mark = addrmark; // we know this is correct, otherwise we'd have failed
    /* volume, track */
    lastfour = 0;
    for (i = 0; i < 4; i++) {
        lastfour <<=8;
        tempnybble = appleII_get_nybble(s, 0);
        if (tempnybble == -1) return -1; // bail out of we ran out of bits
        lastfour |= tempnybble&0xFF;
    }
    address_field->volume = appleII_gcr4_decode((lastfour>>24)&0xFF, (lastfour>>16)&0xFF);
    address_field->track = appleII_gcr4_decode((lastfour>>8)&0xFF, lastfour&0xFF);

    /* sector, checksum */
    lastfour = 0;
    for (i = 0; i < 4; i++) {
        lastfour <<=8;
        tempnybble = appleII_get_nybble(s, 0);
        if (tempnybble == -1) return -1; // bail out of we ran out of bits
        lastfour |= tempnybble&0xFF;
    }
    address_field->sector = appleII_gcr4_decode((lastfour>>24)&0xFF, (lastfour>>16)&0xFF);
    address_field->checksum = appleII_gcr4_decode((lastfour>>8)&0xFF, lastfour&0xFF);

    /* postamble */
    lastfour = 0;
    for (i = 0; i < 3; i++) {
        lastfour <<=8;
        tempnybble = appleII_get_nybble(s, 0);
        if (tempnybble == -1) return -1; // bail out of we ran out of bits
        lastfour |= tempnybble&0xFF;
    }
    address_field->postamble = lastfour;

    return mark_status;
}

static void *appleII_write_raw(
struct disk *d, unsigned int tracknr, struct stream *s)
{
    struct track_info *ti = &d->di->track[tracknr];
    struct appleII_extra_data *extra_data = handlers[ti->type]->extra_data;
    int stream_is_over = 0;
    char *block = memalloc(ti->len + 1); // buffer for decoded output track 
    unsigned int nr_valid_blocks = 0; // number of valid sectors so far

    // loop until we find all sectors
    while (!stream_is_over && (nr_valid_blocks != ti->nr_sectors)) { 

        int idx_off;
        uint8_t buf[extra_data->data_raw_length]; // buffer for un-decoded data
        uint8_t dat[ti->bytes_per_sector]; // buffer for decoded data
        struct appleII_address_field addrfld; // address header struct

        if((idx_off = appleII_scan_address_field(s, extra_data->address_mark, &addrfld)) < 0) {
            if(idx_off == -1) trk_warn(ti, tracknr, "No AM found");
            if(idx_off == -2) { stream_is_over = 1; break; }
            continue;
        }

        if((addrfld.address_mark != extra_data->address_mark)) {
            // SANITY CHECK. THIS SHOULD NEVER HAPPEN SO BAIL OUT.
            errx(1, "Invalid address mark. This should be impossible!");
        }

        uint8_t cksum = (addrfld.sector ^ addrfld.track ^ addrfld.volume);
        int am_status = 2; // 2 = ok(normal), 1 = warn(nonfatal problem), 0 = bad
		int dm_status = 2; // 2 = ok, 1 = warn, 0 = bad, 3 = missing/not found
        static const char *const status_labels[] = { "BAD", "WARN", "OK", "NOT FOUND" };
        if(addrfld.sector >= ti->nr_sectors) { trk_warn(ti, tracknr, "WARN: Sector out of range: expected %02x <= found %02x", ti->nr_sectors, addrfld.sector); am_status = 1; }
        if(addrfld.track != tracknr/2) { trk_warn(ti, tracknr, "WARN: Unexpected Track value: expected %02x, found %02x", tracknr/2, addrfld.track); am_status = 1; }
        if((addrfld.postamble&0x00FFFF00) != (extra_data->postamble&0x00FFFF00)) { trk_warn(ti, tracknr, "ERROR: Unexpected postamble: expected %06x, found %06x", extra_data->postamble, addrfld.postamble); am_status = 0; }
        if(cksum != addrfld.checksum) { trk_warn(ti, tracknr, "ERROR: Incorrect checksum: expected %02x, found %02x", cksum, addrfld.checksum); am_status = 0; }
        trk_warn(ti, tracknr, "AM %s", status_labels[am_status]);


        // find data mark
        if(appleII_scan_mark(s, extra_data->data_mark, 0x00FFFFFF, 20*8) < 0) {
            trk_warn(ti, tracknr, "ERROR: No data mark for sec=%02x within 20 bytes of address header", addrfld.sector);
			dm_status = 3;
            continue;
        }

        // extract data
        if(appleII_read_block(s, buf, extra_data->data_raw_length) == -1) {
            trk_warn(ti, tracknr, "ERROR: Could not read data for sec=%02x", addrfld.sector);
			dm_status = 0;
            continue;
        }
        
        // data checksum
        int16_t dat_cksum;
        if((dat_cksum=appleII_get_nybble(s, 0)) == -1) {
            trk_warn(ti, tracknr, "ERROR: No data checksum for sec=%02x", addrfld.sector);
			dm_status = 0;
            continue;
        }
                
        // decode data
        uint8_t calc_cksum = extra_data->decode_bytes(buf, dat, extra_data->data_raw_length, ti->bytes_per_sector);

        // verify data checksum
        if(gcr6bw_tb[dat_cksum] != calc_cksum) {
            trk_warn(ti, tracknr, "ERROR: Invalid checksum for sec=%02x: Expected=%02x, Actual=%02x", addrfld.sector, dat_cksum, calc_cksum);
			dm_status = 0;
		}
        
        // find data postamble
        if(appleII_scan_mark(s, extra_data->postamble, 0x00FFFF00, 0) == -1) {
            trk_warn(ti, tracknr, "ERROR: No data postamble for sec=%02x", addrfld.sector);
			dm_status = 0;
        }
		
		// print status
		trk_warn(ti, tracknr, "DM %s", status_labels[dm_status]);
		
        if(!is_valid_sector(ti, addrfld.sector)) {
            memcpy(&block[/*sector_translate[*/addrfld.sector*ti->bytes_per_sector], dat, ti->bytes_per_sector);
            set_sector_valid(ti, addrfld.sector);
            nr_valid_blocks++;
        }
    }
    if(nr_valid_blocks == 0) {
        memfree(block);
        return NULL;
    }
    return block;
}

static void appleII_read_raw(
struct disk *d, unsigned int tracknr, struct tbuf *tbuf)
{
}


void appleII_read_sectors(
    struct disk *d, unsigned int tracknr, struct track_sectors *sectors)
{
    struct track_info *ti = &d->di->track[tracknr];

    sectors->nr_bytes = ti->len;
    sectors->data = memalloc(sectors->nr_bytes);
    memcpy(sectors->data, ti->dat, sectors->nr_bytes);
}



/*
 *   Apple II 16-sector format
 */
struct track_handler appleII_16sector_handler = {
    .density = trkden_single,
    .bytes_per_sector = 256,
    .nr_sectors = 16,
    .write_raw = appleII_write_raw,
    .read_raw = appleII_read_raw,
//    .write_sectors = appleII_write_sectors,
    .read_sectors = appleII_read_sectors,
    .extra_data = & (struct appleII_extra_data) {
        .address_mark = 0xFFD5AA96,
        .data_mark = 0xFFD5AAAD,
        .data_raw_length = 342,
        .postamble = 0xDEAAEB,
        .decode_bytes = appleII_16sector_decode_bytes
    }
};





struct track_handler appleII_13sector_handler = {
    .density = trkden_double,
    .bytes_per_sector = 256,
    .nr_sectors = 13,
    .write_raw = appleII_write_raw,
    .read_raw = appleII_read_raw,
 //   .write_sectors = appleII_write_sectors,
 //    .read_sectors = appleII_read_sectors,
    .extra_data = & (struct appleII_extra_data) {
        .address_mark = 0xFFD5AAAB,
        .data_mark = 0xFFD5AAAD,
        .data_raw_length = 410,
        .postamble = 0xDEAAEB,
        .decode_bytes = appleII_13sector_decode_bytes
    }
};



/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */