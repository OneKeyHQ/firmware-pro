#include "emmc_commands.h"
#include "emmc_commands_macros.h"

#include "fw_keys.h"
#include "se_thd89.h"
#include "thd89_boot.h"

// ######## global vars ########
// SDRAM BUFFER
bootloader_buffer* bl_buffer = (bootloader_buffer*)FMC_SDRAM_BOOLOADER_BUFFER_ADDRESS;
// UI progesss
bool ui_progress_bar_visible = false;

// ######## helpers ########

void ui_progress_bar_visible_clear()
{
    ui_progress_bar_visible = false;
}

static void packet_generate_first(
    uint16_t msg_id, uint32_t msg_size, uint8_t* desc_buff, size_t desc_buff_offset, const uint8_t* src_buff,
    size_t data_len
)
{
    // magic
    desc_buff[0] = (uint8_t)'?';
    desc_buff[1] = (uint8_t)'#';
    desc_buff[2] = (uint8_t)'#';
    // id
    desc_buff[3] = (msg_id >> 8) & 0xFF;
    desc_buff[4] = msg_id & 0xFF;
    // size
    desc_buff[5] = (msg_size >> 24) & 0xFF;
    desc_buff[6] = (msg_size >> 16) & 0xFF;
    desc_buff[7] = (msg_size >> 8) & 0xFF;
    desc_buff[8] = msg_size & 0xFF;
    // data
    memcpy((uint8_t*)desc_buff + MSG_HEADER_LEN_FIRST + desc_buff_offset, src_buff, data_len);
}

static void packet_generate_subsequent(
    uint8_t* desc_buff, size_t desc_buff_offset, const uint8_t* src_buff, size_t data_len
)
{
    // magic
    desc_buff[0] = (uint8_t)'?';
    // data
    memcpy((uint8_t*)desc_buff + MSG_HEADER_LEN_SUBSEQUENT + desc_buff_offset, src_buff, data_len);
}

static bool inline packet_process(
    uint8_t* desc_buff, const uint8_t* src_buff, size_t buff_len, size_t msg_len, size_t* outside_counter
)
{
    // Please note, as the protocol not really designed properly, there are cases
    // that the package is subseq with single byte HEADER "3F", and following two
    // byte DATA is "23 23", which will render the header useless for diff
    // bewtween what packet it is. We added (outside_counter != 0) to the
    // condition to workaround this issue
    if ( src_buff[0] == '?' && src_buff[1] == '#' && src_buff[2] == '#' && (*outside_counter == 0) )
    {
        size_t process_size =
            (msg_len < (buff_len - MSG_HEADER_LEN_FIRST)) ? msg_len : (buff_len - MSG_HEADER_LEN_FIRST);
        memcpy(desc_buff, src_buff + MSG_HEADER_LEN_FIRST, process_size);
        *outside_counter += process_size;
        return true;
    }
    else if ( src_buff[0] == '?' && (*outside_counter != 0) )
    {
        size_t process_size = (msg_len < (buff_len - MSG_HEADER_LEN_SUBSEQUENT))
                                ? msg_len
                                : (buff_len - MSG_HEADER_LEN_SUBSEQUENT);
        memcpy(desc_buff, src_buff + MSG_HEADER_LEN_SUBSEQUENT, process_size);
        *outside_counter += process_size;
        return true;
    }
    else
    {
        return false;
    }
}

// ######## communication handlers ########

static bool callback_encode_bytes(pb_ostream_t* stream, const pb_field_iter_t* field, void* const* arg)
{
    const nanopb_callback_args* cb_arg = (nanopb_callback_args*)*arg;

    if ( cb_arg->payload_size > cb_arg->buffer_size )
    {
        return false;
    }
    if ( !pb_encode_tag_for_field(stream, field) )
    {
        return false;
    }
    if ( !pb_encode_string(stream, cb_arg->buffer, cb_arg->payload_size) )
    {
        return false;
    }

    return true;
}
static bool callback_decode_bytes(pb_istream_t* stream, const pb_field_iter_t* field, void** arg)
{
    const nanopb_callback_args* cb_arg = (nanopb_callback_args*)*arg;

    // Don't care cb_arg->payload_size since not used in here
    // if ( cb_arg->payload_size > cb_arg->buffer_size )
    // {
    //     return false;
    // }

    if ( stream->bytes_left > cb_arg->buffer_size )
    {
        return false;
    }

    memzero(cb_arg->buffer, cb_arg->buffer_size);
    while ( stream->bytes_left )
    {
        if ( !pb_read(stream, (pb_byte_t*)(cb_arg->buffer), stream->bytes_left) )
        {
            return false;
        }
    }

    return true;
}

#ifdef ASYNC_PARSE_NOT_USED
// this is just a cleanner way to keep code here
bool recv_msg_sync_parse(
    uint8_t iface_num, uint32_t msg_size, uint8_t* buf, const pb_msgdesc_t* fields, void* msg
)
{
    // sanity check
    if ( msg_size > SDRAM_BOOLOADER_BUFFER_RECV_LEN )
        return false;

    // new transfer, wipe buffer
    memzero(bl_buffer->recv_buff, SDRAM_BOOLOADER_BUFFER_RECV_LEN);

    // recv buffer count
    size_t recv_count = 0;

    // process initial packet
    if ( !packet_process(bl_buffer->recv_buff + recv_count, buf, IO_PACKET_SIZE, msg_size, &recv_count) )
        return false;

    while ( recv_count < msg_size )
    {
        // read next (blocking, with retry)
        int retry = 0;
        int result = 0;
        switch ( host_channel )
        {
        case CHANNEL_USB:
            while ( true )
            {
                // clear buffer
                memzero(buf, IO_PACKET_SIZE);

                // try
                result = usb_webusb_read_blocking(iface_num, buf, IO_PACKET_SIZE, IO_TIMEOUT);

                if ( result == IO_PACKET_SIZE )
                    // succeed, leave
                    break;

                // try again
                retry++;

                // exceed max retry, error
                if ( retry >= IO_RETRY_MAX )
                    break;
                // not error out since nanopb parse will fail anyways, and this way
                // allows retry from
                outside
                // error_shutdown("\0", "\0", "Error reading from USB.", "Try
                // different USB cable.");
            }
            break;
        case CHANNEL_SLAVE:
            if ( spi_slave_poll(buf) == 0 )
            {
                spi_read_retry(buf);
            }
            break;
        default:
            // error unknown channel
            return false;
            break;
        }

        // process packet
        if ( !packet_process(bl_buffer->recv_buff + recv_count, buf, IO_PACKET_SIZE, msg_size, &recv_count) )
            return false;
    }

    // cleanup outside buffer
    memzero(buf, IO_PACKET_SIZE);

    // decode it
    pb_istream_t istream = pb_istream_from_buffer(bl_buffer->recv_buff, msg_size);
    if ( !pb_decode_noinit(&istream, fields, msg) )
        return false;

    return true;
}

#endif

bool recv_msg_async_parse(
    uint8_t iface_num, uint32_t msg_size, uint8_t* buf, const pb_msgdesc_t* fields, void* msg
)
{
    // sanity check
    if ( msg_size > SDRAM_BOOLOADER_BUFFER_RECV_LEN )
        return false;

    // new transfer, wipe buffer
    memzero(bl_buffer->recv_buff, SDRAM_BOOLOADER_BUFFER_RECV_LEN);

    // recv buffer count
    size_t recv_count_raw = 0;

    // process initial packet
    memcpy(bl_buffer->recv_buff + recv_count_raw, buf, IO_PACKET_SIZE);
    recv_count_raw += IO_PACKET_SIZE;

    // expacted raw receive size
    size_t expected_raw_size = 0;

    if ( msg_size > (IO_PACKET_SIZE - MSG_HEADER_LEN_FIRST) )
    {
        // first packet
        expected_raw_size += IO_PACKET_SIZE;
        // mid packetS
        expected_raw_size += (msg_size - (IO_PACKET_SIZE - MSG_HEADER_LEN_FIRST)) /
                             (IO_PACKET_SIZE - MSG_HEADER_LEN_SUBSEQUENT) * IO_PACKET_SIZE;
        // last packet
        expected_raw_size += ((msg_size - (IO_PACKET_SIZE - MSG_HEADER_LEN_FIRST)) %
                                  (IO_PACKET_SIZE - MSG_HEADER_LEN_SUBSEQUENT) ==
                              0)
                               ? 0
                               : IO_PACKET_SIZE;
    }
    else
    {
        expected_raw_size += MSG_HEADER_LEN_FIRST + msg_size;
    }

    int retry = 0;
    int result = 0;
    while ( recv_count_raw < expected_raw_size )
    {
        // read next (blocking, with retry)
        retry = 0;
        switch ( host_channel )
        {
        case CHANNEL_USB:
            while ( true )
            {
                // try
                result = usb_webusb_read_blocking(
                    iface_num, bl_buffer->recv_buff + recv_count_raw, IO_PACKET_SIZE, IO_TIMEOUT
                );

                if ( result == IO_PACKET_SIZE )
                {
                    // succeed, leave
                    recv_count_raw += result;
                    break;
                }

                // try again
                retry++;

                // exceed max retry, error
                if ( retry >= IO_RETRY_MAX )
                {
                    // error_shutdown("\0", "\0", "Error reading from USB.", "Try
                    // different USB cable.");
                    send_failure_nocheck(
                        iface_num, FailureType_Failure_DataError, "Communication timed out!"
                    );
                    return false;
                    break;
                }
            }
            break;
        case CHANNEL_SLAVE:
            while ( true )
            {
                // try
                result = spi_read_blocking(bl_buffer->recv_buff + recv_count_raw, IO_TIMEOUT);

                if ( result == IO_PACKET_SIZE )
                {
                    // succeed, leave
                    recv_count_raw += result;
                    break;
                }

                // try again
                retry++;

                // exceed max retry, error
                if ( retry >= IO_RETRY_MAX )
                {
                    send_failure_nocheck(
                        iface_num, FailureType_Failure_DataError, "Communication timed out!"
                    );
                    return false;
                    break;
                }
            }
            break;
        default:
            // error unknown channel
            return false;
            break;
        }
    }

    // cleanup outside buffer
    memzero(buf, IO_PACKET_SIZE);

    // parse all packet in the same buffer, this works because packet always
    // larger than data in side it
    size_t recv_raw_index = 0;
    size_t recv_parsed_index = 0;
    while ( recv_parsed_index < msg_size )
    {
        // process packet
        if ( !packet_process(
                 bl_buffer->recv_buff + recv_parsed_index, // parsed write to
                 bl_buffer->recv_buff + recv_raw_index,    // parse from
                 IO_PACKET_SIZE, msg_size, &recv_parsed_index
             ) )
            return false;

        recv_raw_index += IO_PACKET_SIZE;
    }

    // wipe unused space
    memzero(bl_buffer->recv_buff + msg_size, SDRAM_BOOLOADER_BUFFER_RECV_LEN - msg_size);

    // decode it
    pb_istream_t istream = pb_istream_from_buffer(bl_buffer->recv_buff, msg_size);
    if ( !pb_decode_noinit(&istream, fields, msg) )
        return false;

    return true;
}
bool send_msg(
    uint8_t iface_num, uint16_t msg_id, const pb_msgdesc_t* fields, const void* msg, bool ignore_check
)
{
    // new transfer, wipe buffer
    memzero(bl_buffer->send_buff, SDRAM_BOOLOADER_BUFFER_SEND_LEN);

    // send buffer count
    size_t send_count = 0;

    // encode it
    pb_ostream_t ostream = pb_ostream_from_buffer(bl_buffer->send_buff, SDRAM_BOOLOADER_BUFFER_SEND_LEN);
    if ( !pb_encode(&ostream, fields, msg) )
        return false;

    // msg_size
    const uint32_t msg_size = ostream.bytes_written;

    // temp buf
    uint8_t buf[IO_PACKET_SIZE];

    while ( send_count < msg_size )
    {
        // wipe temp buffer
        memzero(buf, IO_PACKET_SIZE);

        // generate packet
        if ( send_count == 0 )
        {
            size_t process_size = (msg_size < (IO_PACKET_SIZE - MSG_HEADER_LEN_FIRST))
                                    ? msg_size
                                    : (IO_PACKET_SIZE - MSG_HEADER_LEN_FIRST);
            packet_generate_first(
                msg_id, msg_size,                  // required header
                buf, 0,                            // target buff, no offset
                bl_buffer->send_buff + send_count, // source buff+offset
                process_size                       // bytes that fits or required
            );
            send_count += process_size;
        }
        else
        {
            size_t process_size = (msg_size < (IO_PACKET_SIZE - MSG_HEADER_LEN_SUBSEQUENT))
                                    ? msg_size
                                    : (IO_PACKET_SIZE - MSG_HEADER_LEN_SUBSEQUENT);
            packet_generate_subsequent(
                buf, 0,                            // target buff, no offset
                bl_buffer->send_buff + send_count, // source buff+offset
                process_size                       // bytes that fits or required
            );
            send_count += process_size;
        }

        // write (blocking)
        int result = 0;
        switch ( host_channel )
        {
        case CHANNEL_USB:
            result = usb_webusb_write_blocking(iface_num, buf, IO_PACKET_SIZE, IO_TIMEOUT);
            break;
        case CHANNEL_SLAVE:
            result = spi_slave_send(buf, IO_PACKET_SIZE, IO_TIMEOUT);
            break;
        default:
            // error unknown channel
            break;
        }

        if ( !ignore_check )
            ensure(sectrue * (result == IO_PACKET_SIZE), NULL);
    }

    return true;
}

// ######## standard messages ########
static void send_success(uint8_t iface_num, const char* text)
{
    MSG_INIT(msg_send, Success);
    MSG_ASSIGN_STRING(msg_send, message, text);
    MSG_SEND(msg_send, Success);
}
void send_success_nocheck(uint8_t iface_num, const char* text)
{
    MSG_INIT(msg_send, Success);
    MSG_ASSIGN_STRING(msg_send, message, text);
    MSG_SEND_NOCHECK(msg_send, Success);
}
static void send_failure(uint8_t iface_num, FailureType type, const char* text)
{
    if ( iface_num == USB_IFACE_NULL )
    {
        return;
    }
    MSG_INIT(msg_send, Failure);
    MSG_ASSIGN_VALUE(msg_send, code, type);
    MSG_ASSIGN_STRING(msg_send, message, text);
    MSG_SEND(msg_send, Failure);
}
void send_failure_nocheck(uint8_t iface_num, FailureType type, const char* text)
{
    if ( iface_num == USB_IFACE_NULL )
    {
        return;
    }
    MSG_INIT(msg_send, Failure);
    MSG_ASSIGN_VALUE(msg_send, code, type);
    MSG_ASSIGN_STRING(msg_send, message, text);
    MSG_SEND_NOCHECK(msg_send, Failure);
}
void send_failure_detailed(uint8_t iface_num, FailureType type, const char* fmt, ...)
{
    if ( iface_num == USB_IFACE_NULL )
    {
        return;
    }
    // format message
    char msg_buff[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(msg_buff, sizeof(msg_buff) / sizeof(char), fmt, argptr);
    va_end(argptr);

    MSG_INIT(msg_send, Failure);
    MSG_ASSIGN_VALUE(msg_send, code, type);
    MSG_ASSIGN_STRING(msg_send, message, msg_buff);
    MSG_SEND(msg_send, Failure);
}
static void send_user_abort(uint8_t iface_num, const char* msg)
{
    MSG_INIT(msg_send, Failure);
    MSG_ASSIGN_VALUE(msg_send, code, FailureType_Failure_ActionCancelled);
    MSG_ASSIGN_STRING(msg_send, message, msg);
    MSG_SEND(msg_send, Failure);
}
void send_user_abort_nocheck(uint8_t iface_num, const char* msg)
{
    MSG_INIT(msg_send, Failure);
    MSG_ASSIGN_VALUE(msg_send, code, FailureType_Failure_ActionCancelled);
    MSG_ASSIGN_STRING(msg_send, message, msg);
    MSG_SEND_NOCHECK(msg_send, Failure);
}

// ######## message handlers ########

int version_compare(uint32_t vera, uint32_t verb)
{
    int a, b;
    a = vera & 0xFF;
    b = verb & 0xFF;
    if ( a != b )
        return a - b;
    a = (vera >> 8) & 0xFF;
    b = (verb >> 8) & 0xFF;
    if ( a != b )
        return a - b;
    a = (vera >> 16) & 0xFF;
    b = (verb >> 16) & 0xFF;
    if ( a != b )
        return a - b;
    a = (vera >> 24) & 0xFF;
    b = (verb >> 24) & 0xFF;
    return a - b;
}

// not used since no where to store them
// static void firmware_headers_store(const uint8_t* const input_buffer, size_t
// buffer_len)
// {
//     // not implemented yet
// }
// static void firmware_headers_retrieve(uint8_t* const outputput_buffer, size_t
// buffer_len)
// {
//     memzero(outputput_buffer, buffer_len);
//     // not implemented yet
// }

update_info_t update_info = {0};

#define MCU_HEADER_MAGIC "OKTV"
#define SE_HEADER_MAGIC  "TF89"
#define BLE_HEADER_MAGIC "5283"

#define UPDATE_INFO_FILE "0:update_res"

static int check_file_contents(uint8_t iface_num, const uint8_t* buffer, uint32_t buffer_len)
{
    vendor_header file_vhdr;
    image_header file_hdr, ble_hdr;
    image_header_th89 thd89_hdr;
    uint8_t* p_data = (uint8_t*)buffer;

    memset(&update_info, 0, sizeof(update_info));

    if ( buffer_len < IMAGE_HEADER_SIZE )
    {
        send_failure(iface_num, FailureType_Failure_ProcessError, "Update file too small!");
        return -1;
    }

    while ( buffer_len )
    {
        if ( update_info.item_count >= UPDATE_ITEM_COUNT )
        {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Update file too many items!");
            return -1;
        }
        // System
        if ( memcmp(p_data, MCU_HEADER_MAGIC, 4) == 0 )
        {

            if ( update_info.mcu_location )
            {
                send_failure(
                    iface_num, FailureType_Failure_ProcessError, "Update file already has System update!"
                );
                return -1;
            }
            vendor_header current_vhdr = {0};
            image_header current_hdr = {0};

            // check firmware header
            // check file header
            ExecuteCheck_MSGS_ADV(
                load_vendor_header(p_data, FW_KEY_M, FW_KEY_N, FW_KEYS, &file_vhdr), sectrue,
                {
                    send_failure(
                        iface_num, FailureType_Failure_ProcessError, "Update file vendor header invalid!"
                    );
                    return -1;
                }
            );
            ExecuteCheck_MSGS_ADV(
                load_image_header(
                    p_data + file_vhdr.hdrlen, FIRMWARE_IMAGE_MAGIC, FIRMWARE_IMAGE_MAXSIZE, file_vhdr.vsig_m,
                    file_vhdr.vsig_n, file_vhdr.vpub, &file_hdr
                ),
                sectrue,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Update file header invalid!");
                    return -1;
                }
            );

            if ( file_hdr.codelen - (FIRMWARE_IMAGE_INNER_SIZE - (file_vhdr.hdrlen + file_hdr.hdrlen)) >
                 FMC_SDRAM_FIRMWARE_P2_LEN )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file P2 too big!");
                return -1;
            }

            // check file firmware hash
            ExecuteCheck_MSGS_ADV(
                check_image_contents_ADV(
                    &file_vhdr, &file_hdr, p_data + file_vhdr.hdrlen + file_hdr.hdrlen, 0, file_hdr.codelen,
                    true
                ),
                sectrue,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Update file hash invalid!");
                    return -1;
                }
            );

            // check file size
            ExecuteCheck_MSGS_ADV(
                (file_vhdr.hdrlen + file_hdr.hdrlen + file_hdr.codelen <= FIRMWARE_IMAGE_MAXSIZE), true,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file is too big!");
                    return -1;
                }
            );

            update_info.mcu_update_info.vendor_changed = sectrue;
            update_info.mcu_update_info.wipe_required = sectrue;

            // vhdr
            if ( load_vendor_header(
                     (const uint8_t*)FIRMWARE_START, FW_KEY_M, FW_KEY_N, FW_KEYS, &current_vhdr
                 ) == sectrue )
            {
                if ( load_image_header(
                         (const uint8_t*)FIRMWARE_START + current_vhdr.hdrlen, FIRMWARE_IMAGE_MAGIC,
                         FIRMWARE_IMAGE_MAXSIZE, current_vhdr.vsig_m, current_vhdr.vsig_n, current_vhdr.vpub,
                         &current_hdr
                     ) == sectrue )
                {
                    uint8_t hash1[32], hash2[32];
                    vendor_header_hash(&file_vhdr, hash1);
                    vendor_header_hash(&current_vhdr, hash2);
                    if ( memcmp(hash1, hash2, 32) == 0 )
                    {
                        // vendor identity match
                        update_info.mcu_update_info.vendor_changed = secfalse;

                        // compare version
                        if ( (version_compare(current_hdr.onekey_version, file_hdr.onekey_version) > 0) )
                        {
                            // new firwmare have lower version
                            char desc[64] = "Firmware downgrade not allowed! Current version is: ";
                            const char* ver_str = format_ver("%d.%d.%d", current_hdr.onekey_version);
                            strcat(desc, ver_str);
                            send_failure(iface_num, FailureType_Failure_ProcessError, desc);
                            return -1;
                        }
                        else
                        {
                            update_info.mcu_update_info.wipe_required = secfalse;
                        }
                    }
                }
            }
            strncpy(
                update_info.items[update_info.item_count].current_version,
                format_ver("%d.%d.%d", current_hdr.onekey_version),
                sizeof(update_info.items[update_info.item_count].current_version)
            );
            strncpy(
                update_info.items[update_info.item_count].new_version,
                format_ver("%d.%d.%d", file_hdr.onekey_version),
                sizeof(update_info.items[update_info.item_count].new_version)
            );

            update_info.items[update_info.item_count].type = UPDATE_MCU;
            update_info.items[update_info.item_count].offset = p_data - buffer;
            update_info.items[update_info.item_count].length =
                file_vhdr.hdrlen + file_hdr.hdrlen + file_hdr.codelen;
            update_info.item_count++;
            update_info.mcu_location = update_info.item_count;

            p_data += file_vhdr.hdrlen + file_hdr.hdrlen + file_hdr.codelen;
            buffer_len -= file_vhdr.hdrlen + file_hdr.hdrlen + file_hdr.codelen;
            continue;
        }
        // SE
        else if ( memcmp(p_data, SE_HEADER_MAGIC, 4) == 0 )
        {
            if ( update_info.se_count >= sizeof(update_info.se_location) )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "Update file too many items!");
                return -1;
            }
            ExecuteCheck_MSGS_ADV(
                load_thd89_image_header(
                    p_data, FIRMWARE_IMAGE_MAGIC_THD89, FIRMWARE_IMAGE_MAXSIZE_THD89, &thd89_hdr
                ),
                sectrue,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "SE header error!");
                    return -1;
                }
            );

            if ( (thd89_hdr.codelen + IMAGE_HEADER_SIZE) > buffer_len )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE firmware length error!");
                return -1;
            }
            strncpy(
                update_info.items[update_info.item_count].current_version, se01_get_version(),
                sizeof(update_info.items[update_info.item_count].current_version)
            );
            strncpy(
                update_info.items[update_info.item_count].new_version,
                format_ver("%d.%d.%d", thd89_hdr.version),
                sizeof(update_info.items[update_info.item_count].new_version)
            );

            update_info.items[update_info.item_count].type = UPDATE_SE;
            update_info.items[update_info.item_count].offset = p_data - buffer;
            update_info.items[update_info.item_count].length = thd89_hdr.codelen + IMAGE_HEADER_SIZE;
            update_info.item_count++;
            update_info.se_location[update_info.se_count] = update_info.item_count;
            update_info.se_count++;
            p_data += thd89_hdr.codelen + IMAGE_HEADER_SIZE;
            buffer_len -= thd89_hdr.codelen + IMAGE_HEADER_SIZE;
            continue;
        }
        // BLE
        else if ( memcmp(p_data, BLE_HEADER_MAGIC, 4) == 0 )
        {
            ExecuteCheck_MSGS_ADV(
                load_ble_image_header(p_data, FIRMWARE_IMAGE_MAGIC_BLE, FIRMWARE_IMAGE_MAXSIZE_BLE, &ble_hdr),
                sectrue,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Update file header invalid!");
                    return -1;
                }
            );

            if ( (ble_hdr.codelen + IMAGE_HEADER_SIZE) > buffer_len )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "BLE firmware length error!");
                return -1;
            }

            char* current_ver = NULL;
            if ( ble_get_version_with_timeout(&current_ver) )
            {
                strncpy(
                    update_info.items[update_info.item_count].current_version, current_ver,
                    sizeof(update_info.items[update_info.item_count].current_version)
                );
            }
            else
            {
                strncpy(
                    update_info.items[update_info.item_count].current_version, "unknown",
                    sizeof(update_info.items[update_info.item_count].current_version)
                );
            }
            if ( ble_hdr.version != 0 )
            {
                strncpy(
                    update_info.items[update_info.item_count].new_version,
                    format_ver("%d.%d.%d", ble_hdr.version),
                    sizeof(update_info.items[update_info.item_count].new_version)
                );
            }
            else
            {
                strncpy(
                    update_info.items[update_info.item_count].new_version, "unknown",
                    sizeof(update_info.items[update_info.item_count].new_version)
                );
            }

            update_info.items[update_info.item_count].type = UPDATE_BLE;
            update_info.items[update_info.item_count].offset = p_data - buffer;
            update_info.items[update_info.item_count].length = ble_hdr.codelen + IMAGE_HEADER_SIZE;
            update_info.item_count++;
            update_info.ble_location = update_info.item_count;

            p_data += ble_hdr.codelen + IMAGE_HEADER_SIZE;
            buffer_len -= ble_hdr.codelen + IMAGE_HEADER_SIZE;
            continue;
        }
        // unknown
        send_failure(iface_num, FailureType_Failure_ProcessError, "Update file unknown type!");
        return -1;
    }
    return 0;
}

void delete_bootloader_update(void)
{
    char new_bootloader_path_legacy[] = "0:boot/bootloader.bin";
    char new_bootloader_path[] = "0:updates/bootloader.bin";
    emmc_fs_file_delete(new_bootloader_path);
    emmc_fs_file_delete(new_bootloader_path_legacy);
}

int check_bootloader_update(image_header* file_hdr)
{
    // read file
    char new_bootloader_path_legacy[] = "0:boot/bootloader.bin";
    char new_bootloader_path[] = "0:updates/bootloader.bin";

    char* new_bootloader_path_p = NULL;

    // check file exists
    if ( emmc_fs_path_exist(new_bootloader_path) )
    {
        new_bootloader_path_p = new_bootloader_path;
    }
    else if ( emmc_fs_path_exist(new_bootloader_path_legacy) )
    {
        new_bootloader_path_p = new_bootloader_path_legacy;
    }
    if ( new_bootloader_path_p == NULL )
        return -1;

    // check file size
    EMMC_PATH_INFO file_info;
    uint8_t* buffer = bl_buffer->misc_buff;
    if ( !emmc_fs_path_info(new_bootloader_path_p, &file_info) )
        return -1;
    if ( file_info.size > BOOTLOADER_IMAGE_MAXSIZE )
        return -1;

    // read file to buffer
    uint32_t num_of_read = 0;
    if ( !emmc_fs_file_read(new_bootloader_path_p, 0, buffer, file_info.size, &num_of_read) )
        return -1;

    // check read size matchs file size
    if ( num_of_read != file_info.size )
        return -1;

    if ( !load_image_header(
             buffer, BOOTLOADER_IMAGE_MAGIC, BOOTLOADER_IMAGE_MAXSIZE, FW_KEY_M, FW_KEY_N, FW_KEYS, file_hdr
         ) )
        return -1;

    if ( !check_image_contents_ADV(NULL, file_hdr, buffer + file_hdr->hdrlen, 0, file_hdr->codelen, true) )
        return -1;

    // check header stated size matchs file size
    if ( (file_hdr->hdrlen + file_hdr->codelen) != file_info.size )
        return -1;

    if ( VERSION_UINT32 > file_hdr->version )
        return 1;

    return 0;
}

extern void enable_usb_tiny_task(bool init_usb);
extern void disable_usb_tiny_task(void);

static int update_firmware_from_file(uint8_t iface_num, const char* path, bool check_only)
{
    uint32_t update_data_len = 0;

    // wipe whole buffer
    memzero(bl_buffer->misc_buff, SDRAM_BOOLOADER_BUFFER_MISC_LEN);

    if ( strlen(path) > FF_MAX_LFN )
    {
        send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file path is too long!");
        return -1;
    }

    PathType path_type;
    ExecuteCheck_MSGS_ADV(emmc_fs_path_type(path, &path_type), true, {
        send_failure(iface_num, FailureType_Failure_ProcessError, "path is invalid!");
        return -1;
    });
    // get file info
    EMMC_PATH_INFO file_info;

    if ( path_type == PATH_FILE )
    {

        ExecuteCheck_MSGS_ADV(emmc_fs_path_info(path, &file_info), true, {
#if PRODUCTION
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
            );
#else
                char emmc_fs_status_str[512];
                emmc_fs_format_status(emmc_fs_status_str, 512);
                send_failure_detailed(
                    iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                    emmc_fs_status_str
                );
#endif
            return -1;
        });

        // check file exist, type, size
        ExecuteCheck_MSGS_ADV((file_info.path_exist), true, {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file not exist!");
            return -1;
        });
        ExecuteCheck_MSGS_ADV((file_info.attrib.directory), false, {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file path is a directory!");
            return -1;
        });
        ExecuteCheck_MSGS_ADV(
            ((file_info.size > 0) && (file_info.size < SDRAM_BOOLOADER_BUFFER_MISC_LEN)), true,
            {
                emmc_fs_file_delete(path);
                send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file size invalid!");
                return -1;
            }
        );

        // read firmware file to ram
        uint32_t processed = 0;
        ExecuteCheck_MSGS_ADV(
            emmc_fs_file_read(path, 0, bl_buffer->misc_buff, file_info.size, &processed), true,
            {
#if PRODUCTION
                send_failure_detailed(
                    iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
                );
#else
                char emmc_fs_status_str[512];
                emmc_fs_format_status(emmc_fs_status_str, 512);
                send_failure_detailed(
                    iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__,
                    str_func_call, emmc_fs_status_str
                );
#endif
                return -1;
            }
        );

        // make sure same size read
        if ( processed != file_info.size )
        {
            emmc_fs_file_delete(path);
            send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware reading error!");
            return -1;
        }

        ExecuteCheck_MSGS_ADV(check_file_contents(iface_num, bl_buffer->misc_buff, file_info.size), 0, {
            emmc_fs_file_delete(path);
            return -1;
        });
    }
    else if ( path_type == PATH_DIR )
    {
        FRESULT res;
        DIR dir;
        FILINFO fno;

        uint8_t header[4] = {0};
        uint32_t br = 0;

        if ( f_opendir(&dir, path) != FR_OK )
        {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file path is a directory!");
            return -1;
        }

        f_chdir(path);

        while ( 1 )
        {
            res = f_readdir(&dir, &fno);
            if ( res != FR_OK || fno.fname[0] == 0 )
            {
                break;
            }
            if ( fno.fattrib & AM_DIR )
            {
                continue;
            }

            if ( strlen(fno.fname) > FF_MAX_LFN )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file path is too long!");
                f_chdir("0:/");
                return -1;
            }

            if ( fno.fsize > IMAGE_HEADER_SIZE )
            {

                ExecuteCheck_MSGS_ADV(emmc_fs_file_read(fno.fname, 0, header, sizeof(header), &br), true, {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Firmware file read error!");
                    f_chdir("0:/");
                    return -1;
                });
                if ( memcmp(header, MCU_HEADER_MAGIC, 4) == 0 || memcmp(header, SE_HEADER_MAGIC, 4) == 0 ||
                     memcmp(header, BLE_HEADER_MAGIC, 4) == 0 )
                {
                    if ( update_data_len + fno.fsize > SDRAM_BOOLOADER_BUFFER_MISC_LEN )
                    {
                        send_failure(
                            iface_num, FailureType_Failure_ProcessError, "Firmware file size too big!"
                        );
                        f_chdir("0:/");
                        return -1;
                    }

                    ExecuteCheck_MSGS_ADV(
                        emmc_fs_file_read(
                            fno.fname, 0, bl_buffer->misc_buff + update_data_len, fno.fsize, &br
                        ),
                        true,
                        {
                            send_failure(
                                iface_num, FailureType_Failure_ProcessError, "Firmware file read error!"
                            );
                            f_chdir("0:/");
                            return -1;
                        }
                    );
                    update_data_len += fno.fsize;

                    if ( !check_only )
                    {
                        emmc_fs_file_delete(fno.fname);
                    }
                }
            }
        }

        f_chdir("0:/");

        if ( update_data_len == 0 )
        {
            return 0;
        }

        ExecuteCheck_MSGS_ADV(check_file_contents(iface_num, bl_buffer->misc_buff, update_data_len), 0, {
            return -1;
        });
    }

    if ( check_only )
        return 0;

    if ( iface_num != USB_IFACE_NULL )
    {
        ui_fadeout();
        ui_update_info_show(update_info);
        ui_fadein();

        int response = ui_input_poll(INPUT_CONFIRM | INPUT_CANCEL, true);
        if ( INPUT_CONFIRM != response )
        {
            send_user_abort_nocheck(iface_num, "Firmware install cancelled");
            return -1;
        }

        send_success_nocheck(iface_num, "Firmware install confirmed");
    }

    enable_usb_tiny_task(iface_num == USB_IFACE_NULL ? true : false);

    if ( iface_num == USB_IFACE_NULL )
    {
        iface_num = USB_IFACE_NUM;
    }

    uint8_t* p_data = bl_buffer->misc_buff;
    uint8_t current_percent = 0;

    display_clear();
    ui_screen_progress_bar_init(NULL, NULL, current_percent);

    // detect firmware type
    if ( update_info.ble_location )
    {
        // bluetooth update
        image_header file_hdr;

        p_data = bl_buffer->misc_buff + update_info.items[update_info.ble_location - 1].offset;
        uint8_t ble_weights = 100 / update_info.item_count;

        // check header
        ExecuteCheck_MSGS_ADV(
            load_ble_image_header(p_data, FIRMWARE_IMAGE_MAGIC_BLE, FIRMWARE_IMAGE_MAXSIZE_BLE, &file_hdr),
            sectrue,
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "Update file header invalid!");
                return -3;
            }
        );

        // make sure we have latest bluetooth status
        ble_refresh_dev_info();

        // return success as bluetooth will be disconnected, we have no way to send
        // result back if the update started via bluetooth
        // send_success_nocheck(iface_num, "Succeed");
        // may have to delay a bit to allow the message be sent out (if ui fade in
        // and out time is too short) hal_delay(50);

        // ui start install
        ui_screen_install_title_clear();
        ui_screen_progress_bar_init("Installing BLE", NULL, current_percent);

        // enter dfu
        ExecuteCheck_MSGS_ADV(bluetooth_enter_dfu(), true, {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Bluetooth enter DFU failed!");
            return -6;
        });

        // install
        uint8_t* p_init = p_data + IMAGE_HEADER_SIZE;
        uint32_t init_data_len = p_init[0] + (p_init[1] << 8);
        ExecuteCheck_MSGS_ADV(
            bluetooth_update(
                p_init + 4, init_data_len, p_data + IMAGE_HEADER_SIZE + BLE_INIT_DATA_LEN,
                file_hdr.codelen - BLE_INIT_DATA_LEN, current_percent, ble_weights,
                ui_screen_install_progress_upload
            ),
            true,
            {
                send_failure(
                    iface_num, FailureType_Failure_ProcessError, "Update bluetooth firmware failed!"
                );
                return -6;
            }
        );

        // delay before kick it out of DFU
        // this is important, otherwise the update may fail
        hal_delay(50);
        // reboot bluetooth
        bluetooth_reset();
        // make sure we have latest bluetooth status (and wait for bluetooth become
        // ready)
        ble_refresh_dev_info();
        current_percent += ble_weights;
    }
    if ( update_info.se_count )
    {
        uint8_t se_weights = 100 / update_info.item_count;
        for ( int i = 0; i < update_info.se_count; i++ )
        {
            p_data = bl_buffer->misc_buff + update_info.items[update_info.se_location[i] - 1].offset;

            image_header_th89 thd89_hdr;
            // se thd89 update
            // check header
            ExecuteCheck_MSGS_ADV(
                load_thd89_image_header(
                    p_data, FIRMWARE_IMAGE_MAGIC_THD89, FIRMWARE_IMAGE_MAXSIZE_THD89, &thd89_hdr
                ),
                sectrue,
                {
                    send_failure(iface_num, FailureType_Failure_ProcessError, "Update file header invalid!");
                    return -3;
                }
            );

            if ( thd89_hdr.i2c_address != 0 )
            {
                thd89_boot_set_address(thd89_hdr.i2c_address);
            }

            char se_ver[16] = {0}, boot_ver[16] = {0};
            strncpy(se_ver, se_get_version_ex(), sizeof(se_ver));
            if ( !se_back_to_boot_progress() )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE back to boot error");
                return -1;
            }

            strncpy(boot_ver, se_get_version_ex(), sizeof(boot_ver));

            if ( !se_verify_firmware(p_data, IMAGE_HEADER_SIZE) )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE verify header error");
                return -1;
            }
            char update_str[128] = {0};

            strcat(update_str, "Installing ");

            switch ( thd89_hdr.i2c_address << 1 )
            {
            case THD89_1ST_ADDRESS:
                strcat(update_str, "1st ");
                break;
            case THD89_2ND_ADDRESS:
                strcat(update_str, "2nd ");
                break;
            case THD89_3RD_ADDRESS:
                strcat(update_str, "3rd ");
                break;
            case THD89_4TH_ADDRESS:
                strcat(update_str, "4th ");
                break;
            }
            strcat(update_str, "SE");
            // ui start install
            ui_screen_install_title_clear();
            ui_screen_progress_bar_init(update_str, NULL, current_percent);

            // install
            if ( !se_update_firmware(
                     p_data + IMAGE_HEADER_SIZE, thd89_hdr.codelen, current_percent, se_weights,
                     ui_screen_install_progress_upload
                 ) )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE update error");
                return -1;
            }

            if ( !se_check_firmware() )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE firmware check error");
                return -1;
            }

            if ( !se_active_app_progress() )
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "SE activate app error");
                return -1;
            }
            current_percent += se_weights;
        }
    }
    if ( update_info.mcu_location )
    {
        // System update
        // check firmware header

        p_data = bl_buffer->misc_buff + update_info.items[update_info.mcu_location - 1].offset;
        uint8_t mcu_weights = 100 / update_info.item_count;

        uint32_t firmware_file_size = update_info.items[update_info.mcu_location - 1].length;

        // ui confirm
        if ( update_info.mcu_update_info.vendor_changed )
        {
            ui_fadeout();
            ui_screen_install_confirm_newvendor_or_downgrade_wipe(
                update_info.items[update_info.mcu_location - 1].new_version
            );
            ui_fadein();
            int response = ui_input_poll(INPUT_CONFIRM | INPUT_CANCEL, true);
            if ( INPUT_CONFIRM != response )
            {
                // We could but should not remove the file if user cancels
                // emmc_fs_file_delete(msg_recv.path);
                ui_fadeout();
                ui_bootloader_first(NULL);
                ui_fadein();
                send_user_abort_nocheck(iface_num, "Firmware install cancelled");
                return -4;
            }
        }

        // ui start install
        ui_screen_install_title_clear();
        ui_screen_progress_bar_init("System Firmware", NULL, current_percent);

        // selectively wipe user data and reset se
        if ( update_info.mcu_update_info.wipe_required )
        {
            se_reset_storage();
        }

        char err_msg[64];

        // write firmware
        ExecuteCheck_MSGS_ADV(
            install_firmware(
                p_data, firmware_file_size, err_msg, sizeof(err_msg), NULL, current_percent, mcu_weights,
                ui_screen_install_progress_upload
            ),
            sectrue,
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, err_msg);
                // wipe invalid firmware, don't care the result as we cannot control, but we have to try
                EMMC_WRAPPER_FORCE_IGNORE(
                    flash_erase_sectors(FIRMWARE_SECTORS, FIRMWARE_INNER_SECTORS_COUNT, NULL)
                );
                return -1;
            }
        );

        ExecuteCheck_MSGS_ADV(
            verify_firmware(NULL, NULL, NULL, NULL, NULL, err_msg, sizeof(err_msg)), sectrue,
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "New firmware hash invalid!");
                // wipe invalid firmware, don't care the result as we cannot control, but we have to try
                EMMC_WRAPPER_FORCE_IGNORE(
                    flash_erase_sectors(FIRMWARE_SECTORS, FIRMWARE_INNER_SECTORS_COUNT, NULL)
                );
                return -1;
            }
        );

        // backup new firmware header (not used since no where to stroe it)
        // As the firmware in flash has is the the same as the one from file, and it has been verified, we
        // could use file_vhdr and file_hdr, instead of read them from flash again.
        // firmware_headers_store((const uint8_t*)FIRMWARE_START, file_vhdr.hdrlen + file_hdr.hdrlen);

        // update progress (final)
        ui_screen_install_progress_upload(100);
    }

    disable_usb_tiny_task();
    return 0;
}

int check_firmware_from_file(uint8_t iface_num)
{
    char path_buffer[FF_MAX_LFN] = {0};
    uint32_t br = 0;
    if ( emmc_fs_file_read(UPDATE_INFO_FILE, 0, path_buffer, FF_MAX_LFN, &br) )
    {
        for ( uint32_t i = 0; i < br; i++ )
        {
            if ( path_buffer[i] < 32 || path_buffer[i] > 126 )
            {
                return -1;
            }
        }
        emmc_fs_file_delete(UPDATE_INFO_FILE);
        return update_firmware_from_file(iface_num, path_buffer, false);
    }
    return -1;
}

int process_msg_FirmwareUpdateEmmc(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, FirmwareUpdateEmmc);
    MSG_RECV_RET_ON_ERR(msg_recv, FirmwareUpdateEmmc);

    image_header boot_hdr;

    int ret = check_bootloader_update(&boot_hdr);

    if ( ret == 0 )
    {

        ret = update_firmware_from_file(iface_num, msg_recv.path, true);
        if ( ret != 0 )
        {
            return ret;
        }

        update_info.boot.type = UPDATE_BOOTLOADER;
        strncpy(
            update_info.boot.current_version, format_ver("%d.%d.%d", VERSION_UINT32),
            sizeof(update_info.boot.current_version)
        );
        strncpy(
            update_info.boot.new_version, format_ver("%d.%d.%d", boot_hdr.version),
            sizeof(update_info.boot.new_version)
        );

        ui_fadeout();
        ui_update_info_show(update_info);
        ui_fadein();

        int response = ui_input_poll(INPUT_CONFIRM | INPUT_CANCEL, true);
        if ( INPUT_CONFIRM != response )
        {
            delete_bootloader_update();
            send_user_abort_nocheck(iface_num, "Firmware install cancelled");
            return -1;
        }

        // reboot to update bootloader
        ExecuteCheck_MSGS_ADV(
            emmc_fs_file_write(UPDATE_INFO_FILE, 0, msg_recv.path, strlen(msg_recv.path), NULL, true, false),
            true,
            {
                send_failure(iface_num, FailureType_Failure_ProcessError, "Update file write failed!");
                return -1;
            }
        );
        send_success_nocheck(iface_num, "Firmware install confirmed");
        hal_delay(100);
        restart();
        return 0;
    }
    else if ( ret == 1 )
    {
        send_failure(iface_num, FailureType_Failure_ProcessError, "Bootloader downgrade!");
        return -1;
    }

    ret = update_firmware_from_file(iface_num, msg_recv.path, false);
    if ( ret != 0 )
    {
        return ret;
    }

    // send_success_nocheck(iface_num, "Succeed");
    if ( msg_recv.has_reboot_on_success && msg_recv.reboot_on_success )
    {
        ui_fadeout();
        ui_screen_done(3, sectrue);
        ui_fadein();
        ui_screen_done(2, secfalse);
        hal_delay(1000);
        ui_screen_done(1, secfalse);
        hal_delay(1000);
        *BOOT_TARGET_FLAG_ADDR = BOOT_TARGET_NORMAL;
        restart();
    }
    else
    {
        ui_fadeout();
        ui_bootloader_first(NULL);
        ui_fadein();
    }
    return 0;
}

int process_msg_EmmcFixPermission(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    // fix onekey data and user data
    ExecuteCheck_MSGS_ADV(emmc_fs_mount(true, true), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif
        return -1;
    });
    ExecuteCheck_MSGS_ADV(emmc_fs_fix_permission(true, true), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif
        return -1;
    });
    ExecuteCheck_MSGS_ADV(emmc_fs_unmount(true, true), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif
        return -1;
    });

    send_success_nocheck(iface_num, "Succeed");
    return 0;
}

int process_msg_EmmcPathInfo(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, EmmcPathInfo);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcPathInfo);

    if ( (strlen(msg_recv.path) == 2) && (msg_recv.path[1] == ':') )
    {
        send_failure(
            iface_num, FailureType_Failure_ProcessError, "Use this command on root path not allowed!"
        );
        return -1;
    }

    EMMC_PATH_INFO file_info;
    ExecuteCheck_MSGS_ADV(emmc_fs_path_info(msg_recv.path, &file_info), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
        return -1;
    });

    MSG_INIT(msg_send, EmmcPath);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, exist, file_info.path_exist);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, size, file_info.size);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, year, file_info.date.year);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, month, file_info.date.month);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, day, file_info.date.day);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, hour, file_info.time.hour);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, minute, file_info.time.minute);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, second, file_info.time.second);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, readonly, file_info.attrib.readonly);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, hidden, file_info.attrib.hidden);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, system, file_info.attrib.system);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, archive, file_info.attrib.archive);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, directory, file_info.attrib.directory);
    MSG_SEND_NOCHECK(msg_send, EmmcPath);
    return 0;
}

int process_msg_EmmcFileRead(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    memzero(bl_buffer->misc_buff, SDRAM_BOOLOADER_BUFFER_MISC_LEN);
    nanopb_callback_args cb_args_send = {
        .buffer = bl_buffer->misc_buff,
        .buffer_size = SDRAM_BOOLOADER_BUFFER_MISC_LEN,
        .payload_size = 0,
    };

    MSG_INIT(msg_recv, EmmcFileRead);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcFileRead);

    if ( msg_recv.file.len > cb_args_send.buffer_size )
    {
        send_failure(iface_num, FailureType_Failure_ProcessError, "File length larger than buffer!");
        return -1;
    }

    // handle progress display
    // please note, this value not calculated localy as we don't know the total
    // (requester intersted) size
    if ( msg_recv.has_ui_percentage )
    {
        char ui_progress_title[] = "Transferring Data";

        // sanity check
        if ( (msg_recv.ui_percentage < 0) || (msg_recv.ui_percentage > 100) )
        {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Percentage invalid!");
            return -1;
        }

        else if ( msg_recv.ui_percentage < 100 )
        {
            if ( !ui_progress_bar_visible )
            {
                ui_fadeout();
                ui_screen_progress_bar_prepare(ui_progress_title, NULL);
                ui_fadein();
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_progress_bar_visible = true;
            }
            else
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
        }
        else if ( msg_recv.ui_percentage == 100 )
        {
            if ( !ui_progress_bar_visible )
            {
                // this is for the instant 100% case, which happens if the file is too
                // small
                ui_fadeout();
                ui_screen_progress_bar_prepare(ui_progress_title, NULL);
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_fadein();
            }
            else
            {
                // normal path
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_progress_bar_visible = false;
            }
            ui_fadeout();
            ui_bootloader_first(NULL);
            ui_fadein();
        }
    }
    else
    {
        if ( ui_progress_bar_visible )
        {
            ui_progress_bar_visible = false;
            display_clear();
            ui_bootloader_first(NULL);
        }
    }

    // get file info
    EMMC_PATH_INFO file_info;
    ExecuteCheck_MSGS_ADV(emmc_fs_path_info(msg_recv.file.path, &file_info), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
        return -1;
    });

    // check file exist, type, size
    ExecuteCheck_MSGS_ADV((file_info.path_exist), true, {
        send_failure(iface_num, FailureType_Failure_ProcessError, "File not exist!");
        return -1;
    });
    ExecuteCheck_MSGS_ADV((file_info.attrib.directory), false, {
        send_failure(iface_num, FailureType_Failure_ProcessError, "File path is a directory!");
        return -1;
    });
    ExecuteCheck_MSGS_ADV(((file_info.size > 0) && (file_info.size < cb_args_send.buffer_size)), true, {
        send_failure(iface_num, FailureType_Failure_ProcessError, "File size invalid!");
        return -1;
    });

    // check read size
    if ( (msg_recv.file.offset + msg_recv.file.len) > file_info.size )
    {
        send_failure(
            iface_num, FailureType_Failure_ProcessError, "Read beyond available file size not allowed!"
        );
    }

    uint32_t processed = 0;
    ExecuteCheck_MSGS_ADV(
        emmc_fs_file_read(
            msg_recv.file.path, msg_recv.file.offset, cb_args_send.buffer, msg_recv.file.len, &processed
        ),
        true,
        {
#if PRODUCTION
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
            );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
            return -1;
        }
    );
    cb_args_send.payload_size = processed;

    MSG_INIT(msg_send, EmmcFile);
    MSG_ASSIGN_REQUIRED_STRING(msg_send, path, msg_recv.file.path);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, len, msg_recv.file.len);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, offset, msg_recv.file.offset);
    MSG_ASSIGN_VALUE(msg_send, processed_byte, processed);
    MSG_ASSIGN_CALLBACK_ENCODE(msg_send, data, &callback_encode_bytes, &cb_args_send);
    MSG_SEND_NOCHECK(msg_send, EmmcFile);

    return 0;
}
int process_msg_EmmcFileWrite(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    memzero(bl_buffer->misc_buff, SDRAM_BOOLOADER_BUFFER_MISC_LEN);
    nanopb_callback_args cb_args_recv = {
        .buffer = bl_buffer->misc_buff,
        .buffer_size = SDRAM_BOOLOADER_BUFFER_MISC_LEN,
        .payload_size = 0,
    };

    MSG_INIT(msg_recv, EmmcFileWrite);
    MSG_ASSIGN_CALLBACK_DECODE(msg_recv, file.data, &callback_decode_bytes, &cb_args_recv);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcFileWrite);

    if ( msg_recv.file.len > cb_args_recv.buffer_size )
    {
        send_failure(iface_num, FailureType_Failure_ProcessError, "File length larger than buffer!");
        return -1;
    }

    // handle progress display
    // please note, this value not calculated localy as we don't know the total
    // (requester intersted) size
    if ( msg_recv.has_ui_percentage )
    {
        char ui_progress_title[] = "Transferring Data";

        // sanity check
        if ( (msg_recv.ui_percentage < 0) || (msg_recv.ui_percentage > 100) )
        {
            send_failure(iface_num, FailureType_Failure_ProcessError, "Percentage invalid!");
            return -1;
        }
        else if ( msg_recv.ui_percentage < 100 )
        {
            if ( !ui_progress_bar_visible )
            {
                ui_fadeout();
                ui_screen_progress_bar_prepare(ui_progress_title, NULL);
                ui_fadein();
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_progress_bar_visible = true;
            }
            else
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
        }
        else if ( msg_recv.ui_percentage == 100 )
        {
            if ( !ui_progress_bar_visible )
            {
                // this is for the instant 100% case, which happens if the file is too
                // small
                ui_fadeout();
                ui_screen_progress_bar_prepare(ui_progress_title, NULL);
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_fadein();
            }
            else
            {
                // normal path
                ui_screen_progress_bar_update(NULL, NULL, msg_recv.ui_percentage);
                ui_progress_bar_visible = false;
            }
            ui_fadeout();
            ui_bootloader_first(NULL);
            ui_fadein();
        }
    }
    else
    {
        if ( ui_progress_bar_visible )
        {
            ui_progress_bar_visible = false;
            display_clear();
            ui_bootloader_first(NULL);
        }
    }

    // get file info
    EMMC_PATH_INFO file_info;
    ExecuteCheck_MSGS_ADV(emmc_fs_path_info(msg_recv.file.path, &file_info), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
        return -1;
    });

    if ( file_info.path_exist )
    {
        ExecuteCheck_MSGS_ADV((file_info.attrib.directory), false, {
            send_failure(iface_num, FailureType_Failure_ProcessError, "File path is a directory!");
            return -1;
        });
        ExecuteCheck_MSGS_ADV((true && msg_recv.overwrite && msg_recv.append), false, {
            send_failure(
                iface_num, FailureType_Failure_ProcessError,
                "File exists but overwrite and append both enabled!"
            );
            return -1;
        });
        ExecuteCheck_MSGS_ADV((false || msg_recv.overwrite || msg_recv.append), true, {
            send_failure(
                iface_num, FailureType_Failure_ProcessError,
                "File exists but overwrite and append both disabled!"
            );
            return -1;
        });
    }

    uint32_t processed = 0;
    ExecuteCheck_MSGS_ADV(
        emmc_fs_file_write(
            msg_recv.file.path, msg_recv.file.offset, cb_args_recv.buffer, msg_recv.file.len, &processed,
            msg_recv.overwrite, msg_recv.append
        ),
        true,
        {
#if PRODUCTION
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
            );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
            return -1;
        }
    );

    MSG_INIT(msg_send, EmmcFile);
    MSG_ASSIGN_REQUIRED_STRING(msg_send, path, msg_recv.file.path);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, len, msg_recv.file.len);
    MSG_ASSIGN_REQUIRED_VALUE(msg_send, offset, msg_recv.file.offset);
    MSG_ASSIGN_VALUE(msg_send, processed_byte, processed);
    MSG_SEND_NOCHECK(msg_send, EmmcFile);
    return 0;
}
int process_msg_EmmcFileDelete(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, EmmcFileDelete);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcFileDelete);

    ExecuteCheck_MSGS_ADV(emmc_fs_file_delete(msg_recv.path), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif

        return -1;
    });

    send_success_nocheck(iface_num, "Succeed");
    return 0;
}

int process_msg_EmmcDirList(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, EmmcDirList);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcDirList);

    const size_t max_list_len = 32 * 1024;

    typedef struct
    {
        // both are '\n' seprated multiline strings
        char list_subdirs[max_list_len];
        char list_files[max_list_len];
    } lists_struct;

    memzero(bl_buffer->misc_buff, SDRAM_BOOLOADER_BUFFER_MISC_LEN);
    lists_struct* temp_buf = (lists_struct*)bl_buffer->misc_buff;

    ExecuteCheck_MSGS_ADV(
        emmc_fs_dir_list(
            msg_recv.path, temp_buf->list_subdirs, max_list_len, temp_buf->list_files, max_list_len
        ),
        true,
        {
#if PRODUCTION
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
            );
#else
            char emmc_fs_status_str[512];
            emmc_fs_format_status(emmc_fs_status_str, 512);
            send_failure_detailed(
                iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed \n%s", __func__, str_func_call,
                emmc_fs_status_str
            );
#endif
            return -1;
        }
    );

    nanopb_callback_args cb_args_send_subdirs = {
        .buffer = (uint8_t*)temp_buf->list_subdirs,
        .buffer_size = max_list_len,
        .payload_size = strlen(temp_buf->list_subdirs),
    };
    nanopb_callback_args cb_args_send_files = {
        .buffer = (uint8_t*)temp_buf->list_files,
        .buffer_size = max_list_len,
        .payload_size = strlen(temp_buf->list_files),
    };

    MSG_INIT(msg_send, EmmcDir);
    MSG_ASSIGN_REQUIRED_STRING(msg_send, path, msg_recv.path);
    MSG_ASSIGN_CALLBACK_ENCODE(msg_send, child_dirs, &callback_encode_bytes, &cb_args_send_subdirs);
    MSG_ASSIGN_CALLBACK_ENCODE(msg_send, child_files, &callback_encode_bytes, &cb_args_send_files);
    MSG_SEND_NOCHECK(msg_send, EmmcDir);

    return 0;
}
int process_msg_EmmcDirMake(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, EmmcDirMake);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcDirMake);

    ExecuteCheck_MSGS_ADV(emmc_fs_dir_make(msg_recv.path), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif

        return -1;
    });

    send_success_nocheck(iface_num, "Succeed");
    return 0;
}
int process_msg_EmmcDirRemove(uint8_t iface_num, uint32_t msg_size, uint8_t* buf)
{
    MSG_INIT(msg_recv, EmmcDirRemove);
    MSG_RECV_RET_ON_ERR(msg_recv, EmmcDirRemove);

    ExecuteCheck_MSGS_ADV(emmc_fs_dir_delete(msg_recv.path), true, {
#if PRODUCTION
        send_failure_detailed(
            iface_num, FailureType_Failure_ProcessError, "%s -> %s Failed", __func__, str_func_call
        );
#else
      char emmc_fs_status_str[512];
      emmc_fs_format_status(emmc_fs_status_str, 512);
      send_failure_detailed(
        iface_num, FailureType_Failure_ProcessError,
        "%s -> %s Failed \n%s", __func__, str_func_call, emmc_fs_status_str);
#endif

        return -1;
    });

    send_success_nocheck(iface_num, "Succeed");
    return 0;
}

// allow following to be unused
void emmc_commands_dummy()
{
    // EMMC_WRAPPER_UNUSED(bl_buffer);
    EMMC_WRAPPER_UNUSED(callback_encode_bytes);
    EMMC_WRAPPER_UNUSED(callback_decode_bytes);
    EMMC_WRAPPER_UNUSED(send_msg);
    EMMC_WRAPPER_UNUSED(send_success);
    EMMC_WRAPPER_UNUSED(send_failure);
    EMMC_WRAPPER_UNUSED(send_failure_detailed);
    EMMC_WRAPPER_UNUSED(send_user_abort);
}
